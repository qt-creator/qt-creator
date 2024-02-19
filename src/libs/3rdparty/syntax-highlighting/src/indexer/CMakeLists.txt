# when cross compiling, use either the executable offered or try to cross-compile it in place

if(CMAKE_CROSSCOMPILING AND KATEHIGHLIGHTINGINDEXER_EXECUTABLE)
    add_executable(katehighlightingindexer IMPORTED GLOBAL)
    set_target_properties(katehighlightingindexer PROPERTIES IMPORTED_LOCATION ${KATEHIGHLIGHTINGINDEXER_EXECUTABLE})
elseif(CMAKE_CROSSCOMPILING)
    include(ECMQueryQt)
    ecm_query_qt(NATIVE_PREFIX QT_HOST_PREFIX)
    message(STATUS "Building katehighlightingindexer against ${NATIVE_PREFIX}")

    include(ExternalProject)
    ExternalProject_Add(native_katehighlightingindexer
                        SOURCE_DIR ${CMAKE_SOURCE_DIR}
                        CMAKE_ARGS -DKSYNTAXHIGHLIGHTING_USE_GUI=OFF
                            -DECM_DIR=${ECM_DIR} -DCMAKE_PREFIX_PATH=${NATIVE_PREFIX}
                            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                            -DQT_MAJOR_VERSION=6
                        INSTALL_COMMAND ""
                        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/native_katehighlightingindexer-prefix/src/native_katehighlightingindexer-build/bin/katehighlightingindexer
                    )
    add_executable(katehighlightingindexer IMPORTED GLOBAL)
    add_dependencies(katehighlightingindexer native_katehighlightingindexer)
    set_target_properties(katehighlightingindexer PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_BINARY_DIR}/native_katehighlightingindexer-prefix/src/native_katehighlightingindexer-build/bin/katehighlightingindexer)
else()
    # host build
    add_executable(katehighlightingindexer katehighlightingindexer.cpp ../lib/worddelimiters.cpp)
    ecm_mark_nongui_executable(katehighlightingindexer)
    if(XercesC_FOUND)
        add_definitions(-DHAS_XERCESC)
        kde_target_enable_exceptions(katehighlightingindexer PRIVATE)
        target_link_libraries(katehighlightingindexer Qt6::Core XercesC::XercesC)
    else()
        target_link_libraries(katehighlightingindexer Qt6::Core)
    endif()
endif()
