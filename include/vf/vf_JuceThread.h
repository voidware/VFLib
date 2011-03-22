// Copyright (C) 2008-2011 by Vincent Falco, All rights reserved worldwide.
// This file is released under the MIT License:
// http://www.opensource.org/licenses/mit-license.php

#ifndef __VF_JUCETHREAD_VFHEADER__
#define __VF_JUCETHREAD_VFHEADER__

#if VF_HAVE_JUCE

#include "vf/vf_Function.h"
#include "vf/vf_Mutex.h"
#include "vf/vf_Thread.h"
#include "vf/vf_String.h"

//
// Implementation of vf::Thread using Juce
//

namespace Juce {

class Thread : public VF_JUCE::Thread
{
public:
  typedef VF_JUCE::Thread::ThreadID id;
  typedef detail::Thread::Interrupted Interrupted;
  typedef detail::Thread::Interruption Interruption;

public:
  //
  // Interruption models
  //

  class InterruptionModel
  {
  protected:
    InterruptionModel ();
    bool do_wait ();

  public:
    void interrupt (Thread& thread);

  protected:
    bool do_interruptionPoint ();

  protected:
    enum
    {
      stateReset = 0,
      stateSignaled = 1,
      stateWaiting = 2
    };

    VF_JUCE::Atomic <int> m_state;
  };

  class ExceptionBased : public InterruptionModel
  {
  public:
    void wait (Thread& thread);
    Interrupted interruptionPoint (Thread& thread);
  };

  class PollingBased : public InterruptionModel
  {
  public:
    void wait (Thread& thread);
    Interrupted interruptionPoint (Thread& thread);
  };

public:
  explicit Thread (const VF_NAMESPACE::String& name);
  ~Thread ();

  template <class Callable>
  void start (const Callable& c)
  {
    m_callable = c;
    VF_JUCE::Thread::startThread ();
  }

  void join ();

  id getId () const;

  // only valid if the thread is running
  bool isTheCurrentThread () const;

  void setPriority (int priority);

  virtual Interrupted interruptionPoint () = 0;

private:
  void run ();

  Function m_callable;
};

template <class InterruptionType>
class ThreadType : public Thread
{
public:
  ThreadType (const VF_NAMESPACE::String& name) : Thread (name)
  {
  }

  void wait ()
  {
    m_model.wait (*this);
  }

  void interrupt ()
  {
    m_model.interrupt (*this);
  }

  Interrupted interruptionPoint ()
  {
    return m_model.interruptionPoint (*this);
  }

private:
  InterruptionType m_model;
};

namespace CurrentThread {

extern Thread::id getId ();

// Avoid this function because the implementation is slow.
// Use Juce::Thread::interruptionPoint() instead.
extern Thread::Interrupted interruptionPoint ();

extern void setPriority (int priority);

}

}

#endif

#endif

