cmake_minimum_required(VERSION 3.19)

@if '%{UseJni}' === 'true'
project(%{ProjectName} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
@else
project(%{ProjectName} LANGUAGES NONE)
@endif

find_package(Java REQUIRED COMPONENTS Development)
@if '%{UseJni}' === 'true'
# Only jni.h is needed here. find_package(JNI REQUIRED) would demand the AWT
# and JVM libraries as well, which a headless JDK does not ship.
find_package(JNI QUIET)
if(NOT JNI_INCLUDE_DIRS)
    message(FATAL_ERROR "The JNI headers were not found.")
endif()
@endif
include(UseJava)

# Nothing does for javac what CMAKE_CXX_FLAGS_DEBUG does for the compiler, and
# without -g the class files carry no variable names, so a debugger stopping in
# Java code has no locals to show.
set(CMAKE_JAVA_COMPILE_FLAGS -g)

@if '%{UseJni}' === 'true'
add_library(%{NativeTarget} SHARED %{CppFileName})
target_include_directories(%{NativeTarget} PRIVATE ${JNI_INCLUDE_DIRS})
set_target_properties(%{NativeTarget} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}>"
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}>"
)
if(WIN32)
    # System.mapLibraryName() looks for %{NativeTarget}.dll, which is what MSVC
    # builds, but MinGW would prefix the file name with "lib".
    set_target_properties(%{NativeTarget} PROPERTIES PREFIX "")
endif()

@endif
add_jar(%{ProjectName}
    SOURCES %{JavaFileName}
    ENTRY_POINT %{EntryPoint}
@if '%{UseJni}' === 'true'
    MANIFEST manifest.txt
@endif
)
@if '%{UseJni}' === 'true'
add_dependencies(%{ProjectName} %{NativeTarget})
@endif

install_jar(%{ProjectName} DESTINATION share/%{ProjectName})
@if '%{UseJni}' === 'true'
install(TARGETS %{NativeTarget} DESTINATION share/%{ProjectName})
@endif
