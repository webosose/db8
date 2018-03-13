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


#ifndef MOJREFCOUNT_H_
#define MOJREFCOUNT_H_

#include "core/MojCoreDefs.h"
#include "core/MojAtomicInt.h"
#include "core/MojAutoPtr.h"

// allocation utils
void* MojRefCountAlloc(MojSize numBytes);
template<class T>
T* MojRefCountNew(MojSize numElems);
void* MojRefCountRealloc(void* p, MojSize n);
MojInt32 MojRefCountGet(void* p);
void MojRefCountRetain(void* p);
void MojRefCountRelease(void* p);
template<class T>
void MojRefCountRelease(T* p, MojSize numElems);

// ref-counted object
class MojRefCounted : private MojNoCopy
{
public:
	/// Retain reference postponing object disposal
	void retain() { ++m_refCount; }

	/// Retain reference but only if object still alive
	///
	/// @return false if object have no other refs
	bool retainValid();

	/// Release reference and destroy object if this was a last ref
	void release() { if (--m_refCount == 0) delete this; }
	MojInt32 refCount() const { return m_refCount.value(); }

protected:
	virtual ~MojRefCounted() {}

private:
	MojAtomicInt m_refCount;
};

// smart ptr to ref-counted object
template<class T>
struct MojRefCountedPtrRef
{
	explicit MojRefCountedPtrRef(T* p = NULL) : m_p(p) {}
	T* m_p;
};

template<class T>
class MojRefCountedPtr : public MojSmartPtrBase<T>
{
public:
	typedef MojSmartPtrBase<T> Base;
	MojRefCountedPtr(T* p = NULL) : Base(p) { retain(); }
	MojRefCountedPtr(const MojRefCountedPtr& ap) : Base() { if (ap.get() && ap->retainValid()) m_p = ap.m_p; }
	MojRefCountedPtr(MojRefCountedPtrRef<T> ref) : Base() { if (ref.m_p && ref.m_p->retainValid()) m_p = ref.m_p; }
	~MojRefCountedPtr() { release(); }

	void reset(T* p = NULL) { release(); m_p = p; retain(); }
	void resetValid(T* p) { release(); m_p = p && p->retainValid() ? p : 0; }
	MojRefCountedPtr& operator=(const MojRefCountedPtr& rhs) { resetValid(rhs.get()); return *this; }
	MojRefCountedPtr& operator=(MojRefCountedPtrRef<T> rhs) { resetValid(rhs.m_p); return *this; }

	T& operator*() const { MojAssert(Base::m_p); return *Base::m_p; }
	template<class P>
	MojRefCountedPtr& operator=(const MojRefCountedPtr<P>& ap) { resetValid(ap.get()); return *this; }
	template <class P>
	operator MojRefCountedPtrRef<P>() { return MojRefCountedPtrRef<P>(m_p); }
	template<class P>
	operator MojRefCountedPtr<P>() { return MojRefCountedPtr<P>(m_p); }

private:
	using Base::m_p;

	void retain() { if (m_p) m_p->retain(); }
	void release() { if (m_p) m_p->release(); }
};

#include "core/internal/MojRefCountInternal.h"

#endif /* MOJREFCOUNT_H_ */
