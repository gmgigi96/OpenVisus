CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # qt-related
endif()

if (POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD) # qt-related
endif()


if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD) # swig related 
endif()

if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD)  # swig related
endif()

if (POLICY CMP0087)
  cmake_policy(SET CMP0087 NEW)  # cmake generator expression in install
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  
SET(MSVC_INCREMENTAL_DEFAULT OFF) # disable incremental linking for windows

PROJECT(OpenVisus) 

# detect compiler
if (MSVC)
	MESSAGE(STATUS "Detected visual studio (C++ __MSVC_VER - WIN32)")
elseif (CMAKE_CXX_COMPILER MATCHES ".*clang")
	set(CLANG 1)
	MESSAGE(STATUS "Detected apple clang (C++: __clang__ __APPLE__)")
else()
	set(GNUC  1)
	if (MINGW)
		MESSAGE(STATUS "Detected GNU on windows (C++ __GNUC__ WIN32)")
		#minimum support is Vista
		add_compile_options(-D_WIN32_WINNT=0x0600)
		add_compile_options(-DWINVER=0x0600)
		set(WIN32 1)
	elseif (APPLE)
		MESSAGE(STATUS "Detected GNU on apple (C++ __GNUC__ __APPLE__)")
		set(APPLE 1)
	else()
		MESSAGE(STATUS "Detected GNU on unix (C++ __GNUC__)")
	endif()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD              11)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)

if (CLANG)
	set(CMAKE_MACOSX_BUNDLE TRUE)
	set(CMAKE_MACOSX_RPATH  TRUE)
endif()

if (CMAKE_CONFIGURATION_TYPES)
	set(ConfigName         $<CONFIG>)
	set(IntConfigName      ${CMAKE_CFG_INTDIR})
else()
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
		set(CMAKE_BUILD_TYPE "Release")
	endif()
	set(ConfigName         ${CMAKE_BUILD_TYPE})
	set(IntConfigName      ${CMAKE_BUILD_TYPE})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/lib)

set (CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE STRING "CMAKE_INSTALL_PREFIX" FORCE)

macro(DisableWarning Sources)
	if (MSVC)
		set_source_files_properties(${Sources} PROPERTIES COMPILE_FLAGS "/W0")
	else()
		set_source_files_properties(${Sources} PROPERTIES COMPILE_FLAGS "-w")
	endif()
endmacro()

option(VISUS_MINIMAL OFF)
MESSAGE(STATUS "VISUS_MINIMAL  ${VISUS_MINIMAL}")

if (VISUS_MINIMAL)

	file(GLOB cryptlite_Sources Libs/Kernel/src/cryptlite/*.c)
	file(GLOB dtl_Sources       Libs/Kernel/src/dtl/*.c)
	file(GLOB lz4_Sources       Libs/Kernel/src/lz4/*.c)
	file(GLOB tinyxml_Sources   Libs/Kernel/src/tinyxml/*.cpp)
	file(GLOB zfp_Sources       Libs/Kernel/src/zfp/*.cpp)
	file(GLOB zlib_Sources      Libs/Kernel/src/zlib/*.c)
	file(GLOB KernelSources     Libs/Kernel/src/*.cpp)
	file(GLOB Db_Sources        Libs/Db/src/*.cpp)

	DisableWarning(${cryptlite_Sources})
	DisableWarning(${dtl_Sources})
	DisableWarning(${lz4_Sources})
	DisableWarning(${tinyxml_Sources})
	DisableWarning(${zfp_Sources})	
	DisableWarning(${zlib_Sources})		
			
	IF (CLANG)
		file(GLOB KernelSources_MM Libs/Kernel/src/*.mm)
	endif()	
		
	add_library(VisusIO STATIC 		
		${cryptlite_Sources} 
		${dtl_Sources} 
		${lz4_Sources} 
		${tinyxml_Sources} 
		${zfp_Sources}
		${zlib_Sources}
		${KernelSources} 
		${KernelSources_MM} 
		${Db_Sources}
	)
		
	target_compile_definitions(VisusIO  PUBLIC  VISUS_STATIC_KERNEL_LIB=1 VISUS_STATIC_DB_LIB=1)
	target_include_directories(VisusIO  
		PUBLIC  ${CMAKE_SOURCE_DIR}/Libs/Kernel/include
		PUBLIC  ${CMAKE_SOURCE_DIR}/Libs/Db/include
		PRIVATE ${CMAKE_SOURCE_DIR}/Libs/Kernel/src
		PRIVATE ${CMAKE_SOURCE_DIR}/ExternalLibs)

	if (WIN32)
		if (MSVC)
			target_compile_options(VisusIO PRIVATE /bigobj)
		endif()
		target_compile_options(VisusIO PRIVATE -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
		target_link_libraries(VisusIO  PRIVATE Psapi.lib Iphlpapi.lib DbgHelp.lib Ws2_32.lib advapi32.lib)
	elseif (CLANG)
		target_compile_options(VisusIO PRIVATE  -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
		target_link_libraries(VisusIO  PRIVATE "-framework CoreFoundation" "-framework Foundation")
		set_target_properties(VisusIO  PROPERTIES XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME "NO")  # fixes problem of "_objc_readClassPair" missing
	else()
		target_compile_options(VisusIO PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1 -Wno-attributes -Wno-deprecated-declarations)
		target_link_libraries(VisusIO  PRIVATE dl rt util pthread)
	endif()


	INSTALL(DIRECTORY Libs/Kernel/include/Visus   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Kernel/)
	INSTALL(DIRECTORY Libs/Db/include/Visus       DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Db/)

else()

	set(VISUS_DEFAULT_NET      ON)
	set(VISUS_DEFAULT_IMAGE    ON)
	set(VISUS_DEFAULT_XIDX     ON) # not really used?
	set(VISUS_DEFAULT_DATAFLOW ON)
	set(VISUS_DEFAULT_PYTHON   ON)
	set(VISUS_DEFAULT_GUI      ON)
	set(VISUS_DEFAULT_MODVISUS OFF)
	set(VISUS_DEFAULT_OSPRAY   OFF)

	if(EXISTS "${CMAKE_SOURCE_DIR}/Libs/slamcpp/slam.cpp")
		set(VISUS_DEFAULT_SLAM 1)
	endif()

	if(EXISTS "${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/README.txt")
		set(VISUS_DEFAULT_OSPRAY ON)
	endif()
	

	if (NOT APPLE AND NOT WIN32)
		set(VISUS_DEFAULT_MODVISUS ON)
	endif()

	option(VISUS_NET      "Enable VISUS_NET"      ${VISUS_DEFAULT_NET})
	option(VISUS_IMAGE    "Enable VISUS_IMAGE"    ${VISUS_DEFAULT_IMAGE})
	option(VISUS_XIDX     "Enable VISUS_XIDX"     ${VISUS_DEFAULT_XIDX})
	option(VISUS_DATAFLOW "Enable VISUS_DATAFLOW" ${VISUS_DEFAULT_DATAFLOW})
	option(VISUS_PYTHON   "Enable VISUS_PYTHON"   ${VISUS_DEFAULT_PYTHON})
	option(VISUS_GUI      "Enable VISUS_GUI"      ${VISUS_DEFAULT_GUI})
	option(VISUS_MODVISUS "Enable VISUS_MODVISUS" ${VISUS_DEFAULT_MODVISUS})
	option(VISUS_SLAM     "Enable VISUS_SLAM"     ${VISUS_DEFAULT_SLAM})
	option(VISUS_OSPRAY   "Enable VISUS_OSPRAY"   ${VISUS_DEFAULT_OSPRAY})

	MESSAGE(STATUS "VISUS_NET      ${VISUS_NET}")
	MESSAGE(STATUS "VISUS_IMAGE    ${VISUS_IMAGE}")
	MESSAGE(STATUS "VISUS_XIDX     ${VISUS_XIDX}")
	MESSAGE(STATUS "VISUS_DATAFLOW ${VISUS_DATAFLOW}")
	MESSAGE(STATUS "VISUS_PYTHON   ${VISUS_PYTHON}")
	MESSAGE(STATUS "VISUS_GUI      ${VISUS_GUI}")
	MESSAGE(STATUS "VISUS_MODVISUS ${VISUS_MODVISUS}")
	MESSAGE(STATUS "VISUS_SLAM     ${VISUS_SLAM}")
	MESSAGE(STATUS "VISUS_OSPRAY   ${VISUS_OSPRAY}")

	include(FindPackageHandleStandardArgs)

	if (VISUS_GUI)
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
	endif()

	if (VISUS_PYTHON)
		find_package(Python COMPONENTS Interpreter Development REQUIRED)
	endif()

	if (VISUS_OSPRAY)
		set(OSPCOMMON_TBB_ROOT "${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/tbb")
		find_package(ospray 2.0   REQUIRED HINTS ${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/ospray/lib/cmake/ospray-2.1.1)
		find_package(ospcommon    REQUIRED HINTS ${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/ospcommon/lib/cmake/ospcommon-1.3.0)
	endif()

	add_subdirectory(ExternalLibs)
	add_subdirectory(Libs)
	add_subdirectory(Executable)

endif()

# install step
if (1)

	if(VISUS_GUI)
		set(__qt_ver__ ${CMAKE_BINARY_DIR}/QT_VERSION)
		file(GENERATE OUTPUT ${__qt_ver__} CONTENT "${Qt5Core_VERSION_STRING}")
		INSTALL(FILES ${__qt_ver__} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	endif()

	INSTALL(FILES     LICENSE                                 DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(FILES     README.md                               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(FILES     OpenVisusConfig.cmake                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/lib/cmake/OpenVisus/)
	INSTALL(FILES     datasets/visus.config                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(DIRECTORY datasets                                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)

	INSTALL(FILES     Samples/simple_query/CMakeLists.txt     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_query/)
	INSTALL(FILES     Samples/simple_query/main.cpp           DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_query/)
	INSTALL(FILES     Samples/simple_viewer2d/CMakeLists.txt  DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_viewer2d/)
	INSTALL(FILES     Samples/simple_viewer2d/main.cpp        DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_viewer2d/)


	if (VISUS_PYTHON)
		FILE(GLOB PyFiles Libs/swig/*.py)
		INSTALL(FILES     ${PyFiles}                          DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
		INSTALL(DIRECTORY Samples/jupyter                     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/)
		INSTALL(DIRECTORY Samples/python                      DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/)
	endif()

	if (WIN32)

		# copy  windows *.dll
		include(InstallRequiredSystemLibraries)
		INSTALL(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/bin/)

		# install windows ospray depedencies
		if (VISUS_OSPRAY)
			get_filename_component(OSPRAY_BINDIR "${ospray_DIR}/../../../bin"  REALPATH)
			FILE(GLOB OSPRAY_DLLS ${OSPRAY_BINDIR}/*.dll)
			INSTALL(FILES ${OSPRAY_DLLS} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/bin)
		endif()

	endif()

	INSTALL(DIRECTORY Copyrights                              DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(DIRECTORY Libs/Kernel/include/Visus               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Kernel/)
	INSTALL(DIRECTORY Libs/XIdx/include/Visus                 DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/XIdx/)
	INSTALL(DIRECTORY Libs/Db/include/Visus                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Db/)
	INSTALL(DIRECTORY Libs/Dataflow/include/Visus             DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Dataflow/)
	INSTALL(DIRECTORY Libs/Nodes/include/Visus                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Nodes/)
	INSTALL(DIRECTORY Libs/Gui/include/Visus                  DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Gui/)
endif()



