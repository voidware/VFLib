//==============================================================================
// 
// Copyright (C) 2008 by Vinnie Falco, this file is part of VFLib.
// 
// Based on SOCI - The C++ Database Access Library:
//   http://soci.sourceforge.net/
// 
// This file is distributed under the following terms:
// 
// Boost Software License - Version 1.0, August 17th, 2003
// 
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// This file incorporates work covered by the following copyright
// and permission notice:
// 
// Copyright (C) 2004 Maciej Sobczak, Stephen Hutton, Mateusz Loskot,
// Pawel Aleksander Fedorynski, David Courtney, Rafal Bobrowski,
// Julian Taylor, Henning Basold, Ilia Barahovski, Denis Arnaud,
// Daniel Lidstr�m, Matthieu Kermagoret, Artyom Beilis, Cory Bennett,
// Chris Weed, Michael Davidsaver, Jakub Stachowski, Alex Ott, Rainer Bauer,
// Martin Muenstermann, Philip Pemberton, Eli Green, Frederic Chateau,
// Artyom Tonkikh, Roger Orr, Robert Massaioli, Sergey Nikulov,
// Shridhar Daithankar, S�ren Meyer-Eppler, Mario Valesco.
// 
//==============================================================================

namespace db {

namespace detail {

ref_counted_statement_base::ref_counted_statement_base (session& s)
  : m_refs (1)
  , m_session (s)
{
}

ref_counted_statement_base ::~ref_counted_statement_base ()
{
}

void ref_counted_statement_base::addref()
{
  ++m_refs;
}

void ref_counted_statement_base::release()
{
  if (--m_refs == 0)
  {
    final_action();
    delete this;
  }
}

std::ostringstream& ref_counted_statement_base::get_query_stream()
{
  return m_session.get_query_stream();
}

//------------------------------------------------------------------------------

ref_counted_statement::ref_counted_statement(session& s, Error& error)
  : ref_counted_statement_base(s)
  , m_error (error)
  , m_st(s)
{
}

void ref_counted_statement::exchange (into_type_ptr const& i)
{
  m_st.exchange(i);
}

void ref_counted_statement::exchange(use_type_ptr const& u)
{
  m_st.exchange(u);
}

void ref_counted_statement::final_action()
{
  m_st.prepare (m_session.get_query_stream().str(), false);

  Error error;
  m_st.execute_and_fetch (error);
  m_error = error;

  m_st.clean_up();
}

}

}
