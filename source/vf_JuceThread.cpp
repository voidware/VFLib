// Copyright (C) 2008-2011 by Vincent Falco, All rights reserved worldwide.
// This file is released under the MIT License:
// http://www.opensource.org/licenses/mit-license.php

#include "vf/vf_StandardHeader.h"

BEGIN_VF_NAMESPACE

#include "vf/vf_CatchAny.h"
#include "vf/vf_JuceThread.h"

JuceThread::InterruptionModel::InterruptionModel ()
  : m_state (stateRun)
{
}

// Attempt to transition into the wait state.
// Returns true if we should interrupt instead.
//
bool JuceThread::InterruptionModel::do_wait ()
{
  bool interrupted;

  for (;;)
  {
    vfassert (m_state != stateWait);

    // See if we are interrupted
    if (m_state.tryChangeState (stateInterrupt, stateRun))
    {
      // We were interrupted, state is changed to Run.
      // Caller must run now.
      interrupted = true;
      break;
    }
    else if (m_state.tryChangeState (stateRun, stateWait) ||
             m_state.tryChangeState (stateReturn, stateWait))
    {
      // Transitioned to wait.
      // Caller must wait now.
      interrupted = false;
      break;
    }
  }

  return interrupted;
}

// Called when the event times out
//
bool JuceThread::InterruptionModel::do_timeout ()
{
  bool interrupted;

  if (m_state.tryChangeState (stateWait, stateRun))
  {
    interrupted = false;
  }
  else
  {
    vfassert (m_state == stateInterrupt);

    interrupted = true;
  }

  return interrupted;
}

void JuceThread::InterruptionModel::interrupt (JuceThread& thread)
{
  for (;;)
  {
    int const state = m_state;

    if (state == stateInterrupt ||
        state == stateReturn ||
        m_state.tryChangeState (stateRun, stateInterrupt))
    {
      // Thread will see this at next interruption point.
      break;
    }
    else if (m_state.tryChangeState (stateWait, stateRun))
    {
      thread.notify ();
      break;
    }
  }
}

bool JuceThread::InterruptionModel::do_interruptionPoint ()
{
  if (m_state == stateWait)
  {
    // It is impossible for this function
    // to be called while in the wait state.
    Throw (Error().fail (__FILE__, __LINE__));
  }
  else if (m_state == stateReturn)
  {
    // If this goes off it means the thread called the
    // interruption a second time after already getting interrupted.
    Throw (Error().fail (__FILE__, __LINE__));
  }

  // Switch to Return state if we are interrupted
  //bool const interrupted = m_state.tryChangeState (stateInterrupt, stateReturn);
  bool const interrupted = m_state.tryChangeState (stateInterrupt, stateRun);

  return interrupted;
}

//------------------------------------------------------------------------------

bool JuceThread::PollingBased::wait (int milliseconds, JuceThread& thread)
{
  // Can only be called from the current thread
  vfassert (thread.isTheCurrentThread ());

  bool interrupted = do_wait ();

  if (!interrupted)
  {
    interrupted = thread.Thread::wait (milliseconds);

    if (!interrupted)
      interrupted = do_timeout ();
  }

  return interrupted;
}

ThreadBase::Interrupted JuceThread::PollingBased::interruptionPoint (JuceThread& thread)
{
  // Can only be called from the current thread
  vfassert (thread.isTheCurrentThread ());

  const bool interrupted = do_interruptionPoint ();

  return ThreadBase::Interrupted (interrupted);
}

//------------------------------------------------------------------------------

bool JuceThread::ExceptionBased::wait (int milliseconds, JuceThread& thread)
{
  // Can only be called from the current thread
  vfassert (thread.isTheCurrentThread ());

  bool interrupted = do_wait ();

  if (!interrupted)
  {
    interrupted = thread.VF_JUCE::Thread::wait (milliseconds);

    if (!interrupted)
      interrupted = do_timeout ();
  }

  if (interrupted)
    throw Interruption();

  return interrupted;
}

ThreadBase::Interrupted JuceThread::ExceptionBased::interruptionPoint (JuceThread& thread)
{
  // Can only be called from the current thread
  vfassert (thread.isTheCurrentThread ());

  const bool interrupted = do_interruptionPoint ();

  if (interrupted)
    throw Interruption();

  return ThreadBase::Interrupted (false);
}

//------------------------------------------------------------------------------

JuceThread::JuceThread (String name)
  : JuceThreadWrapper (name, *this)
{
}

JuceThread::~JuceThread ()
{
  join ();
}

void JuceThread::start (const Function <void (void)>& f)
{
  m_function = f;
  
  VF_JUCE::Thread::startThread ();
}

void JuceThread::join ()
{
  VF_JUCE::Thread::stopThread (-1);
}

JuceThread::id JuceThread::getId () const
{
  return VF_JUCE::Thread::getThreadId ();
}

bool JuceThread::isTheCurrentThread () const
{
  return VF_JUCE::Thread::getCurrentThreadId () ==
         VF_JUCE::Thread::getThreadId ();
}

void JuceThread::setPriority (int priority)
{
  VF_JUCE::Thread::setPriority (priority);
}

void JuceThread::run ()
{
  CatchAny (m_function);
}

//------------------------------------------------------------------------------

namespace CurrentJuceThread {

ThreadBase::Interrupted interruptionPoint ()
{
  bool interrupted;

  VF_JUCE::Thread* thread = VF_JUCE::Thread::getCurrentThread();

  // Can't use interruption points on the message thread
  vfassert (thread != 0);
  
  if (thread)
  {
    detail::JuceThreadWrapper* threadWrapper = dynamic_cast <detail::JuceThreadWrapper*> (thread);

    vfassert (threadWrapper != 0);

    if (threadWrapper)
      interrupted = threadWrapper->getThreadBase().interruptionPoint ();
    else
      interrupted = false;
  }
  else
  {
    interrupted = false;
  }

  return ThreadBase::Interrupted (interrupted);
}

}

END_VF_NAMESPACE
