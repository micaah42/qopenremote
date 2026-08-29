cmake_minimum_required(VERSION 3.22)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Quick QuickControls2 Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Quick QuickControls2 Test)

enable_testing(true)

function(qopenremote_add_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    target_link_libraries(${TEST_NAME} PRIVATE Qt::Test Qt::Quick Qt::QuickControls2 qopenremote)
    target_compile_definitions(${TEST_NAME} PRIVATE QOPENREMOTE_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()

qopenremote_add_test(qobjectregistry-test)
qopenremote_add_test(json-test)
qopenremote_add_test(quicktestengine-test)

add_executable(qopenremote-ws-test-server websocket-test-server.cpp)
target_link_libraries(qopenremote-ws-test-server PRIVATE Qt::Core Qt::WebSockets qopenremote)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(
    NAME qopenremote-ws-pytest
    COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/test_qopenremote_ws.py
)
set_tests_properties(qopenremote-ws-pytest PROPERTIES
    ENVIRONMENT "QOPENREMOTE_TEST_SERVER=$<TARGET_FILE:qopenremote-ws-test-server>"
)

