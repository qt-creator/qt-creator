set(ARCHIVE_BLAKE2 1)

if (APPLE)
  set(ARCHIVE_ACL_DARWIN 1)
elseif(LINUX)
  set(ARCHIVE_ACL_LIBACL 1)
endif()

add_subdirectory(libarchive)

# To generate these files, use the CMakeLists.original.txt file, and copy the generated config.h
# from your build directories to the respective config_*.h file.
if(APPLE)
  target_compile_definitions(archive_static PRIVATE PLATFORM_CONFIG_H=<${CMAKE_CURRENT_SOURCE_DIR}/config_mac.h>)
elseif(WIN32)
    if(MINGW)
        target_compile_definitions(archive_static PRIVATE PLATFORM_CONFIG_H=<${CMAKE_CURRENT_SOURCE_DIR}/config_mingw.h>)
    else()
        target_compile_definitions(archive_static PRIVATE PLATFORM_CONFIG_H=<${CMAKE_CURRENT_SOURCE_DIR}/config_win.h>)
    endif()
else()
  target_compile_definitions(archive_static PRIVATE PLATFORM_CONFIG_H=<${CMAKE_CURRENT_SOURCE_DIR}/config_unix.h>)
endif()

if (NOT WIN32)
  find_package(Iconv REQUIRED)
  target_link_libraries(archive_static PRIVATE ${Iconv_LIBRARY})
endif()

target_compile_definitions(archive_static PRIVATE HAVE_ZLIB_H=1 HAVE_LIBZ=1)
target_link_libraries(archive_static PRIVATE qtcZLib)

target_compile_definitions(archive_static PRIVATE HAVE_BZLIB_H=1 HAVE_LIBBZ2=1 USE_BZIP2_STATIC)
target_link_libraries(archive_static PRIVATE qtcBZip2)

target_compile_definitions(archive_static PRIVATE HAVE_LZMA_H=1 HAVE_LIBLZMA=1 LZMA_API_STATIC)
target_link_libraries(archive_static PRIVATE qtcXz)
