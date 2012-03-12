// Copyright (C) 2008 by Vinnie Falco, this file is part of VFLib.
// See the file LICENSE.txt for licensing information.

#include "vf/vf_StandardHeader.h"

#if JUCE_MSVC && _DEBUG
#include <crtdbg.h>
#endif

BEGIN_VF_NAMESPACE

#include "vf/modules/vf_core/diagnostic/vf_Debug.h"

namespace Debug {

//------------------------------------------------------------------------------

bool isDebuggerAttached ()
{
  return VF_JUCE::juce_isRunningUnderDebugger ();
}

//------------------------------------------------------------------------------

#if JUCE_DEBUG && defined (juce_breakDebugger)
void breakPoint ()
{
  if (isDebuggerAttached ())
    juce_breakDebugger;
}

#else
void breakPoint ()
{
  jassertfalse
}

#endif

//----------------------------------------------------------------------------

#if JUCE_MSVC && _DEBUG

void setHeapAlwaysCheck (bool bAlwaysCheck)
{
  int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  if (bAlwaysCheck) flags |= _CRTDBG_CHECK_ALWAYS_DF; // on
  else flags &= ~_CRTDBG_CHECK_ALWAYS_DF; // off
  _CrtSetDbgFlag (flags);
}

void setHeapDelayedFree (bool bDelayedFree)
{
  int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  if (bDelayedFree) flags |= _CRTDBG_DELAY_FREE_MEM_DF; // on
  else flags &= ~_CRTDBG_DELAY_FREE_MEM_DF; // off
  _CrtSetDbgFlag (flags);
}

void setHeapReportLeaks (bool bReportLeaks)
{
  int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
  if (bReportLeaks) flags |= _CRTDBG_LEAK_CHECK_DF; // on
  else flags &= ~_CRTDBG_LEAK_CHECK_DF; // off
  _CrtSetDbgFlag (flags);
}

void checkHeap ()
{
  _CrtCheckMemory();
}

#endif

//------------------------------------------------------------------------------

const String getFileNameFromPath (const char* sourceFileName)
{
  return VF_JUCE::File::createFileWithoutCheckingPath (sourceFileName).getFileName();
}

// Returns a String with double quotes escaped
static const String withEscapedQuotes (String const& string)
{
  VF_JUCE::String escaped;

  int i0 = 0;
  int i;
  do
  {
    i = string.indexOfChar (i0, '"');

    if (i == -1)
    {
      escaped << string.substring (i0, string.length());
    }
    else
    {
      escaped << string.substring (i0, i) << "\\\"";
      i0 = i + 1;
    }
  }
  while (i != -1);

  return escaped;
}

// Converts escaped quotes back into regular quotes
static const String withUnescapedQuotes (String const& string)
{
  String unescaped;

  int i0 = 0;
  int i;
  do
  {
    i = string.indexOfChar (i0, '\\');

    if (i == -1)
    {
      unescaped << string.substring (i0, string.length());
    }
    else
    {
      // peek
      if (string.length() > i && string[i+1] == '\"')
      {
        unescaped << string.substring (i0, i) << '"';
        i0 = i + 2;
      }
      else
      {
        unescaped << string.substring (i0, i + 1);
        i0 = i + 1;
      }
    }
  }
  while (i != -1);

  return unescaped;
}

// Converts a String that may contain newlines, into a
// command line where each line is delimited with quotes.
// Any quotes in the actual string will be escaped via \".
String stringToCommandLine (String const& string)
{
  String commandLine;

  int i0 = 0;
  int i;
  for (i = 0; i < string.length(); i++)
  {
    VF_JUCE::juce_wchar c = string[i];

    if (c == '\n')
    {
      if (i0 != 0)
        commandLine << ' ';
      commandLine << '"' << withEscapedQuotes (string.substring (i0, i)) << '"';
      i0 = i+1;
    }
  }

  if (i0 < i)
  {
    if (i0 != 0)
      commandLine << ' ';
    commandLine << '"' << withEscapedQuotes (string.substring (i0, i)) << '"';
  }

  return commandLine;
}

// Converts a command line consisting of multiple quoted strings
// back into a single string with newlines delimiting each quoted
// string. Escaped quotes \" are turned into real quotes.
String commandLineToString (const String& commandLine)
{
  String string;

  bool quoting = false;
  int i0 = 0;
  int i;
  for (i = 0; i < commandLine.length(); i++)
  {
    VF_JUCE::juce_wchar c = commandLine[i];

    if (c == '\\')
    {
      // peek
      if (commandLine.length() > i && commandLine[i+1] == '\"')
      {
        i++;
      }
    }
    else if (c == '"')
    {
      if (!quoting)
      {
        i0 = i + 1;
        quoting = true;
      }
      else
      {
        if (!string.isEmpty ())
          string << '\n';
        string << withUnescapedQuotes (commandLine.substring (i0, i));
        quoting = false;
      }
    }
  }

  return string;
}

}

END_VF_NAMESPACE
