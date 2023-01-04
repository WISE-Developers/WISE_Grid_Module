/**
 * WISE_Grid_Module: CWFGM_Target.h
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

#ifdef _MSC_VER
#include <winerror.h>
#endif

#include "GridCom_ext.h"
#include "CWFGM_Target.h"
#include "angles.h"
#include "points.h"

#include "CoordinateConverter.h"

#include <errno.h>
#include <stdio.h>

#include <boost/algorithm/string.hpp>

#include "XY_PolyType.h"


ICWFGM_Target::ICWFGM_Target() {
}


ICWFGM_Target::~ICWFGM_Target() {
}


CCWFGM_Target::CCWFGM_Target() {
	m_bRequiresSave = false;
	m_xmin = m_ymin = m_xmax = m_ymax = -99.0;
	m_flags = 0;
}


CCWFGM_Target::CCWFGM_Target(const CCWFGM_Target& toCopy) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore*)&toCopy.m_lock, SEM_FALSE);

	m_gisURL = toCopy.m_gisURL;
	m_gisLayer = toCopy.m_gisLayer;
	m_gisUID = toCopy.m_gisUID;
	m_gisPWD = toCopy.m_gisPWD;

	m_xmin = toCopy.m_xmin;
	m_ymin = toCopy.m_ymin;
	m_xmax = toCopy.m_xmax;
	m_ymax = toCopy.m_ymax;

	m_flags = toCopy.m_flags;

	RefNode<XY_PolyType>* pn = toCopy.m_targets.LH_Head(), * npn;
	while (pn->LN_Succ()) {
		XY_PolyType* p = new XY_PolyType(*pn->LN_Ptr());
		npn = new RefNode<XY_PolyType>();
		npn->LN_Ptr(p);
		m_targets.AddTail(npn);
		pn = pn->LN_Succ();
	}

	m_bRequiresSave = false;
}


CCWFGM_Target::~CCWFGM_Target() {
}


HRESULT CCWFGM_Target::AddPointSet(const XY_PolyConst& xy_pairs, std::uint32_t* set_break) {
	if (!set_break)												return E_POINTER;

	std::uint32_t point_count = xy_pairs.NumPoints();
	if (!point_count)											return E_FAIL;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)												return ERROR_SCENARIO_SIMULATION_RUNNING;

	RefNode<XY_PolyType>* pn = NULL;
	XY_PolyType* poly = NULL;
	try {
		pn = new RefNode<XY_PolyType>();
		poly = new XY_PolyType(xy_pairs);
		poly->m_publicFlags = XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_MULTIPOINT;
		poly->CleanPoly(0.0, XY_PolyType::PolygonType::MULTIPOINT);
		m_bRequiresSave = true;
	}
	catch (std::bad_alloc& cme) {
		if (poly)	delete poly;
		if (pn)		delete pn;
		return E_OUTOFMEMORY;
	}

	*set_break = m_targets.GetCount();
	pn->LN_Ptr(poly);
	m_targets.AddTail(pn);

	poly->RescanRanges();

	if (!(*set_break)) {
		m_xmin = poly->Min_X();
		m_ymin = poly->Min_Y();
		m_xmax = poly->Max_X();
		m_ymax = poly->Max_Y();
	}
	else {
		if (poly->Min_X() < m_xmin)	m_xmin = poly->Min_X();
		if (poly->Min_Y() < m_ymin)	m_ymin = poly->Min_Y();
		if (poly->Max_X() > m_xmax)	m_xmax = poly->Max_X();
		if (poly->Max_Y() > m_ymax)	m_ymax = poly->Max_Y();
	}
	return S_OK;
}


HRESULT CCWFGM_Target::SetPointSet(std::uint32_t index, const XY_PolyConst & xy_pairs) {
	if (!xy_pairs.NumPoints())										return E_POINTER;

	std::uint32_t point_count = xy_pairs.NumPoints();
	if (!point_count)									return E_FAIL;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)										return ERROR_SCENARIO_SIMULATION_RUNNING;

	bool add = false;
	RefNode<XY_PolyType>* pn = m_targets.IndexNode(index);
	if (!pn) {
		if (index == 0) {
			try {
				pn = new RefNode<XY_PolyType>();
				add = true;
			}
			catch (std::bad_alloc& cme) {
				return E_OUTOFMEMORY;
			}
		}
		else
			return ERROR_FIREBREAK_NOT_FOUND;
	}

	XY_PolyType* poly = nullptr;
	try {
		poly = new XY_PolyType(xy_pairs);
		poly->m_publicFlags = XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_MULTIPOINT;
		poly->CleanPoly(0.0, XY_PolyType::PolygonType::MULTIPOINT);
		m_bRequiresSave = true;
	}
	catch (std::bad_alloc& cme) {
		if (poly)	delete poly;
		return E_OUTOFMEMORY;
	}

	delete pn->LN_Ptr();
	pn->LN_Ptr(poly);
	if (add) {
		weak_assert(!m_targets.GetCount());
		m_targets.AddHead(pn);
	}

	poly->RescanRanges();

	if (poly->Min_X() < m_xmin)	m_xmin = poly->Min_X();
	if (poly->Min_Y() < m_ymin)	m_ymin = poly->Min_Y();
	if (poly->Max_X() > m_xmax)	m_xmax = poly->Max_X();
	if (poly->Max_Y() > m_ymax)	m_ymax = poly->Max_Y();

	return S_OK;
}


HRESULT CCWFGM_Target::ClearPointSet(std::uint32_t set_break) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (set_break == (std::uint32_t)-1) {
		RefNode<XY_PolyType>* node;
		while ((node = m_targets.RemHead()) != NULL) {
			delete node->LN_Ptr();
			delete node;
		}
	} else {
		if (set_break >= m_targets.GetCount())					return ERROR_FIREBREAK_NOT_FOUND;

		RefNode<XY_PolyType>* pn = m_targets.IndexNode(set_break);
		m_targets.Remove(pn);
		delete pn->LN_Ptr();
		delete pn;
	}
	m_bRequiresSave = true;

	rescanRanges();
	return S_OK;
}


#ifndef DOXYGEN_IGNORE_CODE

void CCWFGM_Target::rescanRanges() {
	RefNode<XY_PolyType>* pn = m_targets.LH_Head();
	XY_Poly* poly;
	if (pn->LN_Succ()) {
		poly = pn->LN_Ptr();
		weak_assert(poly);

		poly->RescanRanges(TRUE);
		m_xmin = poly->Min_X();
		m_ymin = poly->Min_Y();
		m_xmax = poly->Max_X();
		m_ymax = poly->Max_Y();
		pn = pn->LN_Succ();
	}
	while (pn->LN_Succ()) {
		poly = pn->LN_Ptr();
		weak_assert(poly);

		poly->RescanRanges(TRUE);
		if (poly->Min_X() < m_xmin)	m_xmin = poly->Min_X();
		if (poly->Min_Y() < m_ymin)	m_ymin = poly->Min_Y();
		if (poly->Max_X() > m_xmax)	m_xmax = poly->Max_X();
		if (poly->Max_Y() > m_ymax)	m_ymax = poly->Max_Y();
		pn = pn->LN_Succ();
	}
}

#endif


HRESULT CCWFGM_Target::GetTargetRange(std::uint32_t index, XY_Point* min_pt, XY_Point* max_pt) {
	CRWThreadSemaphoreEngage engage(m_lock, FALSE);
	if ((!min_pt) || (!max_pt))			return E_POINTER;

	if (m_targets.GetCount() > 0) {
		if (index == (ULONG)-1) {
			min_pt->x = m_xmin;
			min_pt->y = m_ymin;
			max_pt->x = m_xmax;
			max_pt->y = m_ymax;
		} else {
			if ((index >= m_targets.GetCount())) {
				min_pt->x = min_pt->y = max_pt->x = max_pt->y = -1.0;
				return ERROR_FIREBREAK_NOT_FOUND;
			}
			XY_PolyType *poly = m_targets.IndexNode(index)->LN_Ptr();

    #ifdef DEBUG
			weak_assert(poly);
    #endif

			min_pt->x = poly->Min_X();
			min_pt->y = poly->Min_Y();
			max_pt->x = poly->Max_X();
			max_pt->y = poly->Max_Y();
		}
	} else {
		min_pt->x = min_pt->y = max_pt->x = max_pt->y = -1.0;
	}
	return S_OK;
}


HRESULT CCWFGM_Target::GetTargetCount(std::uint32_t *count) {
	CRWThreadSemaphoreEngage engage(m_lock, FALSE);
	if (!count)								return E_POINTER;

	*count = m_targets.GetCount();
	return S_OK;
}


HRESULT CCWFGM_Target::GetTargetSetCount(std::uint32_t index, std::uint32_t *count) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	if (!count)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, FALSE);
	if (index == (ULONG)-1) {
		*count = 0;
		RefNode<XY_PolyType>* node = m_targets.LH_Head();
		while (node->LN_Succ()) {
			if (*count < node->LN_Ptr()->NumPoints())
				*count = node->LN_Ptr()->NumPoints();
			node = node->LN_Succ();
		}
	}
	else if (index == (std::uint32_t)-2) {
		*count = 0;
		RefNode<XY_PolyType>* node = m_targets.LH_Head();
		while (node->LN_Succ()) {
			(*count) += node->LN_Ptr()->NumPoints();
			node = node->LN_Succ();
		}
	}
	else {
		if ((index >= m_targets.GetCount())) {
			*count = 0;
			return ERROR_FIREBREAK_NOT_FOUND;
		}
		XY_PolyType* poly = m_targets.IndexNode(index)->LN_Ptr();
		*count = poly->NumPoints();

	}
	return S_OK;
}


HRESULT CCWFGM_Target::GetTarget(std::uint32_t index, std::uint32_t sub_index, XY_Point *target) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	if (index >= m_targets.GetCount())					return ERROR_FIREBREAK_NOT_FOUND;
	XY_PolyType* poly = m_targets.IndexNode(index)->LN_Ptr();
	if (sub_index >= poly->NumPoints())					return ERROR_FIREBREAK_NOT_FOUND;
	target->x = poly->GetPoint(sub_index).x;
	target->y = poly->GetPoint(sub_index).y;
	return S_OK;
}


HRESULT CCWFGM_Target::GetTargetSet(std::uint32_t index, std::uint32_t* size, XY_Poly* xy_pairs) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	if ((!size) || (!xy_pairs))		return E_POINTER;

	XY_PolyType* poly = m_targets.IndexNode(index)->LN_Ptr();
	if (*size < poly->NumPoints()) {
		*size = poly->NumPoints();
		return E_OUTOFMEMORY;
	}
	*size = poly->NumPoints();
	xy_pairs->SetNumPoints(*size);
	for (std::uint32_t i = 0; i < *size; i++) {
		xy_pairs->SetPoint(i, poly->GetPoint(i));
	}

	return S_OK;
}


HRESULT CCWFGM_Target::get_GridEngine( boost::intrusive_ptr<ICWFGM_GridEngine>* pVal) {
	if (!pVal)								return E_POINTER;
	*pVal = m_gridEngine;
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	return S_OK;
}


HRESULT CCWFGM_Target::put_GridEngine( ICWFGM_GridEngine *newVal) {
	if (newVal) {
		boost::intrusive_ptr<ICWFGM_GridEngine> pGridEngine;
		pGridEngine = dynamic_cast<ICWFGM_GridEngine*>(const_cast<ICWFGM_GridEngine*>(newVal));
		if (pGridEngine.get()) {
			m_gridEngine = pGridEngine;
			return S_OK;
		}
		return E_FAIL;
	}

	m_gridEngine = newVal;
	return S_OK;
}


HRESULT CCWFGM_Target::MT_Lock( bool exclusive,  std::uint16_t obtain) {
	if (obtain == (USHORT)-1) {
		LONGLONG state = m_lock.CurrentState();
		if (!state)				return SUCCESS_STATE_OBJECT_UNLOCKED;
		if (state < 0)			return SUCCESS_STATE_OBJECT_LOCKED_WRITE;
		if (state >= 1000000LL)	return SUCCESS_STATE_OBJECT_LOCKED_SCENARIO;
		return SUCCESS_STATE_OBJECT_LOCKED_READ;
	} else if (obtain) {
		if (exclusive)	m_lock.Lock_Write();
		else			m_lock.Lock_Read(1000000LL);
	} else {
		if (exclusive)	m_lock.Unlock();
		else			m_lock.Unlock(1000000LL);
	}
	return S_OK;
}


HRESULT CCWFGM_Target::PreCalculationEvent( const HSS_Time::WTime& time,  std::uint32_t /*mode*/, CalculationEventParms* parms) {
	return S_OK;
}


HRESULT CCWFGM_Target::PostCalculationEvent( const HSS_Time::WTime& time,  std::uint32_t /*mode*/, CalculationEventParms* parms) {
	return S_OK;
}


HRESULT CCWFGM_Target::Clone(boost::intrusive_ptr<ICWFGM_CommonBase>* newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore*)&m_lock, SEM_FALSE);

	try {
		CCWFGM_Target* f = new CCWFGM_Target(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception& e) {
	}
	return E_FAIL;
}


#define MT_FB TRUE

HRESULT CCWFGM_Target::GetAttribute( std::uint16_t option,  PolymorphicAttribute* value) {
	if (!value)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, FALSE);

	switch (option) {
	case CWFGM_ATTRIBUTE_LOAD_WARNING: {
		*value = m_loadWarning;
		return S_OK;
	}

	case CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE:
		*value = (m_flags & CCWFGMGRID_ALLOW_GIS) ? true : false;
		return S_OK;

	case CWFGM_GRID_ATTRIBUTE_GIS_URL: {
		*value = m_gisURL;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_LAYER: {
		*value = m_gisLayer;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_UID: {
		*value = m_gisUID;
		return S_OK;
	}
	case CWFGM_GRID_ATTRIBUTE_GIS_PWD: {
		*value = m_gisPWD;
		return S_OK;
	}
	}
	return E_INVALIDARG;
}


/*! Polymorphic.  This routine sets an attribute/option value given the attribute/option index.  Currently does not
	perform any operation and is simply reserved for future functionality.
	\param option Reserved for future functionality.
	\param value Value for the attribute/option index
	\sa ICWFGM_Target::SetAttribute

	\retval E_NOTIMPL This function is reserved for future functionality.
*/
HRESULT CCWFGM_Target::SetAttribute( std::uint16_t option,  const PolymorphicAttribute& var) {
	std::uint32_t* cnt, old;
	bool bval;
	HRESULT hr;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine;
	if (!gridEngine) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	switch (option) {
	case CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE:
		try {
			bval = std::get<bool>(var);
		}
		catch (std::bad_variant_access&) {
			weak_assert(false);
			break;
		}
		old = m_flags;
		if (bval)
			m_flags |= CCWFGMGRID_ALLOW_GIS;
		else
			m_flags &= (~(CCWFGMGRID_ALLOW_GIS));
		if (old != m_flags)
			m_bRequiresSave = true;
		return S_OK;

	case CWFGM_GRID_ATTRIBUTE_GIS_URL: {
		std::string str;
		try {
			str = std::get<std::string>(var);
		}
		catch (std::bad_variant_access&) {
			weak_assert(false);
			break;
		}
		if (str.length()) {
			m_gisURL = str;
			m_bRequiresSave = true;
		}
	}
									 return S_OK;

	case CWFGM_GRID_ATTRIBUTE_GIS_LAYER: {
		std::string str;
		try {
			str = std::get<std::string>(var);
		}
		catch (std::bad_variant_access&) {
			weak_assert(false);
			break;
		}
		if (str.length()) {
			m_gisLayer = str;
			m_bRequiresSave = true;
		}
	}
									   return S_OK;

	case CWFGM_GRID_ATTRIBUTE_GIS_UID: {
		std::string str;
		try {
			str = std::get<std::string>(var);
		}
		catch (std::bad_variant_access&) {
			weak_assert(false);
			break;
		}
		m_gisUID = str;
		m_bRequiresSave = true;
	}
									 return S_OK;

	case CWFGM_GRID_ATTRIBUTE_GIS_PWD: {
		std::string str;
		try {
			str = std::get<std::string>(var);
		}
		catch (std::bad_variant_access&) {
			weak_assert(false);
			break;
		}
		m_gisPWD = str;
		m_bRequiresSave = true;
	}
									 return S_OK;
	}

	return E_INVALIDARG;
}


HRESULT CCWFGM_Target::GetPointSetAttributeCount(std::uint32_t* count) {
	if (!count)									return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, FALSE);

	*count = m_attributeNames.size();
	return S_OK;
}


HRESULT CCWFGM_Target::GetPointSetAttributeName( std::uint32_t count, std::string* attribute_name) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
		
	if (!attribute_name)									return E_POINTER;
	if (count >= (std::uint32_t)m_attributeNames.size())	return E_INVALIDARG;

	std::set<std::string>::iterator it = m_attributeNames.begin();
	std::advance(it, count);
	*attribute_name = *it;
	return S_OK;
}


HRESULT CCWFGM_Target::GetPointSetAttributeValue(std::uint32_t index, const std::string& attribute_name, PolymorphicAttribute* value) {
	if (!value)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, FALSE);
	std::string attributeName(attribute_name);

	if (index >= m_targets.GetCount())						return E_INVALIDARG;
	XY_PolyType* poly = m_targets.IndexNode(index)->LN_Ptr();

	for (auto a : poly->m_attributes)
	{
		if (!boost::iequals(a.attributeName, attributeName))
		{
			if (std::holds_alternative<std::int32_t>(a.attributeValue))
				*value = std::get<std::int32_t>(a.attributeValue);
			else if (std::holds_alternative<std::int64_t>(a.attributeValue))
				*value = std::get<std::int64_t>(a.attributeValue);
			else if (std::holds_alternative<double>(a.attributeValue))
				*value = std::get<double>(a.attributeValue);
			else if (std::holds_alternative<std::string>(a.attributeValue))
				*value = std::get<std::string>(a.attributeValue);
			else if (std::holds_alternative<std::monostate>(a.attributeValue))
				*value = std::get<std::monostate>(a.attributeValue);
			else
				return ERROR_UNKNOWN_PROPERTY | ERROR_SEVERITY_WARNING;
			return S_OK;
		}
	}
	return E_INVALIDARG;
}
