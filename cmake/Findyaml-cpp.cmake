#.rst:
# Findyaml-cpp
# -----------------
#
# Try to find a yaml-cpp config/package.
# If that fails, build and use our copy of it.
#

find_package(yaml-cpp 0.5 QUIET NO_MODULE)
if (yaml-cpp_FOUND)
  # target doesn't set include directory for some reason
  get_filename_component(yaml_cpp_include_dir ${YAML_CPP_INCLUDE_DIR} ABSOLUTE)
  if (NOT EXISTS yaml_cpp_include_dir)
    unset(yaml_cpp_include_dir)
    unset(yaml_cpp_include_dir CACHE)
    find_path(yaml_cpp_include_dir yaml-cpp/yaml.h)
  endif()
  target_include_directories(yaml-cpp INTERFACE ${yaml_cpp_include_dir})
else()
  set(yaml-cpp_FOUND 1)
  set_package_properties(yaml-cpp PROPERTIES DESCRIPTION "using internal src/libs/3rdparty/yaml-cpp")
  set(YAML_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src/libs/3rdparty/yaml-cpp)
  add_qtc_library(yaml-cpp
    DEFINES YAML_CPP_DLL yaml_cpp_EXPORTS
    INCLUDES ${YAML_SOURCE_DIR}/include
    PUBLIC_DEFINES YAML_CPP_DLL
    PUBLIC_INCLUDES ${YAML_SOURCE_DIR}/include
    SOURCES
      ${YAML_SOURCE_DIR}/include/yaml-cpp
      ${YAML_SOURCE_DIR}/include/yaml-cpp/anchor.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/binary.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/dll.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/emitfromevents.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/emitter.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/emitterdef.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/emittermanip.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/emitterstyle.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/eventhandler.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/exceptions.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/mark.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/convert.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/bool_type.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/impl.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/iterator.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/iterator_fwd.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/memory.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/node.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/node_data.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/node_iterator.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/detail/node_ref.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/emit.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/impl.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/iterator.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/node.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/parse.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/ptr.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/node/type.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/noncopyable.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/null.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/ostream_wrapper.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/parser.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/stlemitter.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/traits.h
      ${YAML_SOURCE_DIR}/include/yaml-cpp/yaml.h
      ${YAML_SOURCE_DIR}/src/binary.cpp
      ${YAML_SOURCE_DIR}/src/collectionstack.h
      ${YAML_SOURCE_DIR}/src/convert.cpp
      ${YAML_SOURCE_DIR}/src/directives.cpp
      ${YAML_SOURCE_DIR}/src/directives.h
      ${YAML_SOURCE_DIR}/src/emit.cpp
      ${YAML_SOURCE_DIR}/src/emitfromevents.cpp
      ${YAML_SOURCE_DIR}/src/emitter.cpp
      ${YAML_SOURCE_DIR}/src/emitterstate.cpp
      ${YAML_SOURCE_DIR}/src/emitterstate.h
      ${YAML_SOURCE_DIR}/src/emitterutils.cpp
      ${YAML_SOURCE_DIR}/src/emitterutils.h
      ${YAML_SOURCE_DIR}/src/exceptions.cpp
      ${YAML_SOURCE_DIR}/src/exp.cpp
      ${YAML_SOURCE_DIR}/src/exp.h
      ${YAML_SOURCE_DIR}/src/indentation.h
      ${YAML_SOURCE_DIR}/src/memory.cpp
      ${YAML_SOURCE_DIR}/src/node.cpp
      ${YAML_SOURCE_DIR}/src/node_data.cpp
      ${YAML_SOURCE_DIR}/src/nodebuilder.cpp
      ${YAML_SOURCE_DIR}/src/nodebuilder.h
      ${YAML_SOURCE_DIR}/src/nodeevents.cpp
      ${YAML_SOURCE_DIR}/src/nodeevents.h
      ${YAML_SOURCE_DIR}/src/null.cpp
      ${YAML_SOURCE_DIR}/src/ostream_wrapper.cpp
      ${YAML_SOURCE_DIR}/src/parse.cpp
      ${YAML_SOURCE_DIR}/src/parser.cpp
      ${YAML_SOURCE_DIR}/src/ptr_vector.h
      ${YAML_SOURCE_DIR}/src/regex_yaml.cpp
      ${YAML_SOURCE_DIR}/src/regex_yaml.h
      ${YAML_SOURCE_DIR}/src/regeximpl.h
      ${YAML_SOURCE_DIR}/src/scanner.cpp
      ${YAML_SOURCE_DIR}/src/scanner.h
      ${YAML_SOURCE_DIR}/src/scanscalar.cpp
      ${YAML_SOURCE_DIR}/src/scanscalar.h
      ${YAML_SOURCE_DIR}/src/scantag.cpp
      ${YAML_SOURCE_DIR}/src/scantag.h
      ${YAML_SOURCE_DIR}/src/scantoken.cpp
      ${YAML_SOURCE_DIR}/src/setting.h
      ${YAML_SOURCE_DIR}/src/simplekey.cpp
      ${YAML_SOURCE_DIR}/src/singledocparser.cpp
      ${YAML_SOURCE_DIR}/src/singledocparser.h
      ${YAML_SOURCE_DIR}/src/stream.cpp
      ${YAML_SOURCE_DIR}/src/stream.h
      ${YAML_SOURCE_DIR}/src/streamcharsource.h
      ${YAML_SOURCE_DIR}/src/stringsource.h
      ${YAML_SOURCE_DIR}/src/tag.cpp
      ${YAML_SOURCE_DIR}/src/tag.h
      ${YAML_SOURCE_DIR}/src/token.h
    )
    unset(YAML_SOURCE_DIR)
endif()
