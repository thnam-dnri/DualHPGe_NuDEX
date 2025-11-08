# CMakeLists.txt for Dual HPGe Detector Simulation

cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(DualHPGe_NuDEX)

# Find required packages
find_package(Geant4 REQUIRED ui_all vis_all)
include(${Geant4_USE_FILE})

# Find ROOT
find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/NuDEX/include)
include_directories(${Geant4_INCLUDE_DIR})

# Determine C++ standard (override via env HPGE_CXX_STANDARD or CMakeLists.local.cmake)
set(HPGE_CXX_STANDARD 14)
if(DEFINED ENV{HPGE_CXX_STANDARD})
    set(HPGE_CXX_STANDARD $ENV{HPGE_CXX_STANDARD})
endif()
set(_HPGE_LOCAL_OVERRIDE "${PROJECT_SOURCE_DIR}/CMakeLists.local.cmake")
if(EXISTS "${_HPGE_LOCAL_OVERRIDE}")
    message(STATUS "Applying local overrides from ${_HPGE_LOCAL_OVERRIDE}")
    include("${_HPGE_LOCAL_OVERRIDE}")
endif()
set(CMAKE_CXX_STANDARD ${HPGE_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Locate sources and headers
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)
file(GLOB nudex_sources ${PROJECT_SOURCE_DIR}/NuDEX/src/*.cc)

# Add the executable
add_executable(DualHPGe_NuDEX HPGeDual.cc ${sources} ${headers} ${nudex_sources})

# Link against Geant4 and ROOT libraries
target_link_libraries(DualHPGe_NuDEX ${Geant4_LIBRARIES} ${ROOT_LIBRARIES})

# Ensure ROOT detects availability of std::string_view in newer libstdc++
target_compile_definitions(DualHPGe_NuDEX PRIVATE R__HAS_STD_STRING_VIEW)

# Copy macro files to build directory
set(HPGeDual_SCRIPTS
    init_vis.mac
    gui.mac
    vis.mac
    run.mac
)

foreach(_script ${HPGeDual_SCRIPTS})
    configure_file(
        ${PROJECT_SOURCE_DIR}/${_script}
        ${PROJECT_BINARY_DIR}/${_script}
        COPYONLY
    )
endforeach()

# Install the executable
install(TARGETS DualHPGe_NuDEX DESTINATION bin)
