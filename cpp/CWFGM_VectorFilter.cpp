/**
 * WISE_Grid_Module: CWFGM_VectorFilter.h
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
#include "CWFGM_VectorFilter.h"
#include "angles.h"
#include "points.h"

#include "CoordinateConverter.h"

#include <errno.h>
#include <stdio.h>

#include <boost/algorithm/string.hpp>

#include "XY_PolyType.h"


#include "XYPoly.h"


#ifndef DOXYGEN_IGNORE_CODE

ICWFGM_VectorEngine::ICWFGM_VectorEngine() {
}


ICWFGM_VectorEngine::~ICWFGM_VectorEngine() {
}


CCWFGM_VectorFilter::CCWFGM_VectorFilter() {
	m_bRequiresSave = false;
	m_xmin = m_ymin = m_xmax = m_ymax = -99.0;
	m_firebreakWidth = 1.0;
	m_resolution = -1.0;
	m_xllcorner = m_yllcorner = -999999999.0;
	m_flags = 0;
}


CCWFGM_VectorFilter::CCWFGM_VectorFilter(const CCWFGM_VectorFilter &toCopy) {
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	m_gisURL = toCopy.m_gisURL;
	m_gisLayer = toCopy.m_gisLayer;
	m_gisUID = toCopy.m_gisUID;
	m_gisPWD = toCopy.m_gisPWD;

	m_xmin = toCopy.m_xmin;
	m_ymin = toCopy.m_ymin;
	m_xmax = toCopy.m_xmax;
	m_ymax = toCopy.m_ymax;

	m_resolution = toCopy.m_resolution;
	m_xllcorner = toCopy.m_xllcorner;
	m_yllcorner = toCopy.m_yllcorner;
	m_flags = toCopy.m_flags;
	m_firebreakWidth = toCopy.m_firebreakWidth;

	RefNode<XY_PolyType> *pn = toCopy.m_polyList.LH_Head(), *npn;
	while (pn->LN_Succ()) {
		XY_PolyType *p = new XY_PolyType(*pn->LN_Ptr());
		npn = new RefNode<XY_PolyType>();
		npn->LN_Ptr(p);
		m_polyList.AddTail(npn);
		pn = pn->LN_Succ();
	}

	m_bRequiresSave = false;
}


CCWFGM_VectorFilter::~CCWFGM_VectorFilter() {
	weak_assert(!m_gridEngine);
	RefNode<XY_PolyType> *node;
	while ((node = m_polyList.RemHead()) != NULL) {
		delete node->LN_Ptr();
		delete node;
	}
}


HRESULT CCWFGM_VectorFilter::fixResolution() {
	HRESULT hr;
	double gridResolution, gridXLL, gridYLL;
	PolymorphicAttribute var;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine)) { weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	/*POLYMORPHIC CHECK*/
	try {
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) return hr; gridResolution = std::get<double>(var);
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_XLLCORNER, &var))) return hr; gridXLL = std::get<double>(var);
		if (FAILED(hr = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_YLLCORNER, &var))) return hr; gridYLL = std::get<double>(var);
	} catch (std::bad_variant_access &) 
	{ 
		weak_assert(false); 
		return ERROR_GRID_UNINITIALIZED; 
	}

	m_resolution = gridResolution;
	m_xllcorner = gridXLL;
	m_yllcorner = gridYLL;

	return S_OK;
}

#endif


HRESULT CCWFGM_VectorFilter::AddPolyLine(const XY_PolyConst &xy_pairs, std::uint16_t type, std::uint32_t *set_break) {
	if (!set_break)							return E_POINTER;

	std::uint32_t point_count = xy_pairs.NumPoints();
	if ((!point_count) || (point_count == 1))						return E_FAIL;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)										return ERROR_SCENARIO_SIMULATION_RUNNING;

	ULONG ctype;
	if (type == XY_PolyLL::Flags::INTERPRET_MULTIPOINT)		ctype = XY_PolyType::PolygonType::MULTIPOINT;
	else if (type == XY_PolyLL::Flags::INTERPRET_POLYLINE)	ctype = XY_PolyType::PolygonType::POLYLINE;
	else													ctype = XY_PolyType::PolygonType::POLYGON;

	RefNode<XY_PolyType> *pn = NULL;
	XY_PolyType *poly = NULL;
	try {
		pn = new RefNode<XY_PolyType>();
		poly = new XY_PolyType(xy_pairs);
		poly->m_publicFlags = type;
		poly->CleanPoly(0.0, ctype);
		m_bRequiresSave = true;
	} catch (std::bad_alloc &cme) {
		if (poly)	delete poly;
		if (pn)		delete pn;
		return E_OUTOFMEMORY;
	}

	*set_break = m_polyList.GetCount();
	pn->LN_Ptr(poly);
	m_polyList.AddTail(pn);

	poly->RescanRanges();

	if (!(*set_break)) {
		m_xmin = poly->Min_X();
		m_ymin = poly->Min_Y();
		m_xmax = poly->Max_X();
		m_ymax = poly->Max_Y();
	} else {
		if (poly->Min_X() < m_xmin)	m_xmin = poly->Min_X();
		if (poly->Min_Y() < m_ymin)	m_ymin = poly->Min_Y();
		if (poly->Max_X() > m_xmax)	m_xmax = poly->Max_X();
		if (poly->Max_Y() > m_ymax)	m_ymax = poly->Max_Y();
	}
	clearFirebreak();
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::SetPolyLine(std::uint32_t index, const XY_PolyConst &xy_pairs, std::uint16_t type) {
	std::uint32_t point_count = xy_pairs.NumPoints();
	if ((!point_count) || (point_count == 1))						return E_FAIL;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)										return ERROR_SCENARIO_SIMULATION_RUNNING;

	bool add = false;
	RefNode<XY_PolyType> *pn = m_polyList.IndexNode(index);
	if (!pn) {
		if (index == 0) {
			try {
				pn = new RefNode<XY_PolyType>();
				add = true;
			} catch(std::bad_alloc &cme) {
				return E_OUTOFMEMORY;
			}
		} else
			return ERROR_FIREBREAK_NOT_FOUND;
	}

	ULONG ctype;
	if (type == XY_PolyLL::Flags::INTERPRET_MULTIPOINT)		ctype = XY_PolyType::PolygonType::MULTIPOINT;
	else if (type == XY_PolyLL::Flags::INTERPRET_POLYLINE)	ctype = XY_PolyType::PolygonType::POLYLINE;
	else													ctype = XY_PolyType::PolygonType::POLYGON;

	XY_PolyType *poly = NULL;
	try {
		poly = new XY_PolyType(xy_pairs);
		poly->m_publicFlags = type;
		poly->CleanPoly(0.0, ctype);
		m_bRequiresSave = true;
	} catch(std::bad_alloc &cme) {
		if (poly)	delete poly;
		return E_OUTOFMEMORY;
	}

	delete pn->LN_Ptr();
	pn->LN_Ptr(poly);
	if (add) {
		weak_assert(!m_polyList.GetCount());
		m_polyList.AddHead(pn);
	}

	poly->RescanRanges();

	if (poly->Min_X() < m_xmin)	m_xmin = poly->Min_X();
	if (poly->Min_Y() < m_ymin)	m_ymin = poly->Min_Y();
	if (poly->Max_X() > m_xmax)	m_xmax = poly->Max_X();
	if (poly->Max_Y() > m_ymax)	m_ymax = poly->Max_Y();

	clearFirebreak();
	return S_OK;
}


#ifndef DOXYGEN_IGNORE_CODE

void CCWFGM_VectorFilter::rescanRanges() {
	RefNode<XY_PolyType> *pn = m_polyList.LH_Head();
	XY_Poly *poly;
	if (pn->LN_Succ()) {
		poly = pn->LN_Ptr();
		weak_assert(poly);

		poly->RescanRanges(true);
		m_xmin = poly->Min_X();
		m_ymin = poly->Min_Y();
		m_xmax = poly->Max_X();
		m_ymax = poly->Max_Y();
		pn = pn->LN_Succ();
	}
	while (pn->LN_Succ()) {
		poly = pn->LN_Ptr();
		weak_assert(poly);

		poly->RescanRanges(true);
		if (poly->Min_X() < m_xmin)	m_xmin = poly->Min_X();
		if (poly->Min_Y() < m_ymin)	m_ymin = poly->Min_Y();
		if (poly->Max_X() > m_xmax)	m_xmax = poly->Max_X();
		if (poly->Max_Y() > m_ymax)	m_ymax = poly->Max_Y();
		pn = pn->LN_Succ();
	}
}

#endif


HRESULT CCWFGM_VectorFilter::ClearPolyLine(std::uint32_t set_break) {
	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	if (set_break == (std::uint32_t)-1) {
		RefNode<XY_PolyType> *node;
		while ((node = m_polyList.RemHead()) != NULL) {
			delete node->LN_Ptr();
			delete node;
		}
	} else {
		if (set_break >= m_polyList.GetCount())					return ERROR_FIREBREAK_NOT_FOUND;

		RefNode<XY_PolyType> *pn = m_polyList.IndexNode(set_break);
		m_polyList.Remove(pn);
		delete pn->LN_Ptr();
		delete pn;
	}
	m_bRequiresSave = true;

	rescanRanges();
	clearFirebreak();
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::GetPolyLineRange(std::uint32_t index, XY_Point* min_pt, XY_Point* max_pt) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	return getPolyRange(index, &min_pt->x, &min_pt->y, &max_pt->x, &max_pt->y);
}


HRESULT CCWFGM_VectorFilter::GetFireBreakRange(std::uint32_t index, const HSS_Time::WTime &time, XY_Point *min_pt,  XY_Point *max_pt) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	buildFirebreak();
	return getBreakRange(index, time, &min_pt->x, &min_pt->y, &max_pt->x, &max_pt->y);
}


#ifndef DOXYGEN_IGNORE_CODE

HRESULT CCWFGM_VectorFilter::getPolyRange(std::uint32_t index, double *x_min, double *y_min, double *x_max, double *y_max) const {
	if ((!x_min) || (!y_min) || (!x_max) || (!y_max))			return E_POINTER;

	if (m_polyList.GetCount() > 0) {
		if (index == (std::uint32_t)-1) {
			*x_min = m_xmin;
			*y_min = m_ymin;
			*x_max = m_xmax;
			*y_max = m_ymax;
		} else {
			if ((index >= m_polyList.GetCount())) {
				*x_min = *y_min = *x_max = *y_max = -1.0;
				return ERROR_FIREBREAK_NOT_FOUND;
			}
			XY_PolyType *poly = m_polyList.IndexNode(index)->LN_Ptr();

    #ifdef DEBUG
			weak_assert(poly);
    #endif
			*x_min = poly->Min_X();
			*y_min = poly->Min_Y();
			*x_max = poly->Max_X();
			*y_max = poly->Max_Y();
		}
	} else {
		*x_min = *y_min = *x_max = *y_max = -1.0;
	}

	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getBreakRange(std::uint32_t index, const HSS_Time::WTime &time, double *x_min, double *y_min, double *x_max, double *y_max) {
	if ((!x_min) || (!y_min) || (!x_max) || (!y_max))			return E_POINTER;

	if (time.GetTotalMicroSeconds()) {						// we aren't supporting dynamic fire breaks
		*x_min = *y_min = *x_max = *y_max = -1.0;
		return ERROR_FIREBREAK_NOT_FOUND;
	}
	bool first = true;
	if (index == (std::uint32_t)-1) {
		XY_Rectangle bbox, bbox1;
		std::uint32_t i, cnt = m_firebreaks.size();
		for (i = 0; i < cnt; i++) {
			if (m_firebreaks[i].BoundingBox(bbox1)) {
				if (first) {
					bbox = bbox1;
					first = false;
				} else
					bbox.EncompassRectangle(bbox1);
			}
		}
		if (!first) {
			*x_min = bbox.m_min.x;
			*y_min = bbox.m_min.y;
			*x_max = bbox.m_max.x;
			*y_max = bbox.m_max.y;
		}
	} else {
		XY_PolySet &m_firebreak = m_firebreaks[index];
		if ((index >= m_firebreak.NumPolys())) {
			*x_min = *y_min = *x_max = *y_max = -1.0;
			return ERROR_FIREBREAK_NOT_FOUND;
		}

		XY_Rectangle bbox;
		if (m_firebreak.BoundingBox(bbox)) {
			*x_min = bbox.m_min.x;
			*y_min = bbox.m_min.y;
			*x_max = bbox.m_max.x;
			*y_max = bbox.m_max.y;
			first = false;
		}
	}
	return first ? ERROR_NO_DATA | ERROR_SEVERITY_WARNING : S_OK;
}


HRESULT CCWFGM_VectorFilter::getPolyMaxSize(std::uint32_t *size) const {
	if (!size)								return E_POINTER;

	*size = 0;
	RefNode<XY_PolyType> *pn = m_polyList.LH_Head();
	while (pn->LN_Succ()) {
		XY_PolyType *poly = pn->LN_Ptr();
		if (poly->NumPoints() > *size)
			*size = poly->NumPoints();
		pn = pn->LN_Succ();
	}
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getBreakMaxSize(const HSS_Time::WTime &time, std::uint32_t *size) const {
	if (!size)								return E_POINTER;

	*size = 0;
	if (!time.GetTotalMicroSeconds()) {
		std::uint32_t i, j, sz, cnt = m_firebreaks.size(), cnt2;
		for (i = 0; i < cnt; i++) {
			cnt2 = m_firebreaks[i].NumPolys();
			for (j = 0; j < cnt2; j++) {
				sz = m_firebreaks[i].GetPolyConst(j).NumPoints();
				if (sz > *size)
					*size = sz;
			}
		}
	}
	return S_OK;
}

#endif


HRESULT CCWFGM_VectorFilter::GetPolyLineCount(std::uint32_t *count) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	return getPolyCount((std::uint32_t *)count);
}


HRESULT CCWFGM_VectorFilter::GetFireBreakCount(const HSS_Time::WTime &time, std::uint32_t *count) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	buildFirebreak();
	return getBreakCount(time, count);
}


HRESULT CCWFGM_VectorFilter::GetFireBreakSetCount(const HSS_Time::WTime &time, std::uint32_t index, std::uint32_t *count) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	buildFirebreak();
	return getBreakSetCount(time, index, count);
}


#ifndef DOXYGEN_IGNORE_CODE

HRESULT CCWFGM_VectorFilter::getPolyCount(std::uint32_t *count) const {
	if (!count)								return E_POINTER;

	*count = m_polyList.GetCount();

	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getBreakCount(const HSS_Time::WTime &time, std::uint32_t *count) const {
	if (!count)								return E_POINTER;

	if (time.GetTotalMicroSeconds())	*count = 0;							// nothing dynamic
	else								*count = m_firebreaks.size();		// all static
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getBreakSetCount(const HSS_Time::WTime &time, std::uint32_t index, std::uint32_t *count) const {
	if (!count)								return E_POINTER;

	if (time.GetTotalMicroSeconds())
		*count = 0;							// nothing dynamic
	else {
		if (index >= (std::uint32_t)m_firebreaks.size()) {
			*count = 0;
			return ERROR_FIREBREAK_NOT_FOUND;
		}
		*count = m_firebreaks[index].NumPolys();		// all static
	}
	return S_OK;
}

#endif


HRESULT CCWFGM_VectorFilter::GetPolyLineSize(std::uint32_t index, std::uint32_t *size) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	if (index == (std::uint32_t)-1)
		return getPolyMaxSize(size);
	return getPolySize(index, size);
}


HRESULT CCWFGM_VectorFilter::GetFireBreakSize(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime &time, std::uint32_t *size) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	buildFirebreak();
	if (index == (std::uint32_t)-1)
		return getBreakMaxSize(time, size);
	return getBreakSize(index, sub_index, time, size);
}


#ifndef DOXYGEN_IGNORE_CODE

HRESULT CCWFGM_VectorFilter::getPolySize(std::uint32_t index, std::uint32_t *size) const {
	if (!size)								return E_POINTER;

	if ((index >= m_polyList.GetCount()) && (index != (std::uint32_t)-2)) {
		*size = 0;
		return ERROR_FIREBREAK_NOT_FOUND;
	}

	if (index == (std::uint32_t)-2) {
		*size = 0;
		RefNode<XY_PolyType> *pn = m_polyList.LH_Head();
		while (pn->LN_Succ()) {
			XY_PolyType *poly = pn->LN_Ptr();
			*size += poly->NumPoints();
			pn = pn->LN_Succ();
		}
		return S_OK;
	}
	XY_PolyType *poly = m_polyList.IndexNode(index)->LN_Ptr();
	*size = poly->NumPoints();
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getType(std::uint32_t index, std::uint16_t* type) const
{
	if (!type)								return E_POINTER;

	if (index >= m_polyList.GetCount())
	{
		*type = 0;
		return ERROR_FIREBREAK_NOT_FOUND;
	}
	XY_PolyType *poly = m_polyList.IndexNode(index)->LN_Ptr();
	*type = poly->m_publicFlags & XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYMASK;
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getBreakSize(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime &time, std::uint32_t *size) const {
	if (!size)								return E_POINTER;

	if ((time.GetTotalMicroSeconds()) ||
	    ((index >= (std::uint32_t)m_firebreaks.size()) && (index != (std::uint32_t)-2)) ||
	    ((sub_index >= m_firebreaks[index].NumPolys()) && (index != (std::uint32_t)-2))) {
		*size = 0;
		return ERROR_FIREBREAK_NOT_FOUND;
	}
	if ((index == (std::uint32_t)-2) && (sub_index == (std::uint32_t)-2)) {
		*size = 0;
		if (!time.GetTotalMicroSeconds()) {
			std::uint32_t i, j, sz, cnt = (std::uint32_t)m_firebreaks.size(), cnt2;
			for (i = 0; i < cnt; i++) {
				cnt2 = m_firebreaks[i].NumPolys();
				for (j = 0; j < cnt2; j++) {
					sz = m_firebreaks[i].GetPolyConst(j).NumPoints();
					*size += sz;
				}
			}
		}
		return S_OK;
	} else if (sub_index == (std::uint32_t)-2) {
		*size = 0;
		if (!time.GetTotalMicroSeconds()) {
			std::uint32_t j, sz, cnt2 = m_firebreaks[index].NumPolys();
			for (j = 0; j < cnt2; j++) {
				sz = m_firebreaks[index].GetPolyConst(j).NumPoints();
				*size += sz;
			}
		}
		return S_OK;
	}
	const XY_PolyConst &poly = m_firebreaks[index].GetPolyConst(sub_index);
	*size = poly.NumPoints();
	return S_OK;
}

#endif


HRESULT CCWFGM_VectorFilter::GetPolyType(std::uint32_t index, std::uint16_t* type)
{
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	return getType(index, type);
}


HRESULT CCWFGM_VectorFilter::GetPolyLine(std::uint32_t index, std::uint32_t *size, XY_Poly *xy_pairs, std::uint16_t *type) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	return getPoly(index, size, xy_pairs, type);
}


HRESULT CCWFGM_VectorFilter::GetFireBreak(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime &time, std::uint32_t *size, XY_Poly *xy_pairs) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	buildFirebreak();
	return getBreak(index, sub_index, time, size, xy_pairs);
}


#ifndef DOXYGEN_IGNORE_CODE

HRESULT CCWFGM_VectorFilter::getPoly(std::uint32_t index, std::uint32_t *size, XY_Poly *xy_pairs, std::uint16_t *type) const {
	if ((!size) || (!xy_pairs) || (!type))		return E_POINTER;

	if (index >= m_polyList.GetCount()) {
		*size = 0;
		return ERROR_FIREBREAK_NOT_FOUND;
	}
	XY_PolyType *poly = m_polyList.IndexNode(index)->LN_Ptr();
	if (*size < poly->NumPoints()) {
		*size = poly->NumPoints();
		return E_OUTOFMEMORY;
	}
	*size = poly->NumPoints();
	*type = poly->m_publicFlags & XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYMASK;

	XY_Point p;
	XY_PolyNode *n;
	xy_pairs->SetNumPoints(*size);

	for (std::uint32_t i = 0; i < *size; i++) {
		xy_pairs->SetPoint(i, poly->GetPoint(i));
	}

	return S_OK;
}


HRESULT CCWFGM_VectorFilter::getBreak(std::uint32_t index, std::uint32_t sub_index, const HSS_Time::WTime &time, std::uint32_t *size, XY_Poly *xy_pairs) const {
	if ((!size) || (!xy_pairs))				return E_POINTER;

	if ((time.GetTotalMicroSeconds()) || (index >= (std::uint32_t)m_firebreaks.size()) || (sub_index >= m_firebreaks[index].NumPolys())) {
		*size = 0;
		return ERROR_FIREBREAK_NOT_FOUND;
	}

	const XY_PolyConst &poly = m_firebreaks[index].GetPolyConst(sub_index);

	*size = poly.NumPoints();

	XY_Point p;
	XY_PolyNode *n;
	xy_pairs->SetNumPoints(*size);

	for (std::uint32_t i = 0; i < *size; i++) {
		xy_pairs->SetPoint(i, poly.GetPoint(i));
	}

	return S_OK;
}

#endif


HRESULT CCWFGM_VectorFilter::get_GridEngine( boost::intrusive_ptr<ICWFGM_GridEngine> *pVal) {
	if (!pVal)								return E_POINTER;
	*pVal = m_gridEngine;
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::put_GridEngine(ICWFGM_GridEngine *newVal) {
	if (newVal) {
		boost::intrusive_ptr<ICWFGM_GridEngine> pGridEngine;
		pGridEngine = dynamic_cast<ICWFGM_GridEngine *>(const_cast<ICWFGM_GridEngine *>(newVal));
		if (pGridEngine.get()) {
			m_gridEngine = pGridEngine;
			fixResolution();
			return S_OK;
		}
		return E_FAIL;
	}

	m_gridEngine = newVal;
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::put_CommonData(ICWFGM_CommonData* pVal) {
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::MT_Lock(bool exclusive, std::uint16_t obtain) {
	if (obtain == (std::uint16_t)-1) {
		std::int64_t state = m_lock.CurrentState();
		if (!state)				return SUCCESS_STATE_OBJECT_UNLOCKED;
		if (state < 0)			return SUCCESS_STATE_OBJECT_LOCKED_WRITE;
		if (state >= 1000000LL)	return SUCCESS_STATE_OBJECT_LOCKED_SCENARIO;
		return						   SUCCESS_STATE_OBJECT_LOCKED_READ;
	} else if (obtain) {
		if (exclusive)	m_lock.Lock_Write();
		else			m_lock.Lock_Read(1000000LL);
	} else {
		if (exclusive)	m_lock.Unlock();
		else			m_lock.Unlock(1000000LL);
	}
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::Valid(const HSS_Time::WTime & /*start_time*/, const HSS_Time::WTimeSpan & /*duration*/) {
	if (!m_gridEngine)							{ weak_assert(false); return ERROR_VECTOR_UNINITIALIZED; }
	return S_OK;
}						


HRESULT CCWFGM_VectorFilter::GetEventTime(std::uint32_t flags, const HSS_Time::WTime & from_time,  HSS_Time::WTime *next_event) {
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::PreCalculationEvent(const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) {
	weak_assert(m_resolution != -1.0);

	return S_OK;
}


HRESULT CCWFGM_VectorFilter::PostCalculationEvent(const HSS_Time::WTime &time, std::uint32_t mode, CalculationEventParms *parms) {
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!newObject)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	try {
		CCWFGM_VectorFilter *f = new CCWFGM_VectorFilter(*this);
		*newObject = f;
		return S_OK;
	}
	catch (std::exception &e) {
	}
	return E_FAIL;
}


HRESULT CCWFGM_VectorFilter::get_Width(double *pVal) {
	if (!pVal)						return E_POINTER;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	*pVal = m_firebreakWidth;
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::put_Width(double newVal) {
	if (newVal < 0.0)					return E_INVALIDARG;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	m_firebreakWidth = newVal;
	m_bRequiresSave = true;
	m_firebreaks.clear();
	return S_OK;
}


//#define COMBINE_ALL_SETS

#define MT_FB true

#ifndef DOXYGEN_IGNORE_CODE

void CCWFGM_VectorFilter::buildFirebreak() {
	CRWThreadSemaphoreEngage engage(m_calcLock, SEM_TRUE);

	if (m_firebreaks.size())
		return;

	weak_assert(m_gridEngine);
	std::uint16_t xdim, ydim;
	RefNode<XY_PolyType> *node = m_polyList.LH_Head();
	while (node->LN_Succ()) {
		if (((m_firebreakWidth > 0.0) && (node->LN_Ptr()->m_publicFlags & XY_PolyLL_BaseTempl<double>::Flags::INTERPRET_POLYLINE)) || (node->LN_Ptr()->NumPoints() < 3)) {
			XY_PolyLLSet firebreak;
			XY_PolySet set;
			set.AddPoly(*node->LN_Ptr(), false);
			XY_PolyLLSet firebreak_sub;

			firebreak_sub.SetCacheScale(m_resolution);
				firebreak_sub.BufferPolygon(&set, m_firebreakWidth, DEGREE_TO_RADIAN(45.0), nullptr, MT_FB);

			if (firebreak_sub.NumPolys()) {
				XY_PolySet m_firebreak;
				m_firebreak = firebreak_sub;

    #ifdef DEBUG
				weak_assert(m_firebreak.NumPolys() == firebreak_sub.NumPolys());
				XY_PolyLL *pl = firebreak_sub.LH_Head();
				for (std::uint32_t i = 0; i < m_firebreak.NumPolys(); i++) {
					XY_PolyConst &p = m_firebreak.GetPoly(i);
					weak_assert(p.NumPoints() == pl->NumPoints());
					pl = pl->LN_Succ();
				}
    #endif

				m_firebreaks.push_back(m_firebreak);
			}
		} else {
			XY_PolyLLSet firebreak;
			XY_PolyLL *poly = new XY_PolyLL(*node->LN_Ptr());
			poly->SetCacheScale(m_resolution);
			poly->m_publicFlags |= node->LN_Ptr()->m_publicFlags;
			poly->CleanPoly(0.0, XY_PolyLL::Flags::INTERPRET_POLYGON);
			firebreak.AddPoly(poly);

			firebreak.Unwind(true, MT_FB, nullptr, nullptr);
			if (firebreak.NumPolys()) {
				XY_PolySet m_firebreak;
				m_firebreak = firebreak;
				m_firebreaks.push_back(m_firebreak);
			}
		}
		node = node->LN_Succ();
	}
}

#endif


HRESULT CCWFGM_VectorFilter::GetAttribute(std::uint16_t option, PolymorphicAttribute *value) {
	if (!value)						return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

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
	\sa ICWFGM_VectorFilter::SetAttribute

	\retval E_NOTIMPL This function is reserved for future functionality.
*/
HRESULT CCWFGM_VectorFilter::SetAttribute(std::uint16_t option, const PolymorphicAttribute &var) {
	std::uint32_t *cnt, old;
	bool bval;
	HRESULT hr;
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine = m_gridEngine;
	if (!gridEngine)							{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	switch (option) {
		case CWFGM_GRID_ATTRIBUTE_GIS_CANRESIZE:
								try {
									bval = std::get<bool>(var);
								}
								catch (std::bad_variant_access &) {
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
								catch (std::bad_variant_access &) {
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
								catch (std::bad_variant_access &) {
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
								catch (std::bad_variant_access &) {
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
								catch (std::bad_variant_access &) {
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


HRESULT CCWFGM_VectorFilter::GetPolyLineAttributeCount(std::uint32_t* count)
{
	if (!count)									return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	*count = m_attributeNames.size();
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::GetPolyLineAttributeName(std::uint32_t count, std::string* attribute_name)
{
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	if (!attribute_name)									return E_POINTER;
	if (count >= (std::uint32_t)m_attributeNames.size())	return E_INVALIDARG;

	std::set<std::string>::iterator it = m_attributeNames.begin();
	std::advance(it, count);
	*attribute_name = *it;
	return S_OK;
}


HRESULT CCWFGM_VectorFilter::GetPolyLineAttributeValue(std::uint32_t index, const std::string& attribute_name, PolymorphicAttribute* value)
{
	if (!value)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);
	std::string attributeName(attribute_name);

	if (index >= m_polyList.GetCount())						return E_INVALIDARG;
	XY_PolyType* poly = m_polyList.IndexNode(index)->LN_Ptr();

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
