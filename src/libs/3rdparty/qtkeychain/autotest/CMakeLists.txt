include(ECMAddTests)

find_package(Qt${QT_MAJOR_VERSION} CONFIG COMPONENTS Core Test REQUIRED)

ecm_add_tests(basic.cpp LINK_LIBRARIES ${QTKEYCHAIN_TARGET_NAME} Qt${QT_MAJOR_VERSION}::Test)
set_property(TARGET basic PROPERTY AUTOMOC ON)

if(EMSCRIPTEN)
    target_link_options(basic PRIVATE "-sASYNCIFY")
endif()
