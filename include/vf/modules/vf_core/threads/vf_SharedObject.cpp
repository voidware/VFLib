// Copyright (C) 2008 by Vinnie Falco, this file is part of VFLib.
// See the file LICENSE.txt for licensing information.

class SharedObject::Deleter : LeakChecked <Deleter>
{
private:
  typedef VF_JUCE::SpinLock LockType;

  Deleter () : m_worker ("Deleter")
  {
	m_worker.start ();
  }

  ~Deleter ()
  {
    m_worker.stop_and_wait ();
  }

private:
  static void doDelete (SharedObject* sharedObject)
  {
    delete sharedObject;
  }

public:
  typedef SharedObjectPtr <Deleter> Ptr;

  Worker& getWorker ()
  {
    return m_worker;
  }

  void Delete (SharedObject* sharedObject)
  {
    if (m_worker.isAssociatedWithCurrentThread ())
      delete sharedObject;
    else
      m_worker.call (&Deleter::doDelete, sharedObject);
  }

  static Deleter& getInstance ()
  {
    if (s_instance == nullptr)
    {
      LockType::ScopedLockType lock (*s_mutex);

      if (s_instance == nullptr)
      {
        s_instance = new Deleter;
      }
    }

    return *s_instance;
  }

  static void performAtExit ()
  {
    if (s_instance != nullptr)
    {
      delete s_instance;
      s_instance = nullptr;
    }
  }

private:
  AtomicCounter m_refs;

  ThreadWorkerType <JuceThread> m_worker;

  static Deleter* s_instance;
  static Static::Storage <LockType, Deleter> s_mutex;
};

SharedObject::Deleter* SharedObject::Deleter::s_instance;
Static::Storage <SharedObject::Deleter::LockType, SharedObject::Deleter> SharedObject::Deleter::s_mutex;

//------------------------------------------------------------------------------

void SharedObject::destroySharedObject ()
{
  Deleter::getInstance().Delete (this);
}

void SharedObject::performLibraryAtExit ()
{
  Deleter::performAtExit ();
}
