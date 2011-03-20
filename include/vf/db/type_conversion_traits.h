// Copyright (C) 2008-2011 by Vincent Falco, All rights reserved worldwide.
// This file is released under the MIT License:
// http://www.opensource.org/licenses/mit-license.php
// Based on ideas from the soci wrapper sqlite back-end

#ifndef __VF_DB_TYPE_CONVERSION_TRAITS_VFHEADER__
#define __VF_DB_TYPE_CONVERSION_TRAITS_VFHEADER__

#include "vf/db/backend.h"

// default conversion (copy in to out)
template<typename T>
struct type_conversion
{
  typedef T base_type;

  static void from_base(base_type const& in, indicator ind, T& out )
  {
    // null not allowed
    if( ind == i_null )
       Throw (Error().fail (__FILE__, __LINE__));
    out = in;
  }

  static void to_base(T const& in, base_type& out, indicator& ind )
  {
    out = in;
    ind = i_ok;
  }
};

// VF_JUCE::Time
template<>
struct type_conversion <VF_JUCE::Time>
{
  //typedef sqlite3_int64 base_type;
  typedef int64 base_type;

  static void from_base (const base_type v, indicator ind, Time& result)
  {
    if (ind == i_null)
    {
      // jassertfalse
      result = Time (0);
    }
    else
    {
      result = Time (v);
    }
  }

  static void to_base (const Time& v, base_type& result, indicator& ind)
  {
    result = v.toMilliseconds ();
    ind = i_ok;
  }
};

#endif
