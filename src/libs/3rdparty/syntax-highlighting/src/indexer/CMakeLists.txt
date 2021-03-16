# when cross compiling, use either the executable offered or try to cross-compile it in place

if(CMAKE_CROSSCOMPILING AND KATEHIGHLIGHTINGINDEXER_EXECUTABLE)
    add_executable(katehighlightingindexer IMPORTED GLOBAL)
    set_target_properties(katehighlightingindexer PROPERTIES IMPORTED_LOCATION ${KATEHIGHLIGHTINGINDEXER_EXECUTABLE})
elseif(CMAKE_CROSSCOMPILING)
    if (NOT KF5_HOST_TOOLING)
        message(FATAL_ERROR "Please provide a prefix with a native Qt build and pass -DKF5_HOST_TOOLING=path")
    endif()

    # search native tooling prefix
    string(FIND ${KF5_HOST_TOOLING} /lib idx)
    string(SUBSTRING ${KF5_HOST_TOOLING} 0 ${idx} NATIVE_PREFIX)
    message(STATUS "Building katehighlightingindexer against ${NATIVE_PREFIX}")

    include(ExternalProject)
    ExternalProject_Add(native_katehighlightingindexer
                        SOURCE_DIR ${CMAKE_SOURCE_DIR}
                        CMAKE_ARGS -DKSYNTAXHIGHLIGHTING_USE_GUI=OFF
                            -DECM_DIR=${ECM_DIR} -DCMAKE_PREFIX_PATH=${NATIVE_PREFIX}
                            -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}
                        INSTALL_COMMAND ""
                    )
    add_executable(katehighlightingindexer IMPORTED GLOBAL)
    add_dependencies(katehighlightingindexer native_katehighlightingindexer)
    set_target_properties(katehighlightingindexer PROPERTIES IMPORTED_LOCATION
        ${CMAKE_CURRENT_BINARY_DIR}/native_katehighlightingindexer-prefix/src/native_katehighlightingindexer-build/bin/katehighlightingindexer)
else()
    # host build
    add_executable(katehighlightingindexer katehighlightingindexer.cpp ../lib/worddelimiters.cpp)
    if(Qt5XmlPatterns_FOUND)
        target_link_libraries(katehighlightingindexer Qt5::XmlPatterns)
    else()
        target_link_libraries(katehighlightingindexer Qt5::Core)
    endif()
endif()
