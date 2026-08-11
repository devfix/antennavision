cmake_minimum_required(VERSION 3.28)
project(antennavision)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
include(CheckIPOSupported)

# Link-Time-Optimization
check_ipo_supported(RESULT ipo_supported)
if (ipo_supported)
    message(STATUS "[AntennaVision] IPO detected")

    # Enable LTO automatically whenever CMAKE_BUILD_TYPE is "Release" or "RelWithDebInfo"
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
else ()
    message(STATUS "[AntennaVision] IPO not found")
endif ()


option(BUILD_ALL_SHARED "[AntennaVision] Build all internal libraries as shared and place next to executable" OFF)
option(ANTENNAVISION_MULTI_THREADED "[AntennaVision] Disable multi-threaded computation" ON)


# === static runtime linkage ===
# since antennavision used latest c++ features that might be missing on other systems,
# we link these libraries directly into the executable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


if (NOT MSVC)
    # Separate functions and data into individual sections during compilation
    add_compile_options(-ffunction-sections -fdata-sections)

    # Tell the linker to drop any unreferenced sections
    add_link_options(-Wl,--gc-sections)
endif ()


# === static libstdc++ & libgcc linkage ===
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_link_options(-static-libstdc++ -static-libgcc)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
    add_link_options(-static-libstdc++) # Clang on Linux/macOS
endif ()


# === static/shared library linkage ===
if (BUILD_ALL_SHARED)
    message(STATUS "[AntennaVision] Build with shared linkage")

    # Force non-interface targets to build as SHARED
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Build all libraries as shared" FORCE)

    #  Output ALL targets (executables & .so files) into the same bin/ directory
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    #  Configure RPATH so the linker finds .so files next to the binary at runtime
    if (NOT WIN32)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
        set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif ()
else ()
    message(STATUS "[AntennaVision] Build with static linkage")

    #    if(NOT MSVC)
    #        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
    #    endif()

    # static dependency linkage
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif ()


######### Enable CCache if available
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    message(STATUS "[AntennaVision] CCache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else ()
    message(STATUS "[AntennaVision] CCache not found. Proceeding with standard compilation.")
endif ()
#########


######### CLI11
FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.7.2
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CLI11)
######### NumCpp


######### NumCpp
set(NUMCPP_NO_USE_BOOST ON CACHE BOOL "Don't use the boost libraries" FORCE)
FetchContent_Declare(
        NumCpp
        GIT_REPOSITORY https://github.com/dpilger26/NumCpp.git
        GIT_TAG Version_2.16.1
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(NumCpp)
#########


######### magic_enum
FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.8
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(magic_enum)
#########


######### nlopt
set(NLOPT_PYTHON OFF CACHE BOOL "" FORCE) # Good practice so it doesn't try to build python bindings statically
set(NLOPT_GUILE OFF CACHE INTERNAL "")
set(NLOPT_MATLAB OFF CACHE INTERNAL "")
set(NLOPT_TESTS OFF CACHE INTERNAL "")
if (NOT BUILD_ALL_SHARED)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(NLOPT_SHARED OFF CACHE BOOL "" FORCE)
    set(NLOPT_GUI OFF CACHE BOOL "" FORCE)
endif ()
FetchContent_Declare(
        nlopt
        GIT_REPOSITORY https://github.com/stevengj/nlopt.git
        GIT_TAG v2.11.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlopt)
#########


######### Cephes
FetchContent_Declare(
        Cephes
        GIT_REPOSITORY https://github.com/jeremybarnes/cephes.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Cephes)
add_library(CephesSiCi STATIC
        ${cephes_SOURCE_DIR}/misc/sici.c
        ${cephes_SOURCE_DIR}/cmath/polevl.c
        ${cephes_SOURCE_DIR}/cmath/const.c
)
# Note: Cephes doesn't use an 'include/' directory; headers live right next to the source files
#target_include_directories(CephesSiCi PUBLIC ${cephes_SOURCE_DIR}/misc)

# Disable old compiler warnings for Cephes (since it's legacy C code)
if (MSVC)
    target_compile_options(CephesSiCi PRIVATE /w)
else ()
    target_compile_options(CephesSiCi PRIVATE -w)
endif ()
#########


######### ansi_color
FetchContent_Declare(
        ansi_color
        URL https://github.com/devfix/ansi_color/archive/refs/heads/main.zip
        URL_HASH SHA256=cdebb0f9e5f48d7e61599247acb8dbf6210bf55e39a19e77446fdde50d1dd7f7
)
FetchContent_MakeAvailable(ansi_color)
add_library(ansi_color INTERFACE)
target_include_directories(ansi_color INTERFACE ${ansi_color_SOURCE_DIR})
#########


######### nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_FastTests OFF CACHE INTERNAL "")
set(JSON_CompiledMode ON CACHE BOOL "" FORCE) # <--- This forces it to compile
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

# Apply SYSTEM to the REAL target name (without ::)
if (TARGET nlohmann_json)
    set_target_properties(nlohmann_json PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:nlohmann_json,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif ()
#########


######### exprtk
FetchContent_Declare(
        exprtk
        GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
        GIT_TAG 66883f0ddb034371ef38f2799f772c05bc904571
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(exprtk)
if (NOT TARGET exprtk)
    add_library(exprtk INTERFACE)
    target_include_directories(exprtk INTERFACE ${exprtk_SOURCE_DIR})
endif ()
#########


######### Catch2
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.15.2
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)
#########


file(GLOB_RECURSE SOURCE_FILES
        CONFIGURE_DEPENDS
        "src/*.cpp"
        "src/*.hpp"
        "include/*.hpp"
)
file(GLOB_RECURSE TEST_FILES
        CONFIGURE_DEPENDS
        "test/*.cpp"
        "test/*.hpp"
)

######### target: antennavision
add_executable(antennavision main.cpp ${SOURCE_FILES})
if(ANTENNAVISION_MULTI_THREADED)
    target_compile_definitions(antennavision PRIVATE ANTENNAVISION_MULTI_THREADED=1)
else()
    target_compile_definitions(antennavision PRIVATE ANTENNAVISION_MULTI_THREADED=0)
endif()
target_compile_definitions(antennavision PRIVATE NUMCPP_NO_USE_BOOST)
target_include_directories(antennavision PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(antennavision PRIVATE
        NumCpp::NumCpp
        magic_enum::magic_enum
        nlopt
        CephesSiCi
        ansi_color
        nlohmann_json::nlohmann_json
        exprtk
        CLI11::CLI11
)
target_compile_definitions(antennavision PRIVATE JSON_DIAGNOSTICS=0)
#########


######### target: testrunner
add_executable(testrunner ${SOURCE_FILES} ${TEST_FILES})
if(ANTENNAVISION_MULTI_THREADED)
    target_compile_definitions(testrunner PRIVATE ANTENNAVISION_MULTI_THREADED=1)
else()
    target_compile_definitions(testrunner PRIVATE ANTENNAVISION_MULTI_THREADED=0)
endif()
target_compile_definitions(testrunner PRIVATE NUMCPP_NO_USE_BOOST)
target_include_directories(testrunner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(testrunner PRIVATE
        NumCpp::NumCpp
        magic_enum::magic_enum
        nlopt
        CephesSiCi
        ansi_color
        nlohmann_json::nlohmann_json
        exprtk
        Catch2::Catch2WithMain
)
target_compile_definitions(testrunner PRIVATE JSON_DIAGNOSTICS=0)

# Enable CTest regression testing framework integration (Optional but recommended)
include(CTest)
include(Catch)
catch_discover_tests(testrunner)
#########
