add_qtc_library(DevContainer
  PUBLIC_DEPENDS Qt::Core Utils Tasking
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
