/**
 * WISE_Grid_Module: XYPoly.h
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

#ifndef SPECIAL_XYPOLY_H
#define SPECIAL_XYPOLY_H


#include "poly.h"

#ifdef _NO_MFC
class CXYPoly : public XY_Poly {
public:
	CXYPoly() : XY_Poly() { };
	CXYPoly(const double *x, const double *y, std::uint32_t array_size) : XY_Poly(x, y, array_size) { };
	CXYPoly(const double *xy_pairs, std::uint32_t array_size) : XY_Poly(xy_pairs, array_size) { };
	CXYPoly(std::uint32_t array_size) : XY_Poly(array_size) { };
};
#else
class CXYPoly : public CObject, public XY_Poly {
public:
	CXYPoly() : CObject(), XY_Poly() { };
	CXYPoly(const double *x, const double *y, std::uint32_t array_size) : CObject(), XY_Poly(x, y, array_size) { };
	CXYPoly(const double *xy_pairs, std::uint32_t array_size) : CObject(), XY_Poly(xy_pairs, array_size) { };
	CXYPoly(std::uint32_t array_size) : CObject(), XY_Poly(array_size) { };

	void *operator new(size_t s)		{ return CObject::operator new(s); }
	void operator delete(void *ptr)		{ return CObject::operator delete(ptr); }

protected:
	DECLARE_SERIAL(CXYPoly);
};
#endif

#endif
