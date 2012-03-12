// Copyright (C) 2008 by Vinnie Falco, this file is part of VFLib.
// See the file LICENSE.txt for licensing information.

#ifndef __VF_THREAD_VFHEADER__
#define __VF_THREAD_VFHEADER__

#include "vf/modules/vf_core/threads/vf_BoostThread.h"
#include "vf/modules/vf_core/threads/vf_JuceThread.h"

#if 1
typedef JuceThread Thread;
namespace CurrentThread = CurrentJuceThread;
#else
typedef BoostThread Thread;
namespace CurrentThread = CurrentBoostThread;
#endif

#endif
