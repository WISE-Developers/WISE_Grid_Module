/**
 * WISE_Grid_Module: CWFGM_ReplaceGridFilter.h
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
#include "FireEngine_ext.h"
#include "CWFGM_ReplaceGridFilter.h"
#include "angles.h"
#include "points.h"
#include "geo_poly.h"
#include "CoordinateConverter.h"
#include "doubleBuilder.h"

#include <errno.h>
#include <stdio.h>

#include <boost/scoped_ptr.hpp>

#ifdef DEBUG
#include <assert.h>
#endif

std::int32_t CCWFGM_ReplaceGridFilter::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmReplaceGridFilter *CCWFGM_ReplaceGridFilter::serialize(const SerializeProtoOptions& options) {
	auto filter = new WISE::GridProto::CwfgmReplaceGridFilter();
	filter->set_version(serialVersionUid(options));

	if ((m_x1 == (std::uint16_t)-1) && (m_y1 == (std::uint16_t)-1) && (m_x2 == (std::uint16_t)-1) && (m_y2 == (std::uint16_t)-1))
		filter->set_allocated_landscape(createProtobufObject(true));
	else {
		GeoPoint point1(XY_Point(invertX(m_x1), invertY(m_y1)));
		point1.setStoredUnits(GeoPoint::UTM);
		filter->set_allocated_pointone(point1.getProtobuf(options.useVerboseFloats()));

		GeoPoint point2(XY_Point(invertX(m_x2), invertY(m_y2)));
		point2.setStoredUnits(GeoPoint::UTM);
		filter->set_allocated_pointtwo(point2.getProtobuf(options.useVerboseFloats()));

		filter->set_allocated_resolution(DoubleBuilder().withValue(m_resolution).forProtobuf(options.useVerboseFloats()));
		filter->set_allocated_xllcorner(DoubleBuilder().withValue(m_xllcorner).forProtobuf(options.useVerboseFloats()));
		filter->set_allocated_yllcorner(DoubleBuilder().withValue(m_yllcorner).forProtobuf(options.useVerboseFloats()));
	}
	return filter;
}


CCWFGM_ReplaceGridFilter *CCWFGM_ReplaceGridFilter::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(0))) {
		if (valid)
			/// <summary>
			/// The gridEngine is not initialized but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmReplaceGridFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "gridEngine");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmReplaceGridFilter: No grid engine";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmReplaceGridFilter: Incomplete initialization");
	}

	auto filter = dynamic_cast_assert<const WISE::GridProto::CwfgmReplaceGridFilter*>(&proto);

	if (!filter)
	{
		if (valid)
			/// <summary>
			/// The object passed as a grid filter is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmReplaceGridFilter", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmReplaceGridFilter: Protobuf object invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmReplaceGridFilter: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}
	if ((filter->version() != 1) && (filter->version() != 2))
	{
		if (valid)
			/// <summary>
			/// The object version is not supported. The grid filter is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmReplaceGridFilter", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(filter->version()));
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmReplaceGridFilter: Version is invalid";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmReplaceGridFilter: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	PolymorphicAttribute var;

	if (FAILED(gridEngine->GetAttribute(0, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &var))) {
		if (valid)
			/// <summary>
			/// The projection is not readable but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmReplaceGridFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "projection");
		weak_assert(false);
		throw std::exception();
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmReplaceGridFilter", name);
	auto v = vt.lock();

	std::string projection;
	projection = std::get<std::string>(var);

	boost::scoped_ptr<CCoordinateConverter> convert(new CCoordinateConverter());
	convert->SetSourceProjection(projection.c_str());

	if ((filter->has_landscape() &&
		(filter->landscape().value()))) {
		m_x1 = m_y1 = m_x2 = m_y2 = (std::uint16_t)-1;
	}
	else {
		if (filter->has_resolution() && filter->has_xllcorner() && filter->has_yllcorner())
		{
			m_resolution = DoubleBuilder().withProtobuf(filter->resolution()).getValue();
			m_xllcorner = DoubleBuilder().withProtobuf(filter->xllcorner()).getValue();
			m_yllcorner = DoubleBuilder().withProtobuf(filter->yllcorner()).getValue();
			m_iresolution = 1.0 / m_resolution;
		}
		else
			fixResolution();

		if (filter->has_pointone())
		{
			GeoPoint geo(filter->pointone());
			geo.setStoredUnits(GeoPoly::UTM);
			geo.setConverter([&convert](std::uint8_t type, double x, double y, double z) -> std::tuple<double, double, double>
				{
					XY_Point loc = convert->start()
						.fromPoints(x, y, z)
						.asLatLon()
						.endInUTM()
						.to2DPoint();
					return std::make_tuple(loc.x, loc.y, 0.0);
				});
			m_x1 = (std::uint16_t)convertX(geo.getPoint(v, "pointone.x").x, nullptr);
			m_y1 = (std::uint16_t)convertY(geo.getPoint(v, "pointone.y").y, nullptr);
		}
		else {
			m_x1 = m_y1 = (std::uint16_t) - 1;
		}

		if (filter->has_pointtwo())
		{
			GeoPoint geo(filter->pointtwo());
			geo.setStoredUnits(GeoPoly::UTM);
			geo.setConverter([&convert](std::uint8_t type, double x, double y, double z) -> std::tuple<double, double, double>
				{
					XY_Point loc = convert->start()
						.fromPoints(x, y, z)
						.asLatLon()
						.endInUTM()
						.to2DPoint();
					return std::make_tuple(loc.x, loc.y, 0.0);
				});
			m_x2 = (std::uint16_t)convertX(geo.getPoint(v, "pointtwo.x").x, nullptr);
			m_y2 = (std::uint16_t)convertY(geo.getPoint(v, "pointtwo.y").y, nullptr);
		}
		else {
			m_x2 = m_y2 = (std::uint16_t) - 1;
		}
	}
	return this;
}
