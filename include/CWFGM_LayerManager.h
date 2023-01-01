/**
 * WISE_Grid_Module: CWFGM_LayerManager.h
 * Copyright (C) 2023  WISE
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "results.h"
#include "linklist.h"
#include "semaphore.h"
#include "objectcache_mt.h"
#include "ICWFGM_GridEngine.h"

#include <string>
#include <boost/intrusive_ptr.hpp>
#include <boost/atomic.hpp>

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(push, 8)
#endif

/////////////////////////////////////////////////////////////////////////////
// CCWFGM_LayerManager

#ifndef DOXYGEN_IGNORE_CODE


class LayerInfo : public MinNode {
    public:
	DECLARE_OBJECT_CACHE_MT(LayerInfo, LayerInfo)

	boost::intrusive_ptr<class ICWFGM_GridEngine>	_this;
	boost::intrusive_ptr<class ICWFGM_GridEngine>	m_engine;
	PolymorphicUserData	m_userData;
	std::uint32_t										cnt;

	LayerInfo()						{ cnt = 0; };

	__INLINE LayerInfo *LN_Succ()	{ return (LayerInfo *)MinNode::LN_Succ(); };
	__INLINE LayerInfo *LN_Pred()	{ return (LayerInfo *)MinNode::LN_Pred(); };
};


class GRIDCOM_API Layer : public MinNode {
    public:
	DECLARE_OBJECT_CACHE_MT(Layer, Layer)

	MinListTempl<LayerInfo>		m_list;
};

#endif


/**
 * Objects inheriting from ICWFGM_GridEngine can be layered and stacked according to the design of a given scenario.  However, the ICWFGM_Scenario object does not manage this
 * layering.  This object manages the layering for each ICWFGM_GridEngine object to use.  layerThreads can be assigned to multiple scenarios, and can be left unused until they are needed.
 */
class GRIDCOM_API CCWFGM_LayerManager : public ICWFGM_CommonBase {
#ifndef DOXYGEN_IGNORE_CODE
public:
	CCWFGM_LayerManager();
	~CCWFGM_LayerManager();

#endif

public:
	virtual NO_THROW HRESULT Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const;
	/**
		Allocates a new layerThread for purposes of recording a sequence of stacking ICWFGM_GridEngines for a given scenario or scenarios.
		\param layerThread	Newly allocated "thread" to record how various ICWFGM_GridEngine objects are stacked.
		\retval E_OUTOFMEMORY	Insufficent memory
		\retval S_OK		A new layerThread is successfully allocated.
	*/
	virtual NO_THROW HRESULT NewLayerThread(Layer **layerThread);
	/**
		Frees a layerThread object which was created from NewLayerThread.
		\param layerThread	A previously allocated "thread" which is no longer needed.
		\retval E_INVALIDARG	The layerThread handle is unknown.
		\retval ERROR_STATE_OBJECT_LOCKED This layerThread object is still used/assigned to some ICWFGM_GridEngine relationship.
		\retval S_OK		The layerThread is successfully deleted.
	*/
	virtual NO_THROW HRESULT DeleteLayerThread(Layer *layerThread);
	/**
		Given a layerThread, and an object (key), returns the next-lower ICWFGM_GridEngine object in the stack.  This method is not thread-safe.
		\param layerThread	Allocated from NewLayerThread
		\param key		The ICWFGM_GridEngine object which wishes to obtain the next-lower ICWFGM_GridEngine object in the stack.
		\param cnt		Pointer to an unused std::uint32_t, which can be used by the client 'key' object.  This value is initialized to 0, and must be set to 0 before clean-up.
		\param pVal		The ICWFGM_GridEngine object which is the next-lower object from the requestor.
		\retval S_OK		The relationship between key and pVal was found and has been returned.
		\retval ERROR_SEVERITY_WARNING No relationship is known for key.
	*/
	virtual NO_THROW HRESULT GetGridEngine(Layer *layerThread, const ICWFGM_GridEngine *key, std::uint32_t **cnt, boost::intrusive_ptr<ICWFGM_GridEngine> *pVal) const;
	/**
		Given a layerThread, and an object (key), stores/records the next-lower ICWFGM_GridENgine object in the stack.  This method is not thread-safe.
		\param layerThread	Allocated from NewLayerThread
		\param key		The ICWFGM_GridEngine object which wishes to obtain the next-lower ICWFGM_GridEngine object in the stack.
		\param layer		The ICWFGM_GridEngine object which is the next-lower object from the requestor.  If this value is NULL, then clean-up may occur.
		\retval E_OUTOFMEMORY	Insufficent memory
		\retval ERROR_STATE_OBJECT_LOCKED This relationship appears to be in use (cnt is not 0).
		\retval S_OK		The relationship has been successfully recorded/updated.
		\retval ERROR_SEVERITY_WARNING Unknown issue. Please report.
	*/
	virtual NO_THROW HRESULT PutGridEngine(Layer *layerThread, const ICWFGM_GridEngine *key, const ICWFGM_GridEngine *theengine);
	virtual NO_THROW HRESULT GetUserData(Layer *layerThread, const ICWFGM_GridEngine *key, PolymorphicUserData *pVal) const;
	virtual NO_THROW HRESULT PutUserData(Layer *layerThread, const ICWFGM_GridEngine *key, const PolymorphicUserData &newVal);

#ifndef DOXYGEN_IGNORE_CODE
protected:
	CRWThreadSemaphore	m_lock;

protected:
	MinListTempl<Layer>	m_list;

protected:
	PolymorphicUserData	m_userData;				// unused by us, just for the user

#endif

};

#ifdef HSS_SHOULD_PRAGMA_PACK
#pragma pack(pop)
#endif
