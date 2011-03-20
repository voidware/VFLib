// Copyright (C) 2008-2011 by Vincent Falco, All rights reserved worldwide.
// This file is released under the MIT License:
// http://www.opensource.org/licenses/mit-license.php

#ifndef __VF_THREADWORKER_VFHEADER__
#define __VF_THREADWORKER_VFHEADER__

#include "vf/vf_Bind.h"
#include "vf/vf_CatchAny.h"
#include "vf/vf_Function.h"
#include "vf/vf_Mutex.h"
#include "vf/vf_Worker.h"

#include "vf/vf_Thread.h" // is this needed?

//
// Worker that comes with its own thread of execution used
// to process calls. When there are no calls to process,
// an idle function will run. The idle function must either
// return quickly, or periodically return the result of the
// interruptionPoint() function in order to stop what it is
// doing in order to process new calls.
//

namespace detail {

template <class ThreadType>
class ThreadWorker : public Worker
{
public:
  explicit ThreadWorker (const char* szName)
    : Worker (szName)
    , m_thread (szName)
    , m_calledStart (false)
    , m_calledStop (false)
    , m_shouldStop (false)
  {
  }

  ~ThreadWorker ()
  {
    stop_and_wait ();
  }

  //
  // Starts the worker.
  //
  void start (FunctionType <bool> worker_idle = Function::None(),
			  Function worker_init = Function::None(),
              Function worker_exit = Function::None())
  {
    {
      // TODO: Atomic for this
      VF_NAMESPACE::ScopedLock lock (m_mutex);
      // start() MUST be called.
      vfassert (!m_calledStart);
      m_calledStart = true;
    }

    m_init = worker_init;
    m_idle = worker_idle;
    m_exit = worker_exit;

    open ();

    m_thread.start (Bind (&ThreadWorker::run, this));
  }

  //
  // Stop the thread and optionally wait until it exits.
  // It is safe to call this function at any time and as many times as desired.
  // It is an error to call stop(true) from inside any of the worker functions.
  //
  // During a call to stop() the Worker is closed, and attempts to
  // queue new calls will throw a runtime exception.
  //
  void stop (const bool wait)
  {
    // can't call stop(true) from within a thread function
    vfassert (!wait || !m_thread.isTheCurrentThread ());

    {
      VF_NAMESPACE::ScopedLock lock (m_mutex);

      // start() MUST be called.
      vfassert (m_calledStart);

      // TODO: Atomic for this
      if (!m_calledStop)
      {
        m_calledStop = true;

        {
          VF_NAMESPACE::ScopedUnlock unlock (m_mutex); // getting fancy

          // Atomically queue a stop and close the worker.
          VF_NAMESPACE::ScopedLock lock (ThreadWorker::getMutex ());
        
          call (&ThreadWorker::do_stop, this);

          close ();
        }
      }
    }

    if (wait)
      m_thread.join ();
  }

  // Sugar
  void stop_request () { stop (false); }
  void stop_and_wait () { stop (true); }

  // Should be called periodically by the idle function.
  // There are three possible results:
  //
  // #1 Returns false. The idle function may continue or return.
  // #2 Returns true. The idle function should return as soon as possible.
  // #3 Throws a Thread::Interruption exception.
  //
  // If interruptionPoint returns true or throws, it must
  // not be called again before the threat has the opportunity to reset.
  //
  const Thread::Interrupted interruptionPoint ()
  {
    return m_thread.interruptionPoint ();
  }

  // Interrupts the idle function by queueing a call that does nothing.
  void interrupt ()
  {
    call (Function::None ());
  }

private:
  void reset ()
  {
  }

  void signal ()
  {
    m_thread.interrupt ();
  }

  void do_stop ()
  {
    m_shouldStop = true;
  }

  void run ()
  {
    m_init ();

    for (;;)
    {
      Worker::process ();

      if (m_shouldStop)
        break;

      try
      {
        // HACK! Relying on FunctionType <bool> returning
        // false for None()::operator()()
        bool interrupted = m_idle ();

        if (!interrupted)
          interrupted = interruptionPoint ();

        if (!interrupted)
          m_thread.wait ();
      }
      catch (VF_NAMESPACE::Thread::Interruption&)
      {
        // loop
      }
    }

    m_exit ();
  }

private:
  bool m_calledStart;
  bool m_calledStop;
  bool m_shouldStop;
  VF_NAMESPACE::Mutex m_mutex;
  ThreadType m_thread;
  Function m_init;
  FunctionType <bool> m_idle;
  Function m_exit;
};

}

//------------------------------------------------------------------------------

#if VF_HAVE_JUCE
typedef detail::ThreadWorker <Juce::ThreadType <Juce::Thread::ExceptionBased> > ExceptionWorker;
typedef detail::ThreadWorker <Juce::ThreadType <Juce::Thread::PollingBased> > PollingWorker;

#elif VF_HAVE_BOOST
typedef detail::ThreadWorker <Boost::Thread> ExceptionWorker;
typedef detail::ThreadWorker <Boost::Thread> PollingWorker;

#endif

#endif