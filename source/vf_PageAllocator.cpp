// Copyright (C) 2008-2011 by Vincent Falco, All rights reserved worldwide.
// This file is released under the MIT License:
// http://www.opensource.org/licenses/mit-license.php

#include "vf/vf_StandardHeader.h"

BEGIN_VF_NAMESPACE

#include "vf/vf_MemoryAlignment.h"
#include "vf/vf_PageAllocator.h"

#define LOG_GC 0

namespace {

// This is the upper limit on the amount of physical memory an instance of the
// allocator will allow. Going over this limit means that consumers cannot keep
// up with producers, and application logic should be re-examined.
//
// TODO: ENFORCE THIS GLOBALLY? MEASURE IN KILOBYTES AND FORCE KILOBYTE PAGE SIZES
const size_t hardLimitMegaBytes = 4 * 256;

}

//PageAllocator GlobalPageAllocator::s_allocator (128);

//------------------------------------------------------------------------------
/*

Implementation notes

- There are two pools, the 'hot' pool and the 'cold' pool.

- When a new page is needed we pop from the 'fresh' stack of the hot pool.

- When a page is deallocated it is pushed to the 'garbage' stack of the hot pool.

- Every so often, a garbage collection is performed on a separate thread.
  During collection, fresh and garbage are swapped in the cold pool.
  Then, the hot and cold pools are atomically swapped.

*/
//------------------------------------------------------------------------------

struct PageAllocator::Page : List::Node
{
  explicit Page (PageAllocator* const allocator) : m_allocator (*allocator) { }
  PageAllocator& getAllocator () const { return m_allocator; }
private:
  PageAllocator& m_allocator;
};

inline void* PageAllocator::fromPage (Page* const p)
{
  return reinterpret_cast <char*> (p) +
    Memory::sizeAdjustedForAlignment (sizeof (Page));
}

inline PageAllocator::Page* PageAllocator::toPage (void* const p)
{
  return reinterpret_cast <Page*> (
    (reinterpret_cast <char *> (p) -
      Memory::sizeAdjustedForAlignment (sizeof (Page))));
}

//------------------------------------------------------------------------------

PageAllocator::PageAllocator (const size_t pageBytes)
  : m_pageBytes (pageBytes)
  , m_pageBytesAvailable (pageBytes - Memory::sizeAdjustedForAlignment (sizeof (Page)))
  , m_newPagesLeft ((hardLimitMegaBytes * 1024 * 1024) / m_pageBytes)
#if LOG_GC
  , m_swaps (0)
#endif
{
  m_hot = &m_pool[0];
  m_cold = &m_pool[1];

  startOncePerSecond ();
}

PageAllocator::~PageAllocator ()
{
  endOncePerSecond ();

#if LOG_GC
  vfassert (m_used.is_reset ());
#endif

  free (m_pool[0]);
  free (m_pool[1]);

#if LOG_GC
  vfassert (m_total.is_reset ());
#endif
}

//------------------------------------------------------------------------------

void* PageAllocator::allocate ()
{
  Page* page = m_hot->fresh.pop_front ();

  if (!page)
  {
    const bool exhausted = m_newPagesLeft.release ();
    if (exhausted)
      Throw (Error().fail (__FILE__, __LINE__,
        TRANS("the limit of memory allocations was reached")));

    page = new (::malloc (m_pageBytes)) Page (this);
    if (!page)
      Throw (Error().fail (__FILE__, __LINE__,
        TRANS("a memory allocation failed")));

#if LOG_GC
    m_total.addref ();
#endif
  }

#if LOG_GC
  m_used.addref ();
#endif

  return fromPage (page);
}

void PageAllocator::deallocate (void* const p)
{
  Page* const page = toPage (p);
  PageAllocator& allocator = page->getAllocator ();

  allocator.m_hot->garbage.push_front (page);

#if LOG_GC
  allocator.m_used.release ();
#endif
}

void PageAllocator::doOncePerSecond ()
{
  // free one garbage page
  Page* page = m_cold->garbage.pop_front ();
  if (page)
  {
    page->~Page ();
    ::free (page);
  }

  m_cold->fresh.swap (m_cold->garbage);

  // Swap atomically with respect to m_hot
  Pool* temp = m_hot;
  m_hot = m_cold;
  m_cold = temp;

#if LOG_GC
  String s;
  s << "swap " << String (++m_swaps);
  s << " (" << String (m_used.get ()) << "/"
    << String (m_total.get ()) << " of "
    << String (m_newPagesLeft.get ()) << ")";
  VF_JUCE::Logger::outputDebugString (s);
#endif
}

void PageAllocator::free (List& list)
{
  for (;;)
  {
    Page* const page = list.pop_front ();

    if (page)
    {
      page->~Page ();
      ::free (page);

#if LOG_GC
      m_total.release ();
#endif
    }
    else
    {
      break;
    }
  }
}

void PageAllocator::free (Pool& pool)
{
  free (pool.fresh);
  free (pool.garbage);
}

END_VF_NAMESPACE