/**
 * WISE_Grid_Module: CWFGM_FuelMap.Serialize.h
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

#include "CWFGM_Fuel_Shared.h"
#include "doubleBuilder.h"
#include "str_printf.h"


std::int32_t CCWFGM_FuelMap::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmFuelMap* CCWFGM_FuelMap::serialize(const SerializeProtoOptions& options) {
	auto map = new WISE::GridProto::CwfgmFuelMap();
	map->set_version(serialVersionUid(options));

	int j = 0;
	for (int i = 254; i >= 0; i--)
		if (m_fileIndex[i] >= 0) {
			j = i + 1;
			break;
		}
	for (int i = 0; i < j; i++) {
		auto data = map->add_data();
		data->set_index(m_fileIndex[i]);
		data->set_exportindex(m_exportFileIndex[i]);

		std::int8_t valid = 0;
		if (m_fuel[i]) {
			valid = 1;
			for (std::int32_t fj = 0; fj < i; fj++) {
				if ((m_fuel[fj]) && (m_fuel[i] == m_fuel[fj])) {
					valid = 2;
					data->set_allocated_fuelindex(createProtobufObject(fj));
					break;
				}
			}
		}

		if (valid == 1) {
			auto fuel = dynamic_cast<CCWFGM_Fuel*>(m_fuel[i].get());
			weak_assert(fuel);
			if (fuel)
				data->set_allocated_fueldata(dynamic_cast_assert<WISE::FuelProto::CcwfgmFuel*>(fuel->serialize(options)));
		}
	}

	return map;
}


CCWFGM_FuelMap *CCWFGM_FuelMap::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	auto map = dynamic_cast_assert<const WISE::GridProto::CwfgmFuelMap*>(&proto);

	if (!map) {
		if (valid)
			/// <summary>
			/// The object passed as a fuel map is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmFuelMap", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(0);
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmFuelMap: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}

	if ((map->version() != 1) && (map->version() != 2)) {
		if (valid)
			/// <summary>
			/// The object version is not supported. The fuel map is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmFuelMap", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(map->version()));
		weak_assert(0);
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmFuelMap: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmFuelMap", name);
	auto myValid = vt.lock();

	for (int i = 0; i < 255; i++) {
		if (i < map->data_size()) {
			auto data = map->data(i);
			m_fileIndex[i] = data.index();
			m_exportFileIndex[i] = data.exportindex();
			if (data.has_fueldata()) {
				std::string name(strprintf("data[%d]", i));
				m_fuel[i] = dynamic_cast<ICWFGM_Fuel *>(CCWFGM_Fuel::deserializeFuel(data.fueldata(), myValid, name));
				if (!m_fuel[i]) {
					/// <summary>
					/// The COM object could not be instantiated.
					/// </summary>
					/// <type>internal</type>
					if (myValid)
						myValid->add_child_validation("WISE.FuelProto.CcwfgmFuel", name, validation::error_level::SEVERE, validation::id::cannot_allocate, "CLSID_CWFGM_Fuel");
					weak_assert(0);
					return nullptr;
				}
			}
			else if (data.has_fuelindex())
				m_fuel[i] = m_fuel[data.fuelindex().value()];
		}
		else
			m_fuel[i] = nullptr;
	}

	return this;
}
