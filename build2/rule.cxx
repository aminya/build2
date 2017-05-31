// file      : build2/rule.cxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#include <build2/rule.hxx>

#include <build2/scope.hxx>
#include <build2/target.hxx>
#include <build2/context.hxx>
#include <build2/algorithm.hxx>
#include <build2/filesystem.hxx>
#include <build2/diagnostics.hxx>

using namespace std;
using namespace butl;

namespace build2
{
  // file_rule
  //
  // Note that this rule is special. It is the last, fallback rule. If
  // it doesn't match, then no other rule can possibly match and we have
  // an error. It also cannot be ambigious with any other rule. As a
  // result the below implementation bends or ignores quite a few rules
  // that normal implementations should follow. So you probably shouldn't
  // use it as a guide to implement your own, normal, rules.
  //
  match_result file_rule::
  match (action a, target& t, const string&) const
  {
    tracer trace ("file_rule::match");

    // While strictly speaking we should check for the file's existence
    // for every action (because that's the condition for us matching),
    // for some actions this is clearly a waste. Say, perform_clean: we
    // are not doing anything for this action so not checking if the file
    // exists seems harmless. So the overall guideline seems to be this:
    // if we don't do anything for the action (other than performing it
    // on the prerequisites), then we match.
    //
    switch (a)
    {
    case perform_update_id:
      {
        // While normally we shouldn't do any of this in match(), no other
        // rule should ever be ambiguous with the fallback one and path/mtime
        // access is atomic. In other words, we know what we are doing but
        // don't do this in normal rules.

        path_target& pt (t.as<path_target> ());

        // First check the timestamp. This takes care of the special "trust
        // me, this file exists" situations (used, for example, for installed
        // stuff where we know it's there, just not exactly where).
        //
        timestamp ts (pt.mtime ());

        if (ts == timestamp_unknown)
        {
          const path* p (&pt.path ());

          // Assign the path.
          //
          if (p->empty ())
          {
            // Since we cannot come up with an extension, ask the target's
            // derivation function to treat this as prerequisite (just like
            // in search_existing_file()).
            //
            if (pt.derive_extension (true) == nullptr)
            {
              l4 ([&]{trace << "no default extension for target " << pt;});
              return false;
            }

            p = &pt.derive_path ();
          }

          ts = file_mtime (*p);
          pt.mtime (ts);
        }

        if (ts != timestamp_unknown && ts != timestamp_nonexistent)
          return true;

        l4 ([&]{trace << "no existing file for target " << t;});
        return false;
      }
    default:
      return true;
    }
  }

  recipe file_rule::
  apply (action a, target& t) const
  {
    // Update triggers the update of this target's prerequisites so it would
    // seem natural that we should also trigger their cleanup. However, this
    // possibility is rather theoretical so until we see a real use-case for
    // this functionality, we simply ignore the clean operation.
    //
    if (a.operation () == clean_id)
      return noop_recipe;

    // If we have no prerequisites, then this means this file is up to date.
    // Return noop_recipe which will also cause the target's state to be set
    // to unchanged. This is an important optimization on which quite a few
    // places that deal with predominantly static content rely.
    //
    if (!t.has_prerequisites ())
      return noop_recipe;

    // Match all the prerequisites.
    //
    match_prerequisites (a, t);

    // Note that we used to provide perform_update() which checked that this
    // target is not older than any of its prerequisites. However, later we
    // realized this is probably wrong: consider a script with a testscript as
    // a prerequisite; chances are the testscript will be newer than the
    // script and there is nothing wrong with that.
    //
    return default_recipe;
  }

  const file_rule file_rule::instance;

  // alias_rule
  //
  match_result alias_rule::
  match (action, target&, const string&) const
  {
    return true;
  }

  recipe alias_rule::
  apply (action a, target& t) const
  {
    // Inject dependency on our directory (note: not parent) so that it is
    // automatically created on update and removed on clean.
    //
    inject_fsdir (a, t, false);

    match_prerequisites (a, t);
    return default_recipe;
  }

  const alias_rule alias_rule::instance;

  // fsdir_rule
  //
  match_result fsdir_rule::
  match (action, target&, const string&) const
  {
    return true;
  }

  recipe fsdir_rule::
  apply (action a, target& t) const
  {
    // Inject dependency on the parent directory. Note that it must be first
    // (see perform_update_direct()).
    //
    inject_fsdir (a, t);

    match_prerequisites (a, t);

    switch (a)
    {
    case perform_update_id: return &perform_update;
    case perform_clean_id: return &perform_clean;
    default: assert (false); return default_recipe;
    }
  }

  target_state fsdir_rule::
  perform_update (action a, const target& t)
  {
    target_state ts (target_state::unchanged);

    // First update prerequisites (e.g. create parent directories) then create
    // this directory.
    //
    if (!t.prerequisite_targets.empty ())
      ts = straight_execute_prerequisites (a, t);

    // The same code as in perform_update_direct() below.
    //
    const dir_path& d (t.dir); // Everything is in t.dir.

    // Generally, it is probably correct to assume that in the majority
    // of cases the directory will already exist. If so, then we are
    // going to get better performance by first checking if it indeed
    // exists. See try_mkdir() for details.
    //
    if (!exists (d))
    {
      if (verb >= 2)
        text << "mkdir " << d;
      else if (verb)
        text << "mkdir " << t;

      try
      {
        try_mkdir (d);
      }
      catch (const system_error& e)
      {
        fail << "unable to create directory " << d << ": " << e;
      }

      ts |= target_state::changed;
    }

    return ts;
  }

  void fsdir_rule::
  perform_update_direct (action a, const target& t)
  {
    // First create the parent directory. If present, it is always first.
    //
    const target* p (t.prerequisite_targets.empty ()
                     ? nullptr
                     : t.prerequisite_targets[0]);

    if (p != nullptr && p->is_a<fsdir> ())
      perform_update_direct (a, *p);

    // The same code as in perform_update() above.
    //
    const dir_path& d (t.dir);

    if (!exists (d))
    {
      if (verb >= 2)
        text << "mkdir " << d;
      else if (verb)
        text << "mkdir " << t;

      try
      {
        try_mkdir (d);
      }
      catch (const system_error& e)
      {
        fail << "unable to create directory " << d << ": " << e;
      }
    }
  }

  target_state fsdir_rule::
  perform_clean (action a, const target& t)
  {
    // The reverse order of update: first delete this directory,
    // then clean prerequisites (e.g., delete parent directories).
    //
    // Don't fail if we couldn't remove the directory because it
    // is not empty (or is current working directory). In this
    // case rmdir() will issue a warning when appropriate.
    //
    target_state ts (rmdir (t.dir, t)
                     ? target_state::changed
                     : target_state::unchanged);

    if (!t.prerequisite_targets.empty ())
      ts |= reverse_execute_prerequisites (a, t);

    return ts;
  }

  const fsdir_rule fsdir_rule::instance;

  // fallback_rule
  //
  const fallback_rule fallback_rule::instance;
}
