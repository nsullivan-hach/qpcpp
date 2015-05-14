/// \file
/// \brief QF/C++ port to Win32 API with cooperative QV kernel (win32-qv)
/// \cond
///***************************************************************************
/// Last updated for version 5.4.0
/// Last updated on  2015-04-30
///
///                    Q u a n t u m     L e a P s
///                    ---------------------------
///                    innovating embedded systems
///
/// Copyright (C) Quantum Leaps, www.state-machine.com.
///
/// This program is open source software: you can redistribute it and/or
/// modify it under the terms of the GNU General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// Alternatively, this program may be distributed and modified under the
/// terms of Quantum Leaps commercial licenses, which expressly supersede
/// the GNU General Public License and are specifically designed for
/// licensees interested in retaining the proprietary status of their code.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program. If not, see <http://www.gnu.org/licenses/>.
///
/// Contact information:
/// Web:   www.state-machine.com
/// Email: info@state-machine.com
///***************************************************************************
/// \endcond

#ifndef qf_port_h
#define qf_port_h

// Win32 event queue and thread types
#define QF_EQUEUE_TYPE       QEQueue
#define QF_OS_OBJECT_TYPE    void*
#define QF_THREAD_TYPE       void*

// The maximum number of active objects in the application
#define QF_MAX_ACTIVE        63

// The number of system clock tick rates
#define QF_MAX_TICK_RATE     2

// various QF object sizes configuration for this port
#define QF_EVENT_SIZ_SIZE    4
#define QF_EQUEUE_CTR_SIZE   4
#define QF_MPOOL_SIZ_SIZE    4
#define QF_MPOOL_CTR_SIZE    4
#define QF_TIMEEVT_CTR_SIZE  4

// QF interrupt disable/enable, see NOTE1
#define QF_INT_DISABLE()     (QP::QF_enterCriticalSection_())
#define QF_INT_ENABLE()      (QP::QF_leaveCriticalSection_())

// Win32 critical section
// QF_CRIT_STAT_TYPE -- not defined
#define QF_CRIT_ENTRY(dummy) QF_INT_DISABLE()
#define QF_CRIT_EXIT(dummy)  QF_INT_ENABLE()

#include "qep_port.h"  // QEP port
#include "qequeue.h"   // Win32-QV needs event-queue
#include "qmpool.h"    // Win32-QV needs memory-pool
#include "qpset.h"     // Win32-QV needs priority-set
#include "qf.h"        // QF platform-independent public interface

namespace QP {

void QF_enterCriticalSection_(void);
void QF_leaveCriticalSection_(void);

// set clock tick rate
void QF_setTickRate(uint32_t ticksPerSec);

// clock tick callback (provided in the app)
void QF_onClockTick(void);

} // namespace QP

//****************************************************************************
// interface used only inside QF, but not in applications

#ifdef QP_IMPL
    // native event queue operations...
    #define QACTIVE_EQUEUE_WAIT_(me_) \
        Q_ASSERT((me_)->m_eQueue.m_frontEvt != static_cast<QEvt const *>(0))
    #define QACTIVE_EQUEUE_SIGNAL_(me_) \
        (QV_readySet_.insert((me_)->m_prio))

    #define QACTIVE_EQUEUE_ONEMPTY_(me_) \
        (QV_readySet_.remove((me_)->m_prio))

    // Win32-QV specific event pool operations
    #define QF_EPOOL_TYPE_  QMPool

    #define QF_EPOOL_INIT_(p_, poolSto_, poolSize_, evtSize_) do { \
        uint_fast32_t fudgedSize = (poolSize_) * QF_WIN32_FUDGE_FACTOR; \
        uint8_t *fudgedSto = new uint8_t[(poolSize_)*QF_WIN32_FUDGE_FACTOR]; \
        Q_ASSERT_ID(210, fudgedSto != (uint8_t *)0); \
        (void)(poolSto_); \
        (p_).init(fudgedSto, fudgedSize, evtSize_); \
    } while (false)

    #define QF_EPOOL_EVENT_SIZE_(p_)  ((p_).getBlockSize())
    #define QF_EPOOL_GET_(p_, e_, m_) \
        ((e_) = static_cast<QEvt *>((p_).get((m_))))
    #define QF_EPOOL_PUT_(p_, e_)     ((p_).put(e_))

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>  // Win32 API

    namespace QP {
        extern QPSet64 QV_readySet_;   // QV-ready set of active objects
        extern HANDLE  QV_win32Event_; // Win32 event to signal events

        // Windows "fudge factor" for oversizing the resources, see NOTE2
        enum {
            QF_WIN32_FUDGE_FACTOR = 100
        };
    } // namespace QP

#endif  // QP_IMPL

// NOTES: ====================================================================
//
// NOTE1:
// QF, like all real-time frameworks, needs to execute certain sections of
// code indivisibly to avoid data corruption. The most straightforward way of
// protecting such critical sections of code is disabling and enabling
// interrupts, which Win32 does not allow.
//
// This QF port uses therefore a single package-scope Win32 critical section
// object QF_win32CritSect_ to protect all critical sections.
//
// Using the single critical section object for all crtical section guarantees
// that only one thread at a time can execute inside a critical section. This
// prevents race conditions and data corruption.
//
// Please note, however, that the Win32 critical section implementation
// behaves differently than interrupt locking. A common Win32 critical section
// ensures that only one thread at a time can execute a critical section, but
// it does not guarantee that a context switch cannot occur within the
// critical section. In fact, such context switches probably will happen, but
// they should not cause concurrency hazards because the critical section
// eliminates all race conditionis.
//
// Unlinke simply disabling and enabling interrupts, the critical section
// approach is also subject to priority inversions. Various versions of
// Windows handle priority inversions differently, but it seems that most of
// them recognize priority inversions and dynamically adjust the priorities of
// threads to prevent it. Please refer to the MSN articles for more
// information.
//
// NOTE2:
// Windows is not a deterministic real-time system, which means that the
// system can occasionally and unexpectedly "choke and freeze" for a number
// of seconds. The designers of Windows have dealt with these sort of issues
// by massively oversizing the resources available to the applications. For
// example, the default Windows GUI message queues size is 10,000 entries,
// which can dynamically grow to an even larger number. Also the stacks of
// Win32 threads can dynamically grow to several megabytes.
//
// In contrast, the event queues, event pools, and stack size inside the
// real-time embedded (RTE) systems can be (and must be) much smaller,
// because you typically can put an upper bound on the real-time behavior
// and the resulting delays.
//
// To be able to run the unmodified applications designed originally for
// RTE systems on Windows, and to reduce the odds of resource shortages in
// this case, the generous QF_WIN32_FUDGE_FACTOR is used to oversize the
// event queues and event pools.
//

#endif // qf_port_h
