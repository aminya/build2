// file      : unit-tests/test/script/regex/driver.cxx -*- C++ -*-
// copyright : Copyright (c) 2014-2016 Code Synthesis Ltd
// license   : MIT; see accompanying LICENSE file

#include <regex>
#include <type_traits> // is_pod, is_array

#include <build2/test/script/regex>

using namespace std;
using namespace build2::test::script::regex;

int
main ()
{
  using lc = line_char;
  using ls = line_string;
  using lr = line_regex;

  // Test line_char.
  //
  {
    static_assert (is_pod<lc>::value && !is_array<lc>::value,
                   "line_char must be char-like");

    // Zero-initialed line_char should be the null-char as required by
    // char_traits<>::length() specification.
    //
    assert (lc () == lc::nul);

    line_pool p;

    assert (lc::eof == -1);
    assert (lc::nul == 0);

    enum meta {mn = 'n', mp = 'p'};

    // Special comparison.
    //
    assert (lc ('0') == lc ('0'));
    assert (lc ('0') == '0');
    assert (lc ('n') == mn);
    assert (mn              == static_cast<meta> (lc ('n')));

    assert (lc ('0') != lc ('1'));
    assert (lc ('0') != '1');
    assert (lc ('n') != mp);
    assert (lc ('0') != lc ("0", p));
    assert (lc ('0') != lc (regex ("0"), p));

    assert (lc ('0') < lc ('1'));
    assert (lc ('0') < '1');
    assert (lc ('1') < lc ("0", p));
    assert (lc ('n') < mp);

    assert (lc ('0') <= '1');
    assert (lc ('0') <= lc ('1'));
    assert (lc ('n') <= mn);
    assert (lc ('1') <= lc ("0", p));

    // Literal comparison.
    //
    assert (lc ("a", p)         == lc ("a", p));
    assert (lc ("a", p).literal == lc ("a", p).literal);
    assert (char (lc ("a", p))  == '\a');

    assert (lc ("a", p)   != lc ("b", p));
    assert (!(lc ("a", p) != lc (regex ("a"), p))); // Matches.
    assert (lc ("a", p)   != lc (regex ("b"), p));

    assert (lc ("a", p)   < lc ("b", p));
    assert (!(lc ("a", p) < lc (regex ("a"), p))); // Matches.

    assert (lc ("a", p) <= lc ("b", p));
    assert (lc ("a", p) <= lc (regex ("a"), p));
    assert (lc ("a", p) <  lc (regex ("c"), p));

    // Regex comparison.
    //
    assert (lc ("a", p)           == lc (regex ("a|b"), p));
    assert (lc (regex ("a|b"), p) == lc ("a", p));
  }

  // Test char_traits<line_char>.
  //
  {
    using ct = char_traits<lc>;
    using vc = vector<lc>;

    lc c;
    ct::assign (c, '0');
    assert (c == ct::char_type ('0'));

    assert (ct::to_char_type (c) == c);
    assert (ct::to_int_type (c) == c);

    assert (ct::eq_int_type (c, c));
    assert (!ct::eq_int_type (c, lc::eof));

    assert (ct::eof () == lc::eof);

    assert (ct::not_eof (c) == c);
    assert (ct::not_eof (lc::eof) != lc::eof);

    ct::assign (&c, 1, '1');
    assert (c == ct::int_type ('1'));

    assert (ct::eq (lc ('0'), lc ('0')));
    assert (ct::lt (lc ('0'), lc ('1')));

    vc v1 ({'0', '1', '2'});
    vc v2 (3, lc::nul);

    assert (ct::find (v1.data (), 3, '1') == v1.data () + 1);

    ct::copy (v2.data (), v1.data (), 3);
    assert (v2 == v1);

    v2.push_back (lc::nul);
    assert (ct::length (v2.data ()) == 3);

    // Overlaping ranges.
    //
    ct::move (v1.data () + 1, v1.data (), 2);
    assert (v1 == vc ({'0', '0', '1'}));

    v1 = vc ({'0', '1', '2'});
    ct::move (v1.data (), v1.data () + 1, 2);
    assert (v1 == vc ({'1', '2', '2'}));
  }

  // Test line_string.
  //
  // @@ Add more tests.
  //
  // Note that the following code crashes if compiled with libc++ (LLVM bug
  // #31454).
  //
  // @@ Probably we can overcome it by providing our own allocator for
  //    basic_string instantiation. The function allocate() could allocate some
  //    more elements that would be enough not to corrupt the memory (which
  //    push_back() does).
  // @@ But maybe doesn't worth to bother as the bug seems to get assigned.
  // @@ Heavily affects MacOS where clang++/libc++ is the default setup.
  //
  {
    line_string s;
    s.push_back (line_char ('0'));
    s.push_back (line_char ('1'));
    s.push_back (line_char ('2'));
  }

  // Test line_char_locale and ctype<line_char> (only non-trivial functions).
  //
  {
    using ct = ctype<lc>;

    line_char_locale l;
    assert (has_facet<ct> (l));

    // It is better not to create q facet on stack as it is
    // reference-countable.
    //
    const ct& t (use_facet<ct> (l));
    line_pool p;

    assert (t.is (ct::digit, '0'));
    assert (!t.is (ct::digit, '?'));
    assert (!t.is (ct::digit, lc ("0", p)));

    const lc chars[] = { '0', '?' };
    ct::mask m[2];

    const lc* b (chars);
    const lc* e (chars + 2);

    // Cast flag value to mask type and compare to mask.
    //
    auto fl = [] (ct::mask m, ct::mask f) {return m == f;};

    t.is (b, e, m);
    assert (fl (m[0], ct::digit) && fl (m[1], 0));

    assert (t.scan_is (ct::digit, b, e) == b);
    assert (t.scan_is (0, b, e) == b + 1);

    assert (t.scan_not (ct::digit, b, e) == b + 1);
    assert (t.scan_not (0, b, e) == b);

    {
      char nr[] = "0?";
      lc wd[2];
      t.widen (nr, nr + 2, wd);
      assert (wd[0] == b[0] && wd[1] == b[1]);
    }

    {
      lc wd[] = {'0', lc ("a", p)};
      char nr[2];
      t.narrow (wd, wd + 2, '-', nr);
      assert (nr[0] == '0' && nr[1] == '-');
    }
  }

  // Test regex_traits<line_char>. Functions other that value() are trivial.
  //
  {
    regex_traits<lc> t;

    const int  radix[] = {8, 10}; // Radix 16 is not supported by line_char.
    const char digits[] = "0123456789ABCDEF";

    for (size_t r (0); r < 2; ++r)
    {
      for (int i (0); i < radix[r]; ++i)
        assert (t.value (digits[i], radix[r]) == i);
    }
  }

  // Test line_regex construction.
  //
  {
    line_pool p;
    lr r1 ({lc ("foo", p), lc (regex ("ba(r|z)"), p)}, move (p));

    lr r2 (move (r1));
    assert (regex_match (ls ({lc ("foo", r2.pool), lc ("bar", r2.pool)}), r2));
    assert (!regex_match (ls ({lc ("foo", r2.pool), lc ("ba", r2.pool)}), r2));
  }

  // Test line_regex match.
  //
  {
    line_pool p;

    const lc foo ("foo", p);
    const lc bar ("bar", p);
    const lc baz ("baz", p);
    const lc blank ("", p);

    assert (regex_match  (ls ({foo, bar}), lr ({foo, bar})));
    assert (!regex_match (ls ({foo, baz}), lr ({foo, bar})));

    assert (regex_match (ls ({bar, foo}),
                         lr ({'(', foo, '|', bar, ')', '+'})));

    assert (regex_match (ls ({foo, foo}), lr ({'(', foo, ')', '\\', '1'})));

    assert (regex_match (ls ({foo}),   lr ({lc (regex ("fo+"), p)})));
    assert (regex_match (ls ({foo}),   lr ({lc (regex (".*"), p)})));
    assert (regex_match (ls ({blank}), lr ({lc (regex (".*"), p)})));

    assert (regex_match (ls ({blank, blank, foo}),
                         lr ({blank, '*', foo, blank, '*'})));

    assert (regex_match (ls ({blank, blank, foo}), lr ({'.', '*'})));

    assert (regex_match (ls ({blank, blank}),
                         lr ({blank, '*', foo, '?', blank, '*'})));

    assert (regex_match  (ls ({foo}),      lr ({foo, '{', '1', '}'})));
    assert (regex_match  (ls ({foo, foo}), lr ({foo, '{', '1', ',', '}'})));

    assert (regex_match  (ls ({foo, foo}),
                          lr ({foo, '{', '1', ',', '2', '}'})));

    assert (!regex_match (ls ({foo, foo}),
                          lr ({foo, '{', '3', ',', '4', '}'})));

    assert (regex_match  (ls ({foo}), lr ({'(', '?', '=', foo, ')', foo})));
    assert (regex_match  (ls ({foo}), lr ({'(', '?', '!', bar, ')', foo})));
  }
}