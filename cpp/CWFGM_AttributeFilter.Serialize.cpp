/**
 * WISE_Grid_Module: CWFGM_AttributeFilter.Serialize.cpp
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

#include "types.h"
#include "CWFGM_AttributeFilter.h"
#include "lines.h"
#include "GridCom_ext.h"
#include "FireEngine_ext.h"
#include <errno.h>
#include <stdio.h>
#include "CoordinateConverter.h"
#include "GDALExporter.h"
#include "GDALImporter.h"
#include "gdalclient.h"
#include "doubleBuilder.h"
#include "boost_compression.h"
#include "GDALextras.h"
#include "filesystem.hpp"
#include <ctime>

using namespace GDALExtras;

/////////////////////////////////////////////////////////////////////////////
// CWFGM_AttributeFilter

HRESULT CCWFGM_AttributeFilter::ExportAttributeGrid(const std::string &prj_file_name, const std::string &grid_file_name, const std::string &band_name) {
	CRWThreadSemaphoreEngage engage(m_lock, SEM_FALSE);

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(nullptr)))					{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }

	if (grid_file_name.length()==0)
		return E_INVALIDARG;

	double* d_array = nullptr;
	int* l_array = nullptr;
	bool use_double = false;
	if (m_optionType == VT_R4 || m_optionType == VT_R8)
		use_double = true;

	if (use_double)
		d_array = new double[m_xsize * m_ysize];
	else
		l_array = new int[m_xsize * m_ysize];

	int* l_pointer = l_array;
	double* d_pointer = d_array;

	NumericVariant var;
	grid::AttributeValue var_valid;
	for(std::uint16_t i = m_ysize - 1; i < m_ysize; i--) {
		for(std::uint16_t j = 0; j < m_xsize; j++) {
			XY_Point pt;
			pt.x = j;
			pt.y = i;
			getPoint(pt, &var, &var_valid);

			/*POLYMORPHIC CHECK*/
			if (var_valid != grid::AttributeValue::NOT_SET) {

				if (std::holds_alternative<std::int8_t>(var)) {
					*l_pointer = std::get<std::int8_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<std::int16_t>(var)) {
					*l_pointer = std::get<std::int16_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<std::int32_t>(var)) {
					*l_pointer = std::get<std::int32_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<std::int64_t>(var)) {
					*l_pointer = std::get<std::int64_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<std::uint8_t>(var)) {

					if (m_optionKey == (std::uint16_t)(-1)) {
						long file_index, export_index;
						ICWFGM_Fuel *fuel;
						m_fuelMap->FuelAtIndex(std::get<std::uint8_t>(var), &file_index, &export_index, &fuel);
						*l_pointer = export_index;
					}
					else {
						*l_pointer = std::get<std::uint8_t>(var);
					}
					l_pointer++;

				}
				else if (std::holds_alternative<std::uint16_t>(var)) {
					*l_pointer = std::get<std::uint16_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<std::uint32_t>(var)) {
					*l_pointer = std::get<std::uint32_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<std::uint64_t>(var)) {
					*l_pointer = std::get<std::uint64_t>(var);
					l_pointer++;

				}
				else if (std::holds_alternative<float>(var)) {
					*d_pointer = std::get<float>(var);
					d_pointer++;

				}
				else if (std::holds_alternative<double>(var)) {
					*d_pointer = std::get<double>(var);
					d_pointer++;

				}
				else if (std::holds_alternative<bool>(var)) {
					*l_pointer = (std::get<bool>(var) == false) ? 0 : 1;
					l_pointer++;

				}
				else {
					weak_assert(false);
					if (use_double)
					{
						*d_pointer = -9999;
						d_pointer++;
					}
					else {
						*l_pointer = -9999;
						l_pointer++;
					}
				}
				
			}
			else {
				if (use_double) {
					*d_pointer = -9999;
					d_pointer++;
				}
				else {
					*l_pointer = -9999;
					l_pointer++;
				}

			}
		}
	}

	GDALExporter exporter;
	exporter.AddTag("TIFFTAG_SOFTWARE", "W.I.S.E.");
	exporter.AddTag("TIFFTAG_GDAL_NODATA", "-9999");
	char mbstr[100];
	std::time_t t = std::time(nullptr);
	std::strftime(mbstr, sizeof(mbstr), "%Y-%m-%d %H:%M:%S %Z", std::localtime(&t));
	exporter.AddTag("TIFFTAG_DATETIME", mbstr);
	PolymorphicAttribute v;
	HRESULT hr = S_OK;
	if (SUCCEEDED(gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &v))) {
		std::string ref;

		/*POLYMORPHIC CHECK*/
		try { ref = std::get<std::string>(v); }
		catch (std::bad_variant_access&) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		exporter.setProjection(ref.c_str());
		exporter.setSize(m_xsize, m_ysize);

		if ((m_xllcorner == -999999999.0) && (m_yllcorner == -999999999.0) && (m_resolution == -1.0)) {
			weak_assert(false);
			fixResolution(nullptr, "");
		}
		exporter.setPixelResolution(m_resolution, m_resolution);
		exporter.setLowerLeft(m_xllcorner, m_yllcorner);
		GDALExporter::ExportResult res;
		if (use_double)
			res = exporter.Export(d_array, grid_file_name.c_str(), band_name.c_str());
		else
			res = exporter.Export(l_array, grid_file_name.c_str(), band_name.c_str());

		if (res == GDALExporter::ExportResult::ERROR_ACCESS)
			hr = E_ACCESSDENIED;
	}
	else
		hr = ERROR_GRID_UNINITIALIZED;

	if (use_double && d_array)
		delete [] d_array;
	else if (l_array)
		delete [] l_array;
	
	return hr;
}


HRESULT CCWFGM_AttributeFilter::ImportAttributeGrid(const std::string & prj_file_name, const std::string & grid_file_name) {
	if (!grid_file_name.length())							return E_INVALIDARG;

	SEM_BOOL engaged;
	CRWThreadSemaphoreEngage engage(m_lock, SEM_TRUE, &engaged, 1000000LL);
	if (!engaged)											return ERROR_SCENARIO_SIMULATION_RUNNING;

	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(nullptr)))				{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	if ((m_optionType == VT_UI1) && (m_optionKey == (std::uint16_t)-1) && (!m_fuelMap))
															{ weak_assert(false); return ERROR_GRID_UNINITIALIZED; }
	std::uint32_t size;

	switch (m_optionType) {
		case VT_BOOL:
		case VT_I1:
		case VT_UI1:	size = 1; break;
		case VT_I2:
		case VT_UI2:	size = 2; break;
		case VT_I4:
		case VT_UI4:
		case VT_R4:	size = 4; break;
		case VT_I8:
		case VT_UI8:
		case VT_R8:	size = 8; break;
		default:	return E_UNEXPECTED;
	}

	HRESULT error = S_OK;

	GDALImporter importer;
	if (importer.Import(grid_file_name.c_str(), nullptr) != GDALImporter::ImportResult::OK) {
		return E_FAIL;
	}
	
	if (strlen(importer.projection())) {
		CSemaphoreEngage lock(GDALClient::GDALClient::getGDALMutex(), SEM_TRUE);

		//ensure the correct projection
		OGRSpatialReferenceH sourceSRS, m_sourceSRS;

		sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(importer.projection());

		PolymorphicAttribute v;
		gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_SPATIALREFERENCE, &v);
		std::string csProject;

		/*POLYMORPHIC CHECK*/
		try { csProject = std::get<std::string>(v); } catch (std::bad_variant_access &) { weak_assert(false); return ERROR_PROJECTION_UNKNOWN; };

		m_sourceSRS = CCoordinateConverter::CreateSpatialReferenceFromWkt(csProject.c_str());
		if (m_sourceSRS) {
			if (!sourceSRS)
				error = ERROR_GRID_LOCATION_OUT_OF_RANGE;
			else {
				if (!OSRIsSame(m_sourceSRS, sourceSRS, false))
					error = ERROR_GRID_LOCATION_OUT_OF_RANGE;
			}
		}
		else
			error = E_FAIL;

		if (sourceSRS)		OSRDestroySpatialReference(sourceSRS);
		if (m_sourceSRS)	OSRDestroySpatialReference(m_sourceSRS);

		if (FAILED(error)) {
			return error;
		}
	}

	std::uint16_t		xsize,
			ysize;					// size of our plots
	double	noData;
	std::uint32_t		index, i;

	int8_t	*m_origArray = m_array_i1;
	bool	*m_origNoDataArray = m_array_nodata;

	int datatype;
	if (importer.importType() == GDALImporter::ImportType::FLOAT32)
		datatype = 0;
	else if (importer.importType() == GDALImporter::ImportType::FLOAT64)
		datatype = 1;
	else
		datatype = 2;

	//ensure the correct location
	double gridResolution, gridXLL, gridYLL;
	std::uint16_t gridXDim, gridYDim;
	PolymorphicAttribute var;

	/*POLYMORPHIC CHECK*/
	if (FAILED(error = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_PLOTRESOLUTION, &var))) return error;
	VariantToDouble_(var, &gridResolution);

	if (FAILED(error = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_XLLCORNER, &var))) return error;
	VariantToDouble_(var, &gridXLL);

	if (FAILED(error = gridEngine->GetAttribute(nullptr, CWFGM_GRID_ATTRIBUTE_YLLCORNER, &var))) return error;
	VariantToDouble_(var, &gridYLL);

	if (FAILED(error = gridEngine->GetDimensions(0, &gridXDim, &gridYDim)))
		return error;
	if ((gridXDim != importer.xSize()) ||
		(gridYDim != importer.ySize()))
		return ERROR_GRID_SIZE_INCORRECT;
	if ((fabs(gridResolution - importer.xPixelSize()) > 0.0001) ||
		(fabs(gridResolution - importer.yPixelSize()) > 0.0001))
		return ERROR_GRID_UNSUPPORTED_RESOLUTION;
	if ((fabs(gridXLL - importer.lowerLeftX()) > 0.001) ||
		(fabs(gridYLL - importer.lowerLeftY()) > 0.001))
		return ERROR_GRID_LOCATION_OUT_OF_RANGE;

	//set the size and nodata
	xsize = importer.xSize();
	ysize = importer.ySize();
	noData = importer.nodata();

	if (error == S_OK) {
		index = xsize * ysize;
		m_array_i1 = nullptr;
		m_array_nodata = nullptr;

		m_array_i1 = (int8_t *)malloc((size_t)index * (size_t)size);
		m_array_nodata = (bool *)malloc((size_t)index * sizeof(bool));
		if ((!m_array_i1) || (!m_array_nodata)) {
			if (m_array_i1)		free(m_array_i1);
			if (m_array_nodata)	free(m_array_nodata);
			m_array_i1 = m_origArray;
			m_array_nodata = m_origNoDataArray;
			return E_OUTOFMEMORY;
		}
							
		for (i = 0; i < index; i++) {
			std::int64_t lscan;
			double dscan;
			bool bNoData = false;

			switch (datatype) {
				case 0:
					dscan = importer.floatData(1, i);
					if (dscan == noData) bNoData = true;
					lscan = (long)dscan;
					break;
				case 1:
					dscan = importer.doubleData(1, i);
					if (dscan == noData) bNoData = true;
					lscan = (long)dscan;
					break;
				case 2:
					lscan = importer.integerData(1, i);
					if (lscan == (std::int64_t)noData) bNoData = true;
					dscan = (double)lscan;
					break;
			}
			if (bNoData)
				m_array_nodata[i] = true;
			else {
				m_array_nodata[i] = false;
				switch (m_optionType) {
					case VT_BOOL:	m_array_i1[i] = (lscan) ? 1 : 0; break;
					case VT_I1:	m_array_i1[i] = (std::int8_t)lscan; break;
					case VT_I2:	m_array_i2[i] = (std::int16_t)lscan; break;
					case VT_I4:	m_array_i4[i] = (std::int32_t)lscan; break;
					case VT_I8:	m_array_i8[i] = lscan; break;
					case VT_UI1:	if (m_optionKey == (std::uint16_t)-1) {
								std::uint8_t internal_index;
								long export_index;
								ICWFGM_Fuel *fuel;
								if (FAILED(m_fuelMap->FuelAtFileIndex(lscan, &internal_index, &export_index, &fuel))) {
									error = ERROR_FUELS_FUEL_UNKNOWN;
									goto FAILURE;
								}
								m_array_ui1[i] = internal_index;
							} else
								m_array_ui1[i] = (std::uint8_t)lscan;
							break;
					case VT_UI2:	m_array_ui2[i] = (std::uint16_t)lscan; break;
					case VT_UI4:	m_array_ui4[i] = (std::uint32_t)lscan; break;
					case VT_UI8:	m_array_ui8[i] = (std::uint32_t)lscan; break;
					case VT_R4:	m_array_r4[i] = (float)dscan; break;
					case VT_R8:	m_array_r8[i] = dscan; break;
				}
			}
		}

		if (m_origArray) {
			free(m_origArray);
			error = SUCCESS_GRID_DATA_UPDATED;
		}
		if (m_origNoDataArray)
			free(m_origNoDataArray);
		m_xsize = xsize;
		m_ysize = ysize;

		m_resolution = gridResolution;
		m_iresolution = 1.0 / m_resolution;
		m_xllcorner = gridXLL;
		m_yllcorner = gridYLL;
		
		m_bRequiresSave = true;
	}
	return error;

FAILURE:
	free(m_array_i1);
	free(m_array_nodata);
	m_array_i1 = m_origArray;
	m_array_nodata = m_origNoDataArray;
	return error;
}


HRESULT CCWFGM_AttributeFilter::ImportAttributeGridWCS( const std::string & url,  const std::string & layer,  const std::string & username,  const std::string & password) {
	return E_NOTIMPL;
}


HRESULT CCWFGM_AttributeFilter::ExportAttributeGridWCS( const std::string & url,  const std::string & layer,  const std::string & username,  const std::string & password) {
	return E_NOTIMPL;
}


std::int32_t CCWFGM_AttributeFilter::serialVersionUid(const SerializeProtoOptions& options) const noexcept {
	return options.fileVersion();
}


WISE::GridProto::CwfgmAttributeFilter *CCWFGM_AttributeFilter::serialize(const SerializeProtoOptions& options) {
	auto filter = new WISE::GridProto::CwfgmAttributeFilter();
	filter->set_version(serialVersionUid(options));

	auto binary = new WISE::GridProto::CwfgmAttributeFilter_BinaryData();
	filter->set_allocated_binary(binary);

	binary->set_xsize(m_xsize);
	binary->set_ysize(m_ysize);
	binary->set_datakey((::WISE::GridProto::CwfgmAttributeFilter_DataKey)m_optionKey);

	if (m_xllcorner == -999999999.0 || m_yllcorner == -999999999.0) {
		weak_assert(false);
		fixResolution(nullptr, "");
	}

	binary->set_allocated_xllcorner(DoubleBuilder().withValue(m_xllcorner).forProtobuf(options.useVerboseFloats()));
	binary->set_allocated_yllcorner(DoubleBuilder().withValue(m_yllcorner).forProtobuf(options.useVerboseFloats()));
	binary->set_allocated_resolution(DoubleBuilder().withValue(m_resolution).forProtobuf(options.useVerboseFloats()));

	int size;
	switch (m_optionType) {
	case VT_BOOL:
		size = 1;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_BOOL);
		break;
	case VT_I1:
		size = 1;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_CHAR);
		break;
	case VT_UI1:
		size = 1;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_CHAR);
		break;
	case VT_I2:
		size = 2;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_SHORT);
		break;
	case VT_UI2:
		size = 2;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_SHORT);
		break;
	case VT_I4:
		size = 4;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_INTEGER);
		break;
	case VT_UI4:
		size = 4;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_INTEGER);
		break;
	case VT_R4:
		size = 4;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_FLOAT);
		break;
	case VT_R8:
		size = 8;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_LONG_FLOAT);
		break;
	case VT_UI8:
		size = 8;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_LONG);
		break;
	case VT_I8:
		size = 8;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_LONG);
		break;
	default:
		size = 0;
		binary->set_type(WISE::GridProto::CwfgmAttributeFilter_Type_EMPTY);
		break;
	}

	if (binary->type() != WISE::GridProto::CwfgmAttributeFilter_Type_EMPTY) {
		if (options.useVerboseOutput() || !options.zipOutput()) {
			auto bts = new google::protobuf::BytesValue();
			bts->set_value(m_array_i1, m_xsize * m_ysize * size);
			binary->set_allocated_data(bts);
		}
		else {
			binary->set_allocated_iszipped(createProtobufObject(true));
			auto bts = new google::protobuf::BytesValue();
			bts->set_value(Compress::compress(reinterpret_cast<const char*>(m_array_i1), m_xsize * m_ysize * size));
			binary->set_allocated_data(bts);
		}
	}

	if (m_array_nodata) {
		if (options.useVerboseOutput() || !options.zipOutput()) {
			auto bts = new google::protobuf::BytesValue();
			bts->set_value(m_array_nodata, m_xsize * m_ysize);
			binary->set_allocated_nodata(bts);
		}
		else {
			binary->set_allocated_iszipped(createProtobufObject(true));
			auto bts = new google::protobuf::BytesValue();
			bts->set_value(Compress::compress(reinterpret_cast<const char*>(m_array_nodata), m_xsize * m_ysize));
			binary->set_allocated_nodata(bts);
		}
	}

	return filter;
}


CCWFGM_AttributeFilter *CCWFGM_AttributeFilter::deserialize(const google::protobuf::Message& proto, std::shared_ptr<validation::validation_object> valid, const std::string& name) {
	boost::intrusive_ptr<ICWFGM_GridEngine> gridEngine;
	if (!(gridEngine = m_gridEngine(nullptr))) {
		if (valid)
			/// <summary>
			/// The gridEngine is not initialized but should be by this time in deserialization.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmAttributeFilter", name, validation::error_level::SEVERE,
				validation::id::initialization_incomplete, "gridEngine");
		weak_assert(false);
		m_loadWarning = "Error: WISE.GridProto.CwfgmAttributeFilter: No grid engine";
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmAttributeFilter: Incomplete initialization");
	}

	auto filter = dynamic_cast_assert<const WISE::GridProto::CwfgmAttributeFilter*>(&proto);

	if (!filter) {
		if (valid)
			/// <summary>
			/// The object passed as an attribute filter is invalid. An incorrect object type was passed to the parser.
			/// </summary>
			/// <type>internal</type>
			valid->add_child_validation("WISE.GridProto.CwfgmAttributeFilter", name, validation::error_level::SEVERE, validation::id::object_invalid, proto.GetDescriptor()->name());
		weak_assert(false);
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmAttributeFilter: Protobuf object invalid", ERROR_PROTOBUF_OBJECT_INVALID);
	}
	if ((filter->version() != 1) && (filter->version() != 2)) {
		if (valid)
			/// <summary>
			/// The object version is not supported. The attribute filter is not supported by this version of Prometheus.
			/// </summary>
			/// <type>user</type>
			valid->add_child_validation("WISE.GridProto.CwfgmAttributeFilter", name, validation::error_level::SEVERE, validation::id::version_mismatch, std::to_string(filter->version()));
		weak_assert(false);
		throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmAttributeFilter: Version is invalid", ERROR_PROTOBUF_OBJECT_VERSION_INVALID);
	}

	auto vt = validation::conditional_make_object(valid, "WISE.GridProto.CwfgmAttributeFilter", name);
	auto v = vt.lock();

	if (filter->data_case() == WISE::GridProto::CwfgmAttributeFilter::kBinary) {
		auto vt2 = validation::conditional_make_object(v, "WISE.GridProto.CwfgmAttributeFilter.Binary", "binary");
		auto v2 = vt2.lock();

		m_xsize = filter->binary().xsize();
		m_ysize = filter->binary().ysize();

		if (filter->binary().has_data()) {
			if (m_array_i1)
				free(m_array_i1);
			if (filter->binary().has_iszipped() && filter->binary().iszipped().value()) {
				std::string val = Compress::decompress(filter->binary().data().value());
				m_array_i1 = (std::int8_t *)malloc(val.size());
				if (!m_array_i1) {
					if (valid)
						/// <summary>
						/// The process is out of memory.
						/// </summary>
						/// <type>internal</type>
						valid->add_child_validation("CWFGM", name, validation::error_level::SEVERE,
							validation::id::out_of_memory, std::to_string(val.size()));
					weak_assert(false);
					m_loadWarning = "Error: WISE.GridProto.CwfgmAttributeFilter: No more memory";
					throw std::bad_alloc();
				}
				std::copy(val.begin(), val.end(), m_array_i1);
			}
			else {
				m_array_i1 = (std::int8_t *)malloc(filter->binary().data().value().length());
				if (!m_array_i1) {
					if (valid)
						/// <summary>
						/// The process is out of memory.
						/// </summary>
						/// <type>internal</type>
						valid->add_child_validation("CWFGM", name, validation::error_level::SEVERE,
							validation::id::out_of_memory, std::to_string(filter->binary().data().value().length()));
					weak_assert(false);
					m_loadWarning = "Error: WISE.GridProto.CwfgmAttributeFilter: No more memory";
					throw std::bad_alloc();
				}
				std::copy(filter->binary().data().value().begin(), filter->binary().data().value().end(), m_array_i1);
			}
		}

		if (filter->binary().has_nodata()) {
			if (m_array_nodata)
				free(m_array_nodata);
			if (filter->binary().has_iszipped() && filter->binary().iszipped().value()) {
				std::string val = Compress::decompress(filter->binary().nodata().value());
				m_array_nodata = (bool *)malloc(val.size() * sizeof(bool));
				if (!m_array_nodata) {
					if (valid)
						/// <summary>
						/// The process is out of memory.
						/// </summary>
						/// <type>internal</type>
						valid->add_child_validation("CWFGM", name, validation::error_level::SEVERE,
							validation::id::out_of_memory, std::to_string(val.size()));
					weak_assert(false);
					m_loadWarning = "Error: WISE.GridProto.CwfgmAttributeFilter: No more memory";
					throw std::bad_alloc();
				}
				std::copy(val.begin(), val.end(), m_array_nodata);
			}
			else {
				m_array_nodata = (bool *)malloc(filter->binary().nodata().value().length() * sizeof(bool));
				if (!m_array_nodata) {
					if (valid)
						/// <summary>
						/// The process is out of memory.
						/// </summary>
						/// <type>internal</type>
						valid->add_child_validation("CWFGM", name, validation::error_level::SEVERE,
							validation::id::out_of_memory, std::to_string(filter->binary().nodata().value().length() * sizeof(bool)));
					weak_assert(false);
					m_loadWarning = "Error: WISE.GridProto.CwfgmAttributeFilter: No more memory";
					throw std::bad_alloc();
				}
				std::copy(filter->binary().nodata().value().begin(), filter->binary().nodata().value().end(), m_array_nodata);
			}
		}

		if (filter->binary().has_xllcorner() && filter->binary().has_yllcorner() && filter->binary().has_resolution()) {
			m_xllcorner = DoubleBuilder().withProtobuf(filter->binary().xllcorner()).getValue();
			m_yllcorner = DoubleBuilder().withProtobuf(filter->binary().yllcorner()).getValue();
			m_resolution = DoubleBuilder().withProtobuf(filter->binary().resolution()).getValue();
			m_iresolution = 1.0 / m_resolution;
		}
		else {
			HRESULT hr;
			if (FAILED(hr = fixResolution(valid, name))) {
				throw ISerializeProto::DeserializeError("WISE.GridProto.CwfgmAttributeFilter: Incomplete initialization");
			}
		}
		if (filter->has_binary()) {
			if (filter->binary().has_key())
				m_optionKey = filter->binary().key().value();
			else
				m_optionKey = (std::uint16_t)filter->binary().datakey();
		}
		else
			m_optionKey = (std::uint16_t)filter->file().datakey();

		if ((m_optionKey >= 15506) && (m_optionKey <= 15511))
			m_optionKey += 100;								// fixes some ancient testing files

		switch (filter->binary().type()) {
		case WISE::GridProto::CwfgmAttributeFilter_Type_BOOL:
			m_optionType = VT_BOOL;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_CHAR:
			m_optionType = VT_I1;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_CHAR:
			m_optionType = VT_UI1;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_SHORT:
			m_optionType = VT_I2;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_SHORT:
			m_optionType = VT_UI2;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_INTEGER:
			m_optionType = VT_I4;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_INTEGER:
			m_optionType = VT_UI4;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_FLOAT:
			m_optionType = VT_R4;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_LONG_FLOAT:
			m_optionType = VT_R8;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_UNSIGNED_LONG:
			m_optionType = VT_UI8;
			break;
		case WISE::GridProto::CwfgmAttributeFilter_Type_LONG:
			m_optionType = VT_I8;
			break;
		default:
			if (v2)
				/// <summary>
				/// The type field has an unknown value.
				/// </summary>
				/// <type>internal</type>
				v2->add_child_validation("WISE.GridProto.CwfgmAttributeFilter.Type", "type", validation::error_level::SEVERE,
					validation::id::enum_invalid, std::to_string(filter->binary().type()));

			break;
		}
	}
	else if (filter->data_case() == WISE::GridProto::CwfgmAttributeFilter::kFile) {
	/* done in the client code
	*/
	}
	else {
	if (v)
		v->add_child_validation("string", "filename", validation::error_level::SEVERE, validation::id::oneof_invalid, std::to_string(filter->data_case()));
	}

	return this;
}
