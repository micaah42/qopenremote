cmake_minimum_required(VERSION 3.22)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

enable_testing(true)

function(qopenremote_add_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    target_link_libraries(${TEST_NAME} PRIVATE Qt::Test qopenremote)
endfunction()

qopenremote_add_test(qobjectregistry-test)
qopenremote_add_test(json-test)

