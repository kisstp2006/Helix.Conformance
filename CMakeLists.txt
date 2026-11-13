cmake_minimum_required(VERSION 3.28)
project(Helix.Conformance LANGUAGES CXX)

if(NOT COMMAND helix_add_library)
    include("${CMAKE_CURRENT_SOURCE_DIR}/External/Helix.CMake/Helix.cmake")
endif()

# The conformance suites of the engine's documented behavior, in C# and in C++, run by the player in batch mode
# without graphics on a data folder (Data/, written as a build would leave it): the C# suites are an assembly of its
# Managed/, the C++ suites a game module (a shared library of its Modules/ on the desktop, linked into the player on
# the Web, packaged in the APK). Each case logs `conformance: <lang>.<suite>.<case> ok|FAIL <detail>`;
# Tests/RunConformance.cmake checks the log against Suites/Expected.txt and the player's exit code.
helix_require(Core)
helix_require(Engine)
helix_require(ScriptCore)
include("${HELIX_BINDGEN_ROOT}/CMake/HelixBindgen.cmake")

set(suites_dir "${CMAKE_CURRENT_SOURCE_DIR}/Suites")
helix_dependency_dir(engine_dir Engine)
set(engine_source "${engine_dir}/Source")
set(engine_references "${engine_source}/Helix/Engine/Object/EntityId.h" "${engine_source}/Helix/Engine/Object/Object.h"
    "${engine_source}/Helix/Engine/GameObject/GameObject.h" "${engine_source}/Helix/Engine/Script/GameBehaviour.h")

# The C++ suites: ConformanceModule, the module the scene's C++ script assets name.
if(HELIX_PLATFORM STREQUAL "web")
    add_library(helix_conformance_module STATIC "${suites_dir}/Cpp/ConformanceScripts.cpp")
    target_link_libraries(helix_conformance_module PRIVATE Helix::Engine)
else()
    add_library(helix_conformance_module SHARED "${suites_dir}/Cpp/ConformanceScripts.cpp")
    target_link_libraries(helix_conformance_module PRIVATE Helix::EngineShared)
endif()
set_target_properties(helix_conformance_module PROPERTIES OUTPUT_NAME ConformanceModule)
target_include_directories(helix_conformance_module PRIVATE "${suites_dir}/Cpp")
helix_apply_options(helix_conformance_module)
helix_bindgen(helix_conformance_module MODULE ConformanceModule NAMESPACE Conformance KIND NativeGame
              HEADERS "${suites_dir}/Cpp/ConformanceScripts.h" REFERENCES ${engine_references} REFERENCE_NAMESPACE HelixEngine
              INCLUDE_ROOTS "${suites_dir}/Cpp" "${engine_source}" CS_OUTPUT module_cs)

# The C# suites: ConformanceSuites.dll (no "Helix." name: the player takes those for the engine's).
set(conformance_artifacts "${CMAKE_CURRENT_BINARY_DIR}/Dotnet")
set(suites_dll "${conformance_artifacts}/bin/Helix.Conformance.Suites/release/ConformanceSuites.dll")
file(GLOB suites_sources CONFIGURE_DEPENDS "${suites_dir}/CSharp/*.cs" "${suites_dir}/CSharp/*.csproj")
add_custom_command(
    OUTPUT "${suites_dll}"
    COMMAND "${HELIX_DOTNET}" build "${suites_dir}/CSharp/Helix.Conformance.Suites.csproj" -c Release --artifacts-path "${conformance_artifacts}"
            --nologo -v quiet "-p:HelixCoreModule=${HELIX_CORE_MODULE}" "-p:HelixScriptingRuntime=${HELIX_SCRIPTING_RUNTIME}"
            "-p:HelixModuleBindings=${module_cs}"
    DEPENDS ${suites_sources} "${HELIX_CORE_MODULE}" "${module_cs}"
    COMMENT "Building the C# conformance suites (with the C++ module's C# bindings)")
add_custom_target(helix_conformance_suites ALL DEPENDS "${suites_dll}")
add_dependencies(helix_conformance_suites helix_core_module)

# The data folder the player runs: Data/ with the suites (and on the desktop the engine's assemblies and the module).
set(conformance_data "${CMAKE_CURRENT_BINARY_DIR}/ConformanceData")
file(GLOB_RECURSE data_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Data/*")
set(stage_commands
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${conformance_data}"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "${conformance_data}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${conformance_data}/Managed"
    COMMAND "${CMAKE_COMMAND}" -E copy "${suites_dll}" "${conformance_data}/Managed")
if(NOT HELIX_PLATFORM STREQUAL "web" AND NOT HELIX_PLATFORM STREQUAL "android")
    # The engine's assemblies (the desktop player takes them from Managed/) and the C++ suites as a module.
    get_filename_component(runtime_dir "${HELIX_SCRIPTING_RUNTIME}" DIRECTORY)
    get_filename_component(runtime_name "${HELIX_SCRIPTING_RUNTIME}" NAME_WLE)
    list(APPEND stage_commands
        COMMAND "${CMAKE_COMMAND}" -E copy "${HELIX_CORE_MODULE}" ${HELIX_CORE_MODULE_REFERENCES} "${HELIX_SCRIPTING_RUNTIME}"
                "${runtime_dir}/${runtime_name}.runtimeconfig.json" "${runtime_dir}/${runtime_name}.deps.json" "${conformance_data}/Managed"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${conformance_data}/Modules"
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:helix_conformance_module>" "${conformance_data}/Modules")
endif()
add_custom_command(
    OUTPUT "${conformance_data}/.staged"
    ${stage_commands}
    COMMAND "${CMAKE_COMMAND}" -E touch "${conformance_data}/.staged"
    DEPENDS ${data_files} "${suites_dll}" helix_conformance_module
    COMMENT "Staging the conformance data folder")
add_custom_target(helix_conformance_data ALL DEPENDS "${conformance_data}/.staged")
add_dependencies(helix_conformance_data helix_conformance_suites helix_core_module)
if(TARGET helix_scripting_runtime)
    add_dependencies(helix_conformance_data helix_scripting_runtime)
endif()

# The player, with the suites built in where the build names them (the Web, Android).
if(HELIX_PLATFORM STREQUAL "web" OR HELIX_PLATFORM STREQUAL "android")
    set(HELIX_PLAYER_GAME_MODULES helix_conformance_module CACHE STRING "" FORCE)
    set(HELIX_PLAYER_GAME_ASSEMBLIES "${suites_dll}" CACHE STRING "" FORCE)
    set(HELIX_PLAYER_DATA "${conformance_data}" CACHE PATH "" FORCE)
endif()
helix_require(Player)
if(TARGET helix_player_web)
    add_dependencies(helix_player_web helix_conformance_suites)
endif()
if(TARGET helix_player_apk)
    add_dependencies(helix_player_apk helix_conformance_data)
endif()
# The player is a dependency's target, outside the default build: built with this one (the web app, the APK).
add_custom_target(helix_conformance_player ALL)
foreach(player IN ITEMS helix_player helix_player_web helix_player_apk helix_player_apk_check)
    if(TARGET ${player})
        add_dependencies(helix_conformance_player ${player})
    endif()
endforeach()

# The runs: the player executable (also on the coreclr starter) on the desktop, its web app in node on the Web.
if(HELIX_BUILD_TESTS AND PROJECT_IS_TOP_LEVEL)
    set(runner "${CMAKE_CURRENT_SOURCE_DIR}/Tests/RunConformance.cmake")
    set(expected "${suites_dir}/Expected.txt")
    if(HELIX_PLATFORM STREQUAL "windows" OR HELIX_PLATFORM STREQUAL "linux")
        add_dependencies(helix_conformance_data helix_player)
        foreach(runtime IN ITEMS hostfxr coreclr)
            add_test(NAME helix_conformance_${runtime}
                     COMMAND "${CMAKE_COMMAND}" "-DPLAYER=$<TARGET_FILE:helix_player>" "-DDATA=${conformance_data}" "-DEXPECTED=${expected}"
                             "-DRUNTIME=${runtime}" "-DLOG=${CMAKE_CURRENT_BINARY_DIR}/conformance-${runtime}.log" -P "${runner}")
            # The .NET runtime is never unloaded: LeakSanitizer is told about its modules (as in Helix.Scripting).
            if(HELIX_ENABLE_ASAN AND HELIX_PLATFORM STREQUAL "linux")
                helix_dependency_dir(scripting_dir Scripting)
                set_tests_properties(helix_conformance_${runtime} PROPERTIES
                                     ENVIRONMENT "LSAN_OPTIONS=suppressions=${scripting_dir}/Tests/Support/LeakSuppressions.txt")
            endif()
        endforeach()
    elseif(HELIX_PLATFORM STREQUAL "web" AND TARGET helix_player_web)
        add_test(NAME helix_conformance_web
                 COMMAND "${CMAKE_COMMAND}" "-DNODE=${HELIX_EMSCRIPTEN_NODE}"
                         "-DPLAYER=${CMAKE_CURRENT_BINARY_DIR}/External/Helix.Player/helix_player.Web/publish/wwwroot/main.mjs"
                         "-DDATA=${conformance_data}" "-DEXPECTED=${expected}" -P "${runner}")
    endif()
    # The 10k Update dispatch of C# and C++ scripts.
    if(HELIX_PLATFORM STREQUAL "windows" OR HELIX_PLATFORM STREQUAL "linux")
        helix_add_benchmark_suite(helix_conformance_bench DEPS Helix::EngineShared)
        if(TARGET helix_conformance_bench)
            helix_bindgen(helix_conformance_bench MODULE ConformanceBench NAMESPACE ConformanceBench KIND NativeGame
                          HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks/BenchScripts.h" REFERENCES ${engine_references}
                          REFERENCE_NAMESPACE HelixEngine INCLUDE_ROOTS "${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks" "${engine_source}")
            add_dependencies(helix_conformance_bench helix_conformance_suites)
            target_include_directories(helix_conformance_bench PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks")
            target_compile_definitions(helix_conformance_bench PRIVATE "HX_RUNTIME_ASSEMBLY=\"${HELIX_SCRIPTING_RUNTIME}\""
                                       "HX_CORE_MODULE=\"${HELIX_CORE_MODULE}\"" "HX_SUITES=\"${suites_dll}\"")
        endif()
    endif()
endif()
