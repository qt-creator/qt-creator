if(Qt6_VERSION VERSION_LESS 6.11.0)
  set(TaskTree QtTaskTree)
else()
  set(TaskTree Qt::TaskTree)
endif()

add_qtc_library(DevContainer
  PUBLIC_DEPENDS Qt::Core ${TaskTree} Utils
  SOURCES
    devcontainer.cpp
    devcontainer.h
    devcontainerconfig.cpp
    devcontainerconfig.h
    devcontainer_global.h
    devcontainertr.h
    substitute.cpp
    substitute.h
)
