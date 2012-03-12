// Copyright (C) 2008 by Vinnie Falco, this file is part of VFLib.
// See the file LICENSE.txt for licensing information.

#include "vf/vf_StandardHeader.h"

BEGIN_VF_NAMESPACE

#include "vf/modules/vf_core/memory/vf_Atomic.h"
#include "vf/modules/vf_core/diagnostic/vf_LeakChecked.h"
#include "vf/modules/vf_core/system/vf_SharedSingleton.h"

#if VF_CHECK_LEAKS

class LeakCheckedBase::CounterBase::Singleton
{
public:
  void push_back (CounterBase* counter)
  {
    CounterBase* next;

    do
    {
      next = m_head.get ();
      counter->m_next = next;
    }
    while (!m_head.compareAndSet (counter, next));
  }

  void detectLeaks ()
  {
    CounterBase* counter = m_head.get ();
    while (counter)
    {
      counter->detectLeak ();
      counter = counter->m_next;
    }
  }

  static Singleton& getInstance ()
  {
    static Singleton* volatile s_instance;
    static Static::Initializer s_initializer;

    if (s_initializer.begin ())
    {
      static Singleton s_object;
      s_instance = &s_object;
      s_initializer.end ();
    }

    return *s_instance;
  }

private:
  Atomic::Pointer <CounterBase> m_head;
};

//------------------------------------------------------------------------------

LeakCheckedBase::CounterBase::CounterBase ()
{
  Singleton::getInstance().push_back (this);
}

void LeakCheckedBase::CounterBase::detectLeaks()
{
  Singleton::getInstance().detectLeaks ();
}

//------------------------------------------------------------------------------

void LeakCheckedBase::performLibraryAtExit ()
{
  CounterBase::detectLeaks ();
}

#endif

END_VF_NAMESPACE
