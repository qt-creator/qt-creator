if (NOT TARGET QmlJSTools)
  return()
endif()

option(BUILD_TEST_TST_CONNECTION_EDITOR "Builds Test Connection View" OFF)

add_subdirectory(codemodel)
add_subdirectory(persistenttrie)
add_subdirectory(qmldesigner)
add_subdirectory(qmleditor)
add_subdirectory(qmljssimplereader)
add_subdirectory(qmljsutils)
add_subdirectory(qrcparser)
add_subdirectory(reformatter)

if ({BUILD_TEST_TST_CONNECTION_EDITOR})
    add_subdirectory(connectioneditor)
endif()
