cmake_minimum_required (VERSION 3.13)
if (MSVC)
set (CMAKE_SYSTEM_VERSION 10.0.22000.0)
endif (MSVC)
project (Grid VERSION ${PROMETHEUS_VERSION} DESCRIPTION "Grid Calculation Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE ON)

SET(LOWLEVEL_INCLUDE_DIR "error" CACHE STRING "The path to the includes from HSS LowLevel")
SET(WTIME_INCLUDE_DIR "error" CACHE STRING "The path to the WTime libraries includes")
SET(MULTITHREAD_INCLUDE_DIR "error" CACHE STRING "The path to the includes from HSS Multithread")
SET(MATH_INCLUDE_DIR "error" CACHE STRING "The path to HSS_Math include files")
SET(GEOGRAPHY_INCLUDE_DIR "error" CACHE STRING "The path to HSS_Math include files")
SET(THIRD_PARTY_INCLUDE_DIR "error" CACHE STRING "The path to third party include files")
SET(BOOST_INCLUDE_DIR "error" CACHE STRING "The path to boost include files")
SET(ERROR_CALC_INCLUDE_DIR "error" CACHE STRING "The path to the error calc include files")
SET(FWI_INCLUDE_DIR "error" CACHE STRING "The path to the FWI include files")
SET(FIREENGINE_INCLUDE_DIR "error" CACHE STRING "The path to the fire engine include files")
SET(FUEL_INCLUDE_DIR "error" CACHE STRING "The path to the fuel include files")
SET(GDAL_INCLUDE_DIR "error" CACHE STRING "The path to the GDAL include files")
SET(GSL_INCLUDE_DIR "error" CACHE STRING "The path to the GSL include files")
SET(PROTOBUF_INCLUDE_DIR "error" CACHE STRING "The path to the protobuf include files")

find_library(FOUND_MULTITHREAD_LIBRARY_PATH NAMES Multithread REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_LOWLEVEL_LIBRARY_PATH NAMES LowLevel REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_WTIME_LIBRARY_PATH NAMES WTime REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_MATH_LIBRARY_PATH NAMES hssmath REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_GEOGRAPHY_LIBRARY_PATH NAMES hssgeography REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_PROTOBUF_LIBRARY_PATH NAMES protobuf libprotobuf REQUIRED PATHS ${PROTOBUF_LIBRARY_DIR})
find_library(FOUND_ERROR_CALC_LIBRARY_PATH NAMES ErrorCalc REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_FWI_LIBRARY_PATH NAMES fwi REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_FUEL_LIBRARY_PATH NAMES fuel REQUIRED PATHS ${LOCAL_LIBRARY_DIR})
find_library(FOUND_GDAL_LIBRARY_PATH NAMES gdal gdal_i REQUIRED PATHS ${GDAL_LIBRARY_DIR})

if (MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -DPROTOBUF_USE_DLLS -DBOOST_ALL_DYN_LINK -D_CRT_SECURE_NO_WARNINGS /Zc:__cplusplus")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD -D_AFXDLL")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd -D_AFXDLL /W4")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost COMPONENTS atomic system iostreams REQUIRED PATHS ${BOOST_LIBRARY_DIR})
else ()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

set(Boost_LIBRARIES
    -L${BOOST_LIBRARY_DIR}/lib
    -lboost_system
    -lboost_iostreams)
endif (MSVC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_NO_MFC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG -DDEBUG")

find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

add_library(grid SHARED
    include/GridCOM.h
    include/CWFGM_internal.h
    cpp/cwfgmFilter.pb.cc
    cpp/cwfgmFuelMap.pb.cc
    cpp/cwfgmGrid.pb.cc
    cpp/wcsData.pb.cc
    cpp/CWFGM_Asset.cpp
    cpp/CWFGM_Asset.Serialize.cpp
    cpp/CWFGM_AttributeFilter.cpp
    cpp/CWFGM_AttributeFilter.Serialize.cpp
    cpp/CWFGM_FuelMap.cpp
    cpp/CWFGM_FuelMap.Serialize.cpp
    cpp/CWFGM_Grid.cpp
    cpp/CWFGM_Grid.Serialize.cpp
    cpp/CWFGM_LayerManager.cpp
    cpp/CWFGM_PolyReplaceGridFilter.cpp
    cpp/CWFGM_PolyReplaceGridFilter.Serialize.cpp
    cpp/CWFGM_ReplaceGridFilter.cpp
    cpp/CWFGM_ReplaceGridFilter.Serialize.cpp
    cpp/CWFGM_Target.cpp
    cpp/CWFGM_Target.Serialize.cpp
    cpp/CWFGM_TemporalAttributeFilter.cpp
    cpp/CWFGM_TemporalAttributeFilter.Serialize.cpp
    cpp/CWFGM_VectorFilter.cpp
    cpp/CWFGM_VectorFilter.Serialize.cpp
    cpp/ICWFGM_GridEngine.cpp
)

target_include_directories(grid
    PUBLIC ${WTIME_INCLUDE_DIR}
    PUBLIC ${PROTOBUF_INCLUDE_DIR}
    PUBLIC ${LOWLEVEL_INCLUDE_DIR}
    PUBLIC ${MULTITHREAD_INCLUDE_DIR}
    PUBLIC ${ERROR_CALC_INCLUDE_DIR}
    PUBLIC ${THIRD_PARTY_INCLUDE_DIR}
    PUBLIC ${MATH_INCLUDE_DIR}
    PUBLIC ${GEOGRAPHY_INCLUDE_DIR}
    PUBLIC ${FWI_INCLUDE_DIR}
    PUBLIC ${FUEL_INCLUDE_DIR}
    PUBLIC ${FIREENGINE_INCLUDE_DIR}
    PUBLIC ${GDAL_INCLUDE_DIR}
    PUBLIC ${GSL_INCLUDE_DIR}
    PUBLIC ${BOOST_INCLUDE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(grid PROPERTIES VERSION ${CMAKE_PROJECT_VERSION})
set_target_properties(grid PROPERTIES SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR})
set_target_properties(grid PROPERTIES DEFINE_SYMBOL "GRID_EXPORTS")

set_target_properties(grid PROPERTIES
    PUBLIC_HEADER include/CWFGM_Asset.h
    PUBLIC_HEADER include/CWFGM_AttributeFilter.h
    PUBLIC_HEADER include/CWFGM_FuelMap.h
    PUBLIC_HEADER include/CWFGM_Grid.h
    PUBLIC_HEADER include/CWFGM_LayerManager.h
    PUBLIC_HEADER include/CWFGM_PolyReplaceGridFilter.h
    PUBLIC_HEADER include/CWFGM_ReplaceGridFilter.h
    PUBLIC_HEADER include/CWFGM_Target.h
    PUBLIC_HEADER include/CWFGM_TemporalAttributeFilter.h
    PUBLIC_HEADER include/CWFGM_VectorFilter.h
    PUBLIC_HEADER include/cwfgmFilter.pb.h
    PUBLIC_HEADER include/cwfgmFuelMap.pb.h
    PUBLIC_HEADER include/cwfgmGrid.pb.h
    PUBLIC_HEADER include/GridCom_ext.h
    PUBLIC_HEADER include/ICWFGM_GridEngine.h
    PUBLIC_HEADER include/ICWFGM_Target.h
    PUBLIC_HEADER include/ICWFGM_VectorEngine.h
    PUBLIC_HEADER include/ICWFGM_Weather.h
    PUBLIC_HEADER include/results.h
    PUBLIC_HEADER include/wcsData.pb.h
    PUBLIC_HEADER include/XYPoly.h
)

target_link_libraries(grid ${FOUND_GDAL_LIBRARY_PATH} ${Boost_LIBRARIES} ${FOUND_PROTOBUF_LIBRARY_PATH})
target_link_libraries(grid ${FOUND_FWI_LIBRARY_PATH} ${FOUND_FUEL_LIBRARY_PATH})
target_link_libraries(grid ${FOUND_WTIME_LIBRARY_PATH} ${FOUND_LOWLEVEL_LIBRARY_PATH} ${FOUND_MULTITHREAD_LIBRARY_PATH} ${FOUND_ERROR_CALC_LIBRARY_PATH} ${FOUND_MATH_LIBRARY_PATH} ${FOUND_GEOGRAPHY_LIBRARY_PATH})
if (MSVC)
target_link_libraries(grid -L${LOCAL_LIBRARY_DIR})
else ()
target_link_libraries(grid -lstdc++fs)
endif (MSVC)
