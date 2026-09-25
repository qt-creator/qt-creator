#
# Record the Java archives of the project for Qt Creator
#
# UseJava's add_jar() builds a jar from a custom target, so the CMake file API
# reports neither the .java files nor the archive: the target comes out as
# UTILITY with no artifacts and no sources. What add_jar() does leave behind
# are the JAR_FILE and CLASSDIR target properties and, in the class directory,
# the "java_sources" file listing every source it compiles. Walking the targets
# once at the end of the configure turns that into a JSON file the IDE reads.
#

function(qtc_java_collect_targets directory out_targets)
  set(result "")

  get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    get_property(jar_file TARGET "${target}" PROPERTY JAR_FILE)
    if (jar_file)
      list(APPEND result "${target}")
    endif()
  endforeach()

  get_property(subdirectories DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
  foreach(subdirectory IN LISTS subdirectories)
    qtc_java_collect_targets("${subdirectory}" subdirectory_targets)
    list(APPEND result ${subdirectory_targets})
  endforeach()

  set(${out_targets} "${result}" PARENT_SCOPE)
endfunction()

function(qtc_java_quote value out_string)
  string(REPLACE "\\" "\\\\" value "${value}")
  string(REPLACE "\"" "\\\"" value "${value}")
  set(${out_string} "\"${value}\"" PARENT_SCOPE)
endfunction()

# Sources that add_jar() was given as "@filelist" are not in here: UseJava
# keeps those in a list of its own and passes the file to javac as it is, so
# what it compiles from them is not recoverable from the target.
function(qtc_java_target_sources target source_directory out_sources)
  get_property(class_directory TARGET "${target}" PROPERTY CLASSDIR)
  if (NOT class_directory OR NOT EXISTS "${class_directory}/java_sources")
    set(${out_sources} "" PARENT_SCOPE)
    return()
  endif()

  # One quoted file name per line, as add_jar() passes them to javac. They are
  # written the way the project spelled them, which may be relative to the
  # directory of the add_jar() call. add_jar() writes the file through
  # configure_file(), so it is UTF-8; read without saying so, a path with a
  # non-ASCII character in it comes apart at that character.
  file(STRINGS "${class_directory}/java_sources" sources ENCODING UTF-8)
  set(result "")
  foreach(source IN LISTS sources)
    string(REGEX REPLACE "^\"(.*)\"$" "\\1" source "${source}")
    get_filename_component(source "${source}" ABSOLUTE BASE_DIR "${source_directory}")
    list(APPEND result "${source}")
  endforeach()

  set(${out_sources} "${result}" PARENT_SCOPE)
endfunction()

# A path of the tree is written the way the file API reports its own: relative
# to the top of the tree it is in, so that whoever reads it resolves it against
# the directory it knows that tree under. CMake may have been handed the tree
# under another name than the IDE uses - on Windows it is configured through a
# junction - and an absolute path would carry that name with it.
function(qtc_java_path_in_tree path base out_path)
  file(RELATIVE_PATH relative "${base}" "${path}")
  if (relative STREQUAL "")
    # The top of the tree itself.
    set(relative ".")
  endif()
  if (NOT IS_ABSOLUTE "${relative}" AND NOT relative MATCHES "^\\.\\.($|/)")
    set(${out_path} "${relative}" PARENT_SCOPE)
  else()
    set(${out_path} "${path}" PARENT_SCOPE)
  endif()
endfunction()

function(qtc_java_write_targets)
  set(output_file "${CMAKE_BINARY_DIR}/.qtcreator/java-targets.json")

  qtc_java_collect_targets("${CMAKE_SOURCE_DIR}" java_targets)
  if (NOT java_targets)
    # A project that stopped building jars must not keep the ones it used to.
    file(REMOVE "${output_file}")
    return()
  endif()

  set(entries "")
  foreach(target IN LISTS java_targets)
    get_property(jar_file TARGET "${target}" PROPERTY JAR_FILE)
    get_property(source_directory TARGET "${target}" PROPERTY SOURCE_DIR)
    qtc_java_target_sources("${target}" "${source_directory}" sources)

    set(quoted_sources "")
    foreach(source IN LISTS sources)
      qtc_java_path_in_tree("${source}" "${CMAKE_SOURCE_DIR}" source)
      qtc_java_quote("${source}" quoted_source)
      list(APPEND quoted_sources "${quoted_source}")
    endforeach()
    list(JOIN quoted_sources ", " quoted_sources)

    qtc_java_path_in_tree("${jar_file}" "${CMAKE_BINARY_DIR}" jar_file)
    qtc_java_path_in_tree("${source_directory}" "${CMAKE_SOURCE_DIR}" source_directory)

    qtc_java_quote("${target}" quoted_target)
    qtc_java_quote("${jar_file}" quoted_jar_file)
    qtc_java_quote("${source_directory}" quoted_source_directory)

    set(entry "    {\n")
    string(APPEND entry "      \"name\": ${quoted_target},\n")
    string(APPEND entry "      \"jar\": ${quoted_jar_file},\n")
    string(APPEND entry "      \"sourceDirectory\": ${quoted_source_directory},\n")
    string(APPEND entry "      \"sources\": [${quoted_sources}]\n")
    string(APPEND entry "    }")
    list(APPEND entries "${entry}")
  endforeach()
  list(JOIN entries ",\n" entries)

  file(WRITE "${output_file}" "{\n  \"targets\": [\n${entries}\n  ]\n}\n")
endfunction()

# CMAKE_PROJECT_INCLUDE_BEFORE is included by every project() call in the
# tree, and a superbuild or a FetchContent dependency has one of its own. The
# first call is the outermost, so deferring there alone writes the file once,
# at the end of the directory that all others have been processed under.
# The guard is a directory variable, not a cache one: every configure has to
# defer the call again, and a subdirectory inherits what the outermost set.
if (NOT QTCREATOR_JAVA_TARGETS_DEFERRED)
  set(QTCREATOR_JAVA_TARGETS_DEFERRED ON)
  cmake_language(DEFER CALL qtc_java_write_targets)
endif()
