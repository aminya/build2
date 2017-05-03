// file      : build2/bin/target.hxx -*- C++ -*-
// copyright : Copyright (c) 2014-2017 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#ifndef BUILD2_BIN_TARGET_HXX
#define BUILD2_BIN_TARGET_HXX

#include <build2/types.hxx>
#include <build2/utility.hxx>

#include <build2/target.hxx>

namespace build2
{
  namespace bin
  {
    // The obj{} target group.
    //
    class obje: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class obja: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class objs: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class obj: public target
    {
    public:
      using target::target;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    // The lib{} target group.
    //
    class liba: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };

    class libs: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;

      virtual const target_type&
      dynamic_type () const override {return static_type;}
    };

    // Standard layout type compatible with group_view's const target*[2].
    //
    struct lib_members
    {
      const liba* a = nullptr;
      const libs* s = nullptr;
    };

    class lib: public target, public lib_members
    {
    public:
      using target::target;

      virtual group_view
      group_members (action_type) const override;

    public:
      static const target_type static_type;

      virtual const target_type&
      dynamic_type () const override {return static_type;}
    };

    // Windows import library.
    //
    class libi: public file
    {
    public:
      using file::file;

    public:
      static const target_type static_type;
      virtual const target_type& dynamic_type () const {return static_type;}
    };
  }
}

#endif // BUILD2_BIN_TARGET_HXX