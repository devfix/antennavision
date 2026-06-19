cmake_minimum_required(VERSION 3.28)
project(antennavision)

# Matplot++ requires C++17
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
        URL https://github.com/dpilger26/NumCpp/archive/refs/tags/Version_2.16.1.tar.gz
        URL_HASH SHA256=e543049b267ce6bc28463e45db3eaefa4e7c0944c89d709df515736bfef807b4
)
FetchContent_MakeAvailable(NumCpp)
#########


######### FmtLib
FetchContent_Declare(
        fmt
        URL https://github.com/fmtlib/fmt/archive/refs/tags/12.1.0.tar.gz
        URL_HASH SHA256=ea7de4299689e12b6dddd392f9896f08fb0777ac7168897a244a6d6085043fea
)
FetchContent_MakeAvailable(fmt)
#########


######### nlohmann/json
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_FastTests OFF CACHE INTERNAL "")
set(JSON_CompiledMode ON CACHE BOOL "" FORCE) # <--- This forces it to compile
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz
        URL_HASH SHA256=4b92eb0c06d10683f7447ce9406cb97cd4b453be18d7279320f7b2f025c10187
)
FetchContent_MakeAvailable(json)
#########


######### Catch2
FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.15.0.tar.gz
        URL_HASH SHA256=9650c55e497759cc39b977e45524bc8acb15256061c112080916ab6cb0b1ea66
)
FetchContent_MakeAvailable(Catch2)
#########


######### dplotpp
FetchContent_Declare(
        dplotpp
        GIT_REPOSITORY https://github.com/devfix/dplotpp.git
        GIT_TAG        main
)
FetchContent_MakeAvailable(dplotpp)
#########


######### exprtk
FetchContent_Declare(
        exprtk
        URL https://github.com/ArashPartow/exprtk/archive/refs/heads/master.zip
        URL_HASH SHA256=9e9dd13f1f3c841e22dceb4e7e005812f4f4c0bf37410ab822f37426c632923e
)
FetchContent_MakeAvailable(exprtk)
if(NOT TARGET exprtk)
    add_library(exprtk INTERFACE)
    target_include_directories(exprtk INTERFACE ${exprtk_SOURCE_DIR})
endif()
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
        matplot
        fmt::fmt
        nlohmann_json::nlohmann_json
        dplotpp
        exprtk
)

# Add this in your CMakeLists.txt right after target_link_libraries
target_compile_definitions(antennavision PRIVATE JSON_DIAGNOSTICS=0)
#########


######### target: testrunner
add_executable(testrunner ${SOURCE_FILES} ${TEST_FILES})
target_compile_definitions(testrunner PRIVATE NUMCPP_NO_USE_BOOST)
target_include_directories(testrunner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(testrunner PRIVATE
        NumCpp::NumCpp
        matplot
        fmt::fmt
        nlohmann_json::nlohmann_json
        dplotpp
        exprtk
        Catch2::Catch2WithMain
)

# Add this in your CMakeLists.txt right after target_link_libraries
target_compile_definitions(testrunner PRIVATE JSON_DIAGNOSTICS=0)

# Enable CTest regression testing framework integration (Optional but recommended)
include(CTest)
include(Catch)
catch_discover_tests(testrunner)
#########
