cmake_minimum_required(VERSION 3.25)
project(HeadsetDesk VERSION 0.2.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Headset Desk v1 targets Windows 11 only.")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(HEADSET_DESK_BUILD_DESKTOP "Build the Qt desktop application" OFF)
if(HEADSET_DESK_BUILD_DESKTOP)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include(CTest)
find_package(Threads REQUIRED)

set(UPSTREAM "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sony-device-center")
add_subdirectory("${UPSTREAM}/libs/sony-transport")
target_link_libraries(sony-transport PRIVATE rpcrt4)
target_compile_definitions(sony-transport PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
add_subdirectory("${UPSTREAM}/libs/sony-protocol")

add_library(headset-desk-core
    core/src/DeviceProfile.cpp
    core/src/DeviceDiscovery.cpp
    core/src/ReadOnlyTransport.cpp
    core/src/DeviceSession.cpp
    core/src/Simulation.cpp
)
target_include_directories(headset-desk-core PUBLIC core/include)
target_include_directories(headset-desk-core PRIVATE "${UPSTREAM}/Client" "${UPSTREAM}/Client/windows")
target_link_libraries(headset-desk-core PUBLIC sony-protocol sony-transport)
if(MSVC)
    target_compile_options(headset-desk-core PRIVATE /W4 /permissive-)
else()
    target_compile_options(headset-desk-core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Internal hardware validation tool.
add_executable(headset-desk-diagnostics apps/diagnostics/main.cpp)
target_link_libraries(headset-desk-diagnostics PRIVATE headset-desk-core)
if(MINGW)
    target_link_options(headset-desk-diagnostics PRIVATE -static)
endif()

if(HEADSET_DESK_BUILD_DESKTOP)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Network)
    qt_standard_project_setup(REQUIRES 6.8)
    qt_add_executable(headset-desk WIN32
        apps/desktop/main.cpp apps/desktop/DeviceController.cpp
        apps/desktop/DeviceController.h apps/desktop/DeviceWorker.cpp
        apps/desktop/DeviceWorker.h apps/desktop/SingleInstance.cpp
        apps/desktop/SingleInstance.h)
    target_link_libraries(headset-desk PRIVATE headset-desk-core
        Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Network psapi)
    set_source_files_properties(apps/desktop/qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
    set_source_files_properties(apps/desktop/qml/Colors.qml PROPERTIES QT_RESOURCE_ALIAS Colors.qml)
    set_source_files_properties(apps/desktop/qml/Section.qml PROPERTIES QT_RESOURCE_ALIAS Section.qml)
    qt_add_qml_module(headset-desk URI HeadsetDesk VERSION 1.0
        QML_FILES apps/desktop/qml/Main.qml apps/desktop/qml/Colors.qml
        apps/desktop/qml/Section.qml)
    if(BUILD_TESTING)
        find_package(Qt6 REQUIRED COMPONENTS Test)
        qt_add_executable(headset-desk-desktop-tests tests/DesktopTests.cpp
            apps/desktop/DeviceController.cpp apps/desktop/DeviceController.h
            apps/desktop/DeviceWorker.cpp apps/desktop/DeviceWorker.h
            apps/desktop/SingleInstance.cpp apps/desktop/SingleInstance.h)
        target_include_directories(headset-desk-desktop-tests PRIVATE apps/desktop)
        target_link_libraries(headset-desk-desktop-tests PRIVATE headset-desk-core
            Qt6::Test Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Network)
        target_compile_definitions(headset-desk-desktop-tests PRIVATE HEADSET_DESK_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/apps/desktop/qml")
        add_test(NAME desktop-offline COMMAND headset-desk-desktop-tests)
        set_tests_properties(desktop-offline PROPERTIES TIMEOUT 90)
    endif()
endif()

if(BUILD_TESTING)
    find_package(Catch2 3 QUIET)
    if(NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(Catch2
            URL https://codeload.github.com/catchorg/Catch2/tar.gz/refs/tags/v3.8.1
            URL_HASH SHA256=18b3f70ac80fccc340d8c6ff0f339b2ae64944782f8d2fca2bd705cf47cadb79
        )
        FetchContent_MakeAvailable(Catch2)
    endif()
    add_executable(upstream-protocol-tests
        "${UPSTREAM}/tests/protocol/FrameCodecTests.cpp"
        "${UPSTREAM}/tests/protocol/SonyProtocolSessionTests.cpp"
        "${UPSTREAM}/tests/protocol/ProtocolV1Tests.cpp"
        "${UPSTREAM}/tests/protocol/ProtocolV2Tests.cpp"
        "${UPSTREAM}/tests/protocol/DeviceProfileTests.cpp"
        "${UPSTREAM}/tests/protocol/CapabilityDiscoveryTests.cpp"
        "${UPSTREAM}/tests/protocol/DeviceStateTests.cpp"
        "${UPSTREAM}/tests/protocol/DeviceEventDispatcherTests.cpp"
        "${UPSTREAM}/tests/protocol/SonyErrorTests.cpp"
        "${UPSTREAM}/tests/transport/FakeTransportTests.cpp"
        "${UPSTREAM}/tests/transport/LoggerTests.cpp"
        "${UPSTREAM}/tests/transport/SonyDeviceFilterTests.cpp"
    )
    target_link_libraries(upstream-protocol-tests PRIVATE sony-protocol Catch2::Catch2WithMain)
    add_executable(headset-desk-core-tests tests/CoreTests.cpp tests/ProtocolSafetyTests.cpp tests/HardwareCaptureTests.cpp)
    target_compile_definitions(headset-desk-core-tests PRIVATE HEADSET_DESK_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    target_include_directories(headset-desk-core-tests PRIVATE "${UPSTREAM}/Client" "${UPSTREAM}/Client/windows" "${UPSTREAM}/tests/protocol")
    target_link_libraries(headset-desk-core-tests PRIVATE headset-desk-core Catch2::Catch2WithMain)
    add_test(NAME upstream-protocol COMMAND upstream-protocol-tests)
    add_test(NAME headset-desk-core COMMAND headset-desk-core-tests)
    add_test(NAME diagnostic-simulation COMMAND headset-desk-diagnostics --simulate xm5)
    add_test(NAME diagnostic-ch720n-simulation COMMAND headset-desk-diagnostics --simulate ch720n)
    add_test(NAME diagnostic-rejects-implicit-hardware COMMAND headset-desk-diagnostics --device-index 1)
    add_test(NAME diagnostic-rejects-mixed-modes COMMAND headset-desk-diagnostics --simulate xm5 --devices)
    set_tests_properties(diagnostic-rejects-implicit-hardware diagnostic-rejects-mixed-modes PROPERTIES WILL_FAIL TRUE TIMEOUT 5)
    set_tests_properties(upstream-protocol headset-desk-core PROPERTIES TIMEOUT 90)
    set_tests_properties(diagnostic-simulation diagnostic-ch720n-simulation PROPERTIES TIMEOUT 15)
endif()
