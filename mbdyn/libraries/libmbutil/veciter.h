/*
 * MBDyn (C) is a multibody analysis code.
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2003
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Iteratore per vettori */


#ifndef VECITER_H
#define VECITER_H

#ifdef USE_MULTITHREAD
#include <signal.h>
#include "ac/spinlock.h"
#endif /* USE_MULTITHREAD */

#include "myassert.h"

/* GetFirst ritorna true ed assegna a TReturn il primo item del vettore.
 * Si assume che il vettore contenga almeno un termine;
 * GetNext ritorna true ed assegna a TReturn il termine successivo se esiste,
 * altrimenti ritorna false e l'assegnamento a TReturn e' unpredictable. */


template <class T>
class Iter {
public:
	virtual ~Iter(void) { NO_OP; };
	virtual bool bGetFirst(T& TReturn) const = 0;
	virtual bool bGetNext(T& TReturn) const = 0;
};

template<class T>
class VecIter : public Iter<T> {
protected:
	mutable T* pStart;
	mutable T* pCount;
	unsigned iSize;

public:
	VecIter(void) : pStart(NULL), pCount(NULL), iSize(0) { NO_OP; };
	VecIter(const T* p, unsigned i) : pStart(p), pCount(p), iSize(i)
	{
		ASSERT(pStart != NULL);
		ASSERT(iSize > 0);
	};

	virtual ~VecIter(void)
	{
		NO_OP;
	};

	void Init(const T* p, unsigned i)
	{
		ASSERT(p != NULL);
		ASSERT(i > 0);

		pStart = pCount = (T*)p;
		iSize = i;
	};

	inline bool bGetFirst(T& TReturn) const
	{
		ASSERT(pStart != NULL);
		ASSERT(iSize > 0);

		pCount = pStart;
		TReturn = *pStart;

		return true;
	};

	inline bool bGetNext(T& TReturn) const
	{
		ASSERT(pStart != NULL);
		ASSERT(iSize > 0);
		ASSERT(pCount >= (T*)pStart);

		++pCount;
		if (pCount == pStart + iSize) {
			return false;
		}

		TReturn = *pCount;

		return true;
	};
};

#ifdef USE_MULTITHREAD
/*
 * The user's class must inherit from InUse to be used by the MT_VecIter
 * the user must reset the inuse flag by using SetInUse() before 
 * concurrently iterating over the array; the iterator provides a 
 * helper routine for this; provide it is called only once and not
 * concurrently.
 */
class InUse {
private:
	mutable sig_atomic_t	inuse;

public:
	InUse(void) : inuse(false) { NO_OP; };
	virtual ~InUse(void) { NO_OP; };

	inline bool bIsInUse(void) const
	{
		/*
		 * If inuse is...
		 * 	true:	leave it as is; return false
		 * 	false:	make it true; return true
		 */
		/* FIXME: make it portable */
		bool b = mbdyn_compare_and_swap(&inuse,
				sig_atomic_t(true), sig_atomic_t(false));

		return !b;
	};
	inline void SetInUse(bool b = false) { inuse = b; };
};

//#define DEBUG_VECITER

template<class T>
class MT_VecIter : public VecIter<T> {
protected:
#ifdef DEBUG_VECITER
	mutable unsigned iCount;
#endif /* DEBUG_VECITER */

public:
	MT_VecIter(void) : VecIter<T>() { NO_OP; };
	MT_VecIter(const T* p, unsigned i) : VecIter<T>(p, i)
	{
		NO_OP;
	};

	virtual ~MT_VecIter(void)
	{
		NO_OP;
	};

	/* NOTE: it must be called only once */
	void ResetAccessData(void)
	{
		ASSERT(pStart != NULL);
		ASSERT(iSize > 0);

		for (unsigned i = 0; i < iSize; i++) {
			pStart[i]->SetInUse();
		}
	}

	inline bool bGetFirst(T& TReturn) const
	{
		ASSERT(pStart != NULL);
		ASSERT(iSize > 0);

#ifdef DEBUG_VECITER
		iCount = 0;
#endif /* DEBUG_VECITER */

		pCount = pStart - 1;

		return bGetNext(TReturn);
	};

	inline bool bGetNext(T& TReturn) const
	{
		ASSERT(pStart != NULL);
		ASSERT(iSize > 0);
		ASSERT(pCount >= (T *)pStart - 1 && pCount < pStart + iSize);

		for (pCount++; pCount < pStart + iSize; pCount++) {
			if (!(*pCount)->bIsInUse()) {
				TReturn = *pCount;
#ifdef DEBUG_VECITER
				iCount++;
#endif /* DEBUG_VECITER */
				return true;
			}
		}

#ifdef DEBUG_VECITER
		silent_cerr("[" << pthread_self() << "]: total=" << iCount
				<< std::endl);
#endif /* DEBUG_VECITER */
		return false;
	};
};

#endif /* USE_MULTITHREAD */

#endif /* VECITER_H */
