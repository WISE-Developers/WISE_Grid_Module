/**
 * WISE_Grid_Module: CWFGM_FuelMap.cpp
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

#include "ICWFGM_Fuel.h"
#include "GridCom_ext.h"
#include "CWFGM_FuelMap.h"



#ifndef DOXYGEN_IGNORE_CODE

CCWFGM_FuelMap::CCWFGM_FuelMap() : m_cache(16) {
	m_bRequiresSave = false;
	for (std::uint16_t i = 0; i < 256; i++) {
		m_fileIndex[i] = -1;
		m_exportFileIndex[i]=-1;
		m_fuel[i] = nullptr;
	}
}


CCWFGM_FuelMap::CCWFGM_FuelMap(const CCWFGM_FuelMap &toCopy) : m_cache(16) {
	CRWThreadSemaphoreEngage engage2(*(CRWThreadSemaphore *)&toCopy.m_lock, SEM_FALSE);

	boost::intrusive_ptr<const CCWFGM_FuelMap> fuelMapPtr(dynamic_cast<const CCWFGM_FuelMap *>(&toCopy));
	if (fuelMapPtr) {
		ICWFGM_Fuel *fuel;
		for (std::uint8_t i = 0; i < 255; i++) {
			fuelMapPtr->FuelAtIndex(i, &m_fileIndex[i], &m_exportFileIndex[i], &fuel);
			m_fuel[i].reset(fuel);
		}
		m_cache.Clear();
		m_bRequiresSave = false;
	}
}


CCWFGM_FuelMap::~CCWFGM_FuelMap() {
}

#endif


HRESULT CCWFGM_FuelMap::MT_Lock(bool exclusive, std::uint16_t obtain) {
	if (obtain == (std::uint16_t)-1) {
		std::int64_t state = m_lock.CurrentState();
		if (!state)				return SUCCESS_STATE_OBJECT_UNLOCKED;
		if (state < 0)			return SUCCESS_STATE_OBJECT_LOCKED_WRITE;
		if (state >= 1000000LL)	return SUCCESS_STATE_OBJECT_LOCKED_SCENARIO;
		return						   SUCCESS_STATE_OBJECT_LOCKED_READ;
	}
	else if (obtain) {
		if (exclusive)	m_lock.Lock_Write();
		else			m_lock.Lock_Read(1000000LL);

		for (std::uint16_t i = 0; i < 256; i++)
			if (m_fuel[i])
				m_fuel[i]->MT_Lock(exclusive, obtain);
	}
	else {
		for (std::uint16_t i = 0; i < 256; i++)
			if (m_fuel[i])
				m_fuel[i]->MT_Lock(exclusive, obtain);

		if (exclusive)	m_lock.Unlock();
		else		m_lock.Unlock(1000000LL);
	}
	return S_OK;
}


HRESULT CCWFGM_FuelMap::Clone(boost::intrusive_ptr<ICWFGM_CommonBase> *newObject) const {
	if (!(newObject))							return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);
	HRESULT hr = 0;
	try {
		CCWFGM_FuelMap *f = new CCWFGM_FuelMap();
		for (std::uint8_t i = 0; i < 255; i++) {
			if (!m_fuel[i])
				continue;
			bool found = false;
			for (std::uint8_t j = 0; j < i; j++)
				if ((m_fuel[j]) && (m_fuel[i] == m_fuel[j])) {
					found = true;
					f->m_fuel[i] = f->m_fuel[j];
					f->m_fileIndex[i] = m_fileIndex[i];
					f->m_exportFileIndex[i] = m_exportFileIndex[i];
					break;
				}
			if (!found) {
				boost::intrusive_ptr<ICWFGM_Fuel> fu;
				m_fuel[i]->Mutate(&fu);
				f->m_fuel[i] = fu;
				f->m_fileIndex[i] = m_fileIndex[i];
				f->m_exportFileIndex[i] = m_exportFileIndex[i];
			}
		}
		f->m_cache.Clear();
		f->m_bRequiresSave = true;
		*newObject = f;
	}
	catch (std::exception &e) {
		hr = E_FAIL;
	}
	return hr;
}


HRESULT CCWFGM_FuelMap::GetFuelCount(std::uint8_t *count, std::uint8_t *unique_count) const {
	if ((!count) || (!unique_count))					return E_POINTER;
	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	std::uint8_t cnt = 0, unique = 0;
	for (std::uint8_t i = 0; i < 255; i++)
		if (m_fuel[i]) {
			cnt++;
			unique++;
			for (std::uint16_t j = 0; j < i; j++)
				if (m_fuel[j])
					if (m_fuel[i] == m_fuel[j]) {
						unique--;
						break;
					}
		}
	*count = cnt;
	*unique_count = unique;
	return S_OK;
}


HRESULT CCWFGM_FuelMap::AddFuel(ICWFGM_Fuel *fuel, long file_index, long export_file_index, std::uint8_t *fuel_index) {
									// the fuel can exist many times in this list with different indicies
	if ((!fuel) || (!fuel_index))						return E_POINTER;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	ICWFGM_Fuel *fuelPtr = dynamic_cast<ICWFGM_Fuel *>(fuel);
	HRESULT retval = (fuelPtr) ? S_OK : E_NOINTERFACE;
	if (SUCCEEDED(retval)) {
		std::uint8_t i;
		for (i = 0; i < 255; i++)				// first, check to see if it's already in the list
			if (m_fuel[i]) {
				if (m_fuel[i].get() == fuel) {
					retval = SUCCESS_FUELS_FUEL_ALREADY_ADDED;
					if ((m_fileIndex[i] == file_index) && (m_exportFileIndex[i] == export_file_index)) {
						*fuel_index = i;	// test if duplicate entry
						return retval;
					}
				}
				if (m_fileIndex[i] != -1)		// -1 is "not defined file index"
					if (m_fileIndex[i] == file_index)
						return ERROR_FUELS_GRIDVALUE_ALREADY_ASSIGNED;
									// this error code will also occur if the same fuel is attempted to be
									// added with the same input index but with a different export index
			}
		for (i = 0; i < 255; i++)
			if (!m_fuel[i]) {
				m_fuel[i].reset(fuel);
				m_fileIndex[i] = file_index;
				m_exportFileIndex[i] = export_file_index;
				*fuel_index = /*(UBYTE)*/i;
				m_cache.Clear();
				m_bRequiresSave = true;
				return retval;
			}
		retval = ERROR_NOT_ENOUGH_MEMORY | ERROR_SEVERITY_WARNING;			// no room left for it
	}

	return retval;
}


HRESULT CCWFGM_FuelMap::RemoveFuel(ICWFGM_Fuel *fuel) {
	if (!fuel)								return E_POINTER;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	HRESULT result = ERROR_FUELS_FUEL_UNKNOWN;
	for (std::uint8_t i = 0; i < 255; i++)
		if (m_fuel[i])
			if (m_fuel[i].get() == fuel) {
				m_fuel[i] = NULL;
				m_fileIndex[i] = -1;
				m_exportFileIndex[i] = -1;
				m_cache.Clear();
				m_bRequiresSave = true;
				result = S_OK;
			}

	return result;
}


HRESULT CCWFGM_FuelMap::RemoveFuelIndex(ICWFGM_Fuel *fuel, long file_index) {
	if (!fuel)								return E_POINTER;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)								return ERROR_SCENARIO_SIMULATION_RUNNING;

	for (std::uint8_t i = 0; i < 255; i++)
		if (m_fuel[i])
			if ((m_fuel[i].get() == fuel) && (file_index == m_fileIndex[i])) {
				m_fuel[i] = NULL;
				m_fileIndex[i] = -1;
				m_exportFileIndex[i] = -1;
				m_cache.Clear();
				m_bRequiresSave = true;
				return S_OK;
			}

	return ERROR_FUELS_FUEL_UNKNOWN;
}


HRESULT CCWFGM_FuelMap::FuelAtIndex(std::uint8_t fuel_index, long *file_index, long *export_file_index, ICWFGM_Fuel **fuel) const {
	if ((!file_index) || (!export_file_index) || (!fuel))			return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	*file_index = m_fileIndex[fuel_index];
	*export_file_index = m_exportFileIndex[fuel_index];
	*fuel = m_fuel[fuel_index].get();
	if (m_fuel[fuel_index])
		return S_OK;
	return ERROR_FUELS_FUEL_UNKNOWN;
}


HRESULT CCWFGM_FuelMap::FuelAtFileIndex(long file_index, std::uint8_t *fuel_index, long *export_file_index, ICWFGM_Fuel **fuel) const {
	if ((!fuel_index) || (!export_file_index) || (!fuel))			return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	for (std::uint8_t i = 0; i < 255; i++)
		if (m_fuel[i])
			if (file_index == m_fileIndex[i]) {
				*fuel = m_fuel[i].get();
				*fuel_index = i;
				*export_file_index = m_exportFileIndex[i];
				return S_OK;
			}
	return ERROR_FUELS_FUEL_UNKNOWN;
}


HRESULT CCWFGM_FuelMap::IndexOfFuel(ICWFGM_Fuel *fuel, long *file_index, long *export_file_index, std::uint8_t *fuel_index) const {
	if ((!file_index) || (!export_file_index) || (!fuel) || (!fuel_index))	return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	fuel_cache_key c_key;
	c_key.fuel_pointer = fuel;
	c_key.fuel_index = *fuel_index;
	fuel_cache_result *c_result;
	fuel_cache_result result;
			
	if (c_result = ((ValueCacheTempl_MT<fuel_cache_key, fuel_cache_result> *)&m_cache)->Retrieve(&c_key, &result)) {
		*file_index = c_result->file_index;
		*export_file_index = c_result->export_index;
		*fuel_index = c_result->fuel_index;
		return S_OK;
	}
	else {
		c_result = &result;
	}
	for (std::uint8_t i = (*fuel_index + 1) & 0xff; i < 255; i++) {
		if (m_fuel[i])
			if (m_fuel[i].get() == fuel) {
				*file_index = m_fileIndex[i];
				*fuel_index = i;
				*export_file_index = m_exportFileIndex[i];
				
				c_result->file_index = *file_index;
				c_result->export_index = *export_file_index;
				c_result->fuel_index = *fuel_index;
				((ValueCacheTempl_MT<fuel_cache_key, fuel_cache_result> *)&m_cache)->Store(&c_key, c_result);

				return S_OK;
			}
	}

	return ERROR_FUELS_FUEL_UNKNOWN;
}


HRESULT CCWFGM_FuelMap::GetAvailableFileIndex(long *index) const {
	if (!index)								return E_POINTER;

	CRWThreadSemaphoreEngage engage(*(CRWThreadSemaphore *)&m_lock, SEM_FALSE);

	long avail=-1;
	for (std::uint8_t i = 0;i < 255; i++) {
		if (m_fileIndex[i] < avail && m_fileIndex[i] != -9999)
			avail = m_fileIndex[i];
	}
	if (avail == -1)
		avail = -2;
	else
		avail -= 1;
	*index = avail;
	return S_OK;
}


HRESULT CCWFGM_FuelMap::GetAttribute(std::uint16_t option, PolymorphicAttribute *value) const {
	if (!value)								return E_POINTER;

	switch (option) {
		case CWFGM_ATTRIBUTE_LOAD_WARNING: {
							*value = m_loadWarning;
							return S_OK;
						   }
	}
	return E_INVALIDARG;
}

/*! Polymorphic.  This routine sets an attribute/option value given the attribute/option index.  Currently does not
	perform any operation and is simply reserved for future functionality.
	\param option Reserved for future functionality.
	\param value Value for the attribute/option index
	\sa ICWFGM_FuelMap::SetAttribute

	\retval E_NOTIMPL This function is reserved for future functionality.
*/
HRESULT CCWFGM_FuelMap::SetAttribute(std::uint16_t /*option*/, const PolymorphicAttribute & /*value*/) {
	return E_NOTIMPL;
}
