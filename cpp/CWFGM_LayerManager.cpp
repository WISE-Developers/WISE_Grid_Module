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

#include "GridCom_ext.h"
#include "CWFGM_LayerManager.h"



#ifndef DOXYGEN_IGNORE_CODE

IMPLEMENT_OBJECT_CACHE_MT_NO_TEMPLATE(LayerInfo, LayerInfo, 1024*1024/sizeof(LayerInfo), true, 16)
IMPLEMENT_OBJECT_CACHE_MT_NO_TEMPLATE(Layer, Layer, 128*1024/sizeof(Layer), true, 16)


CCWFGM_LayerManager::CCWFGM_LayerManager() {
}


CCWFGM_LayerManager::~CCWFGM_LayerManager() {
	weak_assert(m_list.IsEmpty());
}

#endif


HRESULT CCWFGM_LayerManager::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	return E_FAIL;
}


HRESULT CCWFGM_LayerManager::NewLayerThread(Layer **layerThread) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE);

	HRESULT hr;

	Layer *layer = new Layer();
	if (!layer)
		hr = E_OUTOFMEMORY;
	else {
		m_list.AddTail(layer);
		hr = S_OK;
	}

	*layerThread = layer;
	return hr;
}


HRESULT CCWFGM_LayerManager::DeleteLayerThread(Layer * layerThread) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE);

	Layer *layer = layerThread;
	if (m_list.NodeIndex(layer) == (std::uint32_t)-1) {
		weak_assert(false);
		return E_INVALIDARG;
	}

	if (layer->m_list.GetCount()) {
		weak_assert(false);
		return ERROR_STATE_OBJECT_LOCKED;
	}

	m_list.Remove(layer);
	delete layer;

	return S_OK;
}


HRESULT CCWFGM_LayerManager::GetGridEngine(Layer * layerThread, const ICWFGM_GridEngine *key, std::uint32_t **cnt, boost::intrusive_ptr<ICWFGM_GridEngine> *pVal) const {
	if (!pVal)									return E_POINTER;
	if (!layerThread)							return E_INVALIDARG;
	Layer *layer = layerThread;

#ifdef _DEBUG
	weak_assert(m_list.NodeIndex(layer) != ((std::uint32_t)-1));
	weak_assert(key);
#endif

	LayerInfo *li = layer->m_list.LH_Head();
	while (li->LN_Succ()) {
		if (li->_this == key) {
			*pVal = li->m_engine;
			if (cnt)
				*cnt = &li->cnt;
			return S_OK;
		}
		li = li->LN_Succ();
	}

	*pVal = NULL;
	if (cnt)
		*cnt = 0;
	return ERROR_SEVERITY_WARNING;
}


HRESULT CCWFGM_LayerManager::PutGridEngine(Layer * layerThread, const ICWFGM_GridEngine *key, const ICWFGM_GridEngine *theengine)  {
	if (!layerThread)							return E_INVALIDARG;
	Layer *layer = layerThread;

    #ifdef _DEBUG
	weak_assert(m_list.NodeIndex(layer) != ((std::uint32_t)-1));
	weak_assert(key);
    #endif

	boost::intrusive_ptr<ICWFGM_GridEngine> engine;

	if (theengine) {
		engine = dynamic_cast<ICWFGM_GridEngine *>(const_cast<ICWFGM_GridEngine *>(theengine));
		if (engine == nullptr) {
			HRESULT retval = E_FAIL;
			weak_assert(0);
			return retval;
		}
	}

	if (engine) {
		LayerInfo *li = layer->m_list.LH_Head();
		while (li->LN_Succ()) {
			if (key == li->_this) {
				li->m_engine = engine;
				return S_OK;
			}
			li = li->LN_Succ();
		}
		
		li = new LayerInfo();
		if (!li) {
			weak_assert(0);
			return E_OUTOFMEMORY;
		}
		li->_this = const_cast<ICWFGM_GridEngine *>(key);
		li->m_engine = engine;
		layer->m_list.AddTail(li);
		return S_OK;
	}
	else {
		LayerInfo *li = layer->m_list.LH_Head();
		while (li->LN_Succ()) {
			if (key == li->_this) {
				weak_assert(li->cnt == 0);
				if (li->cnt != 0) {
					weak_assert(0);
					return ERROR_STATE_OBJECT_LOCKED;
				}
				layer->m_list.Remove(li);
				delete li;
				return S_OK;
			}
			li = li->LN_Succ();
		}
		return ERROR_SEVERITY_WARNING;
	}

	return S_OK;
}


HRESULT CCWFGM_LayerManager::GetUserData(Layer * layerThread, const ICWFGM_GridEngine *key, PolymorphicUserData *pVal) const {
	Layer *layer = layerThread;

#ifdef _DEBUG
	weak_assert(m_list.NodeIndex(layer) != ((std::uint32_t)-1));
	weak_assert(key);
#endif

	LayerInfo *li = layer->m_list.LH_Head();
	while (li->LN_Succ()) {
		if (li->_this == key) {
			*pVal = li->m_userData;
			return S_OK;
		}
		li = li->LN_Succ();
	}

	PolymorphicUserData blank;
	*pVal = blank;
	return ERROR_SEVERITY_ERROR;
}


HRESULT CCWFGM_LayerManager::PutUserData(Layer * layerThread, const ICWFGM_GridEngine *key, const PolymorphicUserData &newVal)  {
	Layer *layer = (Layer *)layerThread;

#ifdef _DEBUG
	weak_assert(m_list.NodeIndex(layer) != ((std::uint32_t)-1));
	weak_assert(key);
#endif

	boost::intrusive_ptr<ICWFGM_GridEngine> engine;

	LayerInfo *li = layer->m_list.LH_Head();
	while (li->LN_Succ()) {
		if (key == li->_this) {
			li->m_userData = newVal;
			return S_OK;
		}
		li = li->LN_Succ();
	}
		
	return ERROR_SEVERITY_ERROR;
}
