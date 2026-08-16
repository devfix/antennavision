######### CLI11
FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v2.7.2
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CLI11)
#########


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
