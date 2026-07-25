cmake_minimum_required(VERSION 3.28)
project(antennavision)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# static runtime linkage
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(NOT MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()

include(FetchContent)


######### Enable CCache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "CCache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "CCache not found. Proceeding with standard compilation.")
endif()
#########


######### NumCpp
set(NUMCPP_NO_USE_BOOST ON CACHE BOOL "Don't use the boost libraries" FORCE)
FetchContent_Declare(
        NumCpp
        GIT_REPOSITORY https://github.com/dpilger26/NumCpp.git
        GIT_TAG        Version_2.16.1
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(NumCpp)
#########


######### magic_enum
FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG        v0.9.8
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(magic_enum)
#########


######### nlopt
set(NLOPT_PYTHON OFF CACHE INTERNAL "")
set(NLOPT_GUILE OFF CACHE INTERNAL "")
set(NLOPT_MATLAB OFF CACHE INTERNAL "")
set(NLOPT_TESTS OFF CACHE INTERNAL "")
FetchContent_Declare(
        nlopt
        GIT_REPOSITORY https://github.com/stevengj/nlopt.git
        GIT_TAG        v2.11.0
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlopt)
#########


######### Cephes
FetchContent_Declare(
        Cephes
        GIT_REPOSITORY https://github.com/jeremybarnes/cephes.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
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
if(MSVC)
    target_compile_options(CephesSiCi PRIVATE /w)
else()
    target_compile_options(CephesSiCi PRIVATE -w)
endif()
#########


######### ansi_color
FetchContent_Declare(
        ansi_color
        URL      https://github.com/devfix/ansi_color/archive/refs/heads/main.zip
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
        GIT_TAG        v3.12.0
        GIT_SHALLOW    TRUE
        SYSTEM# <--- This treats all exported headers as system include paths, supress all deprecation warnings
)
FetchContent_MakeAvailable(json)
#########


######### exprtk
FetchContent_Declare(
        exprtk
        GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
        GIT_TAG        66883f0ddb034371ef38f2799f772c05bc904571
        GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(exprtk)
if(NOT TARGET exprtk)
    add_library(exprtk INTERFACE)
    target_include_directories(exprtk INTERFACE ${exprtk_SOURCE_DIR})
endif()
#########


######### Catch2
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.15.2
        GIT_SHALLOW    TRUE
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
)
target_compile_definitions(antennavision PRIVATE JSON_DIAGNOSTICS=0)
#########


######### target: testrunner
add_executable(testrunner ${SOURCE_FILES} ${TEST_FILES})
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
