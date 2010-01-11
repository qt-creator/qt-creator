cmake_minimum_required(VERSION 2.0)
project(net7ssh CXX)
INCLUDE(CheckIncludeFileCXX)
check_include_file_cxx("botan/botan.h" HAVE_BOTAN)
if (NOT ${HAVE_BOTAN})
MESSAGE(FATAL_ERROR "Could not find Botan library.")
endif()
set(CMAKE_INSTALL_PREFIX "/usr")
add_subdirectory ( src )
include_directories ( src )
if ( UNIX )
  SET(UNIX_STYLE_FLAGS 1)
  SET(WIN32_STYLE_FLAGS 0)
  EXECUTE_PROCESS(
    COMMAND uname -a
    OUTPUT_VARIABLE OS_ARCH
    )
    
    IF ( OS_ARCH MATCHES ".*SunOS.*" )
      MESSAGE ("Making Changes for SunOS!")
      SET(CMAKE_C_COMPILER "cc")
      SET(CMAKE_CXX_COMPILER "CC")
      SET(CMAKE_LINKER "cc")
      SET(CMAKE_AR "/usr/ccs/bin/ar")
      SET(CMAKE_CXX_FLAGS "-I/usr/local/include")
      SET(CMAKE_SHARED_LINKER_FLAGS "-L/usr/local/lib")
      SET(CMAKE_MODULE_LINKER_FLAGS "-L/usr/local/lib")
    ENDIF ( OS_ARCH MATCHES ".*SunOS.*" )

ENDIF ( UNIX )
set(net7ssh_LIB_SRCS
    src/crypt.cpp
    src/crypt.h
    src/ne7ssh.cpp
    src/ne7ssh.h
    src/ne7ssh_channel.cpp
    src/ne7ssh_channel.h
    src/ne7ssh_connection.cpp
    src/ne7ssh_connection.h
    src/ne7ssh_kex.cpp
    src/ne7ssh_kex.h
    src/ne7ssh_session.cpp
    src/ne7ssh_session.h
    src/ne7ssh_string.cpp
    src/ne7ssh_string.h
    src/ne7ssh_transport.cpp
    src/ne7ssh_transport.h
    src/ne7ssh_types.h
    src/ne7ssh_keys.cpp
    src/ne7ssh_keys.h
    src/ne7ssh_error.cpp
    src/ne7ssh_error.h
    src/ne7ssh_sftp.cpp
    src/ne7ssh_sftp.h
    src/ne7ssh_sftp_packet.cpp
    src/ne7ssh_sftp_packet.h
    src/ne7ssh_mutex.cpp
    src/ne7ssh_mutex.h)

add_library(ne7ssh STATIC ${net7ssh_LIB_SRCS})

add_library(net7ssh SHARED ${net7ssh_LIB_SRCS})
target_link_libraries(net7ssh botan)
set_target_properties(net7ssh PROPERTIES VERSION 0.3.1 SOVERSION 0)


########### install files ###############

install(TARGETS net7ssh LIBRARY DESTINATION lib)
install(FILES src/ne7ssh.h src/ne7ssh_types.h src/ne7ssh_error.h src/ne7ssh_mutex.h DESTINATION include)
install(FILES LICENSE.QPL AUTHORS INSTALL COPYING CHANGELOG DESTINATION share/doc/ne7ssh)
install(DIRECTORY doc DESTINATION share/doc/ne7ssh)
install(DIRECTORY doc/html DESTINATION share/doc/ne7ssh)
install(DIRECTORY examples DESTINATION share/doc/ne7ssh)
