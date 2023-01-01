/**
 * WISE_Grid_Module: CWFGM_internal.h
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

#define CCWFGMGRID_DEFAULT_ELEV_SET			0x00000001
#define CCWFGMGRID_ELEV_NODATA_EXISTS		0x00000008	// set if there's an elevation grid, AND it contains NODATA
#define CCWFGMGRID_SPECIFIED_FMC_ACTIVE		0x00000010
#define CCWFGMGRID_ALLOW_GIS				0x00000020	// set if we are allowed to load data from a GIS automatically, for existing FGM's this is left off
#define CCWFGMGRID_VALID					0x80000000	// replaces check on m_xsize == (std::uint16_t)-1
