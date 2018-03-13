// Copyright (c) 2009-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#include "core/MojSignal.h"

MojSignalHandler::~MojSignalHandler()
{
}

MojErr MojSignalHandler::handleCancel()
{
	return MojErrNone;
}

MojSlotBase::MojSlotBase()
: m_connected(false)
{
}

MojSlotBase::~MojSlotBase()
{
	MojAssert(!m_connected);
}

void MojSlotBase::cancel()
{
   MojRefCountedPtr<SignalMgr> signalMgr(m_signalMgr);
   if (!signalMgr.get()) return; // no connection

   MojThreadReadGuard guardShared(m_signalMgr->m_lock); // inhibit MojSignalBase dtor
   MojAssert( signalMgr.get() == m_signalMgr.get() );

   auto* signal = m_signalMgr->m_signal;
   if(signal == NULL)
      return;

   // We may get situation when signal dtor were blocked on dtor lock.
   // As well signal destroy should be postponed until we release that dtor lock
   MojRefCountedPtr<MojSignalHandler> signalRef;
   signalRef.resetValid(signal->m_handler);
   if (!signalRef.get())
       return;

   if (m_connected) {
      // On cancel() we may loose last ref to handler which in its turn may
      // control life-time of this slot. So prolong life-time of handler until
      // we finish changing our state.
      MojRefCountedPtr<MojSignalHandler> slotRef;
      slotRef.resetValid(handler());
      MojAssert(slotRef.get()); // to be called we should be referenced from somewhere

      // During MojSignalBase::cancel() we may refer to MojSlotBase::m_signal.
      // So keep it valid until we return back here.
      signal->cancel(this);
      m_signalMgr.reset();
   }
   guardShared.unlock(); // unlock for signalRef->releae()
}

void MojSlotBase::disconnect()
{
	MojAssert(m_signalMgr->m_signal);
	MojAssertMutexLocked(m_signalMgr->m_signal->m_mutex);
	MojAssert(m_connected);

	m_connected = false;
	handler()->release();
}

MojSignalBase::MojSignalBase(MojSignalHandler* handler)
: m_handler(handler)
, m_manager(new MojSlotBase::SignalMgr)
{
	MojAssert(handler);
	m_manager->m_signal = this;
}

MojSignalBase::~MojSignalBase()
{
	MojThreadWriteGuard guardExclusive(m_manager->m_lock);
	MojThreadGuard guard(m_mutex);
	while (!m_slots.empty())
		m_slots.popFront()->disconnect();
	m_manager->m_signal = 0; // mark as zombie
}

void MojSignalBase::connect(MojSlotBase* slot)
{
	MojThreadGuard guard(m_mutex);
	MojAssert(slot);
	MojAssert(!slot->m_connected);

	slot->handler()->retain();
	slot->m_signalMgr = m_manager;
	slot->m_connected = true;
	m_slots.pushBack(slot);
}

void MojSignalBase::cancel(MojSlotBase* slot)
{
	MojThreadGuard guard(m_mutex);
	// in case if we were concurrently called with dtor and slot->disconnect()
	// already were called
    if(slot->m_connected == false)
       return;

	// add ref to make sure that handler isn't destroyed before handleCancel returns.
	MojRefCountedPtr<MojSignalHandler> handler;
	handler.resetValid(m_handler); // in parallel thread disposal might be started already
	if (handler.get()) // we do not call handleCancel() on partially destroyed MojSignalHandler
	{
		guard.unlock();

		MojErr err = m_handler->handleCancel();
		MojErrCatchAll(err);

		guard.lock();

		// Not sure if it's possible but who knows what might happened once we unlock the mutex
		if(slot->m_connected == false)
			return;
	}

	MojAssert(slot && m_slots.contains(slot));
	m_slots.erase(slot);
	slot->disconnect();
    guard.unlock(); // unlock for handler->release()
}

MojErr MojSignal0::call()
{
	MojThreadGuard guard(m_mutex);
	SlotList list = m_slots;
	while (!list.empty()) {
		MojSlotBase0* slot = static_cast<MojSlotBase0*>(list.popFront());
		MojRefCountedPtr<MojSignalHandler> handler(slot->handler());
		m_slots.pushBack(slot);
		guard.unlock();
		MojErr err = slot->invoke();
		MojErrCheck(err);
		guard.lock();
	}
	return MojErrNone;
}

MojErr MojSignal0::fire()
{
	MojThreadGuard guard(m_mutex);
	SlotList list = m_slots;
	while (!list.empty()) {
		MojSlotBase0* slot = static_cast<MojSlotBase0*>(list.popFront());
		MojRefCountedPtr<MojSignalHandler> handler(slot->handler());
		disconnect(slot);
		guard.unlock();
		MojErr err = slot->invoke();
		MojErrCheck(err);
		guard.lock();
	}
	return MojErrNone;
}
