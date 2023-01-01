/**
 * WISE_Grid_Module: ICWFGM_Weather.h
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

#include <cstdint>


class IWXData {
public:
	double Temperature;
	double DewPointTemperature;
	double RH;
	double Precipitation;
	double WindSpeed;
	double WindGust;
	double WindDirection;
	std::uint32_t SpecifiedBits;
};

class IFWIData {
public:
	double FFMC;
	double ISI;
	double FWI;
	std::uint32_t SpecifiedBits;
};

class DFWIData {
public:
	double dFFMC;
	double dISI;
	double dFWI;
	double dDMC;
	double dDC;
	double dBUI;
	std::uint32_t SpecifiedBits;
};
