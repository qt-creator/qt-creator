add_qtc_library(DevContainer
  PUBLIC_DEPENDS Qt::Core Utils
  SOURCES
    devcontainer.cpp
    devcontainer.h
    devcontainerconfig.cpp
    devcontainerconfig.h
    devcontainerfeature.cpp
    devcontainerfeature.h
    devcontainer_global.h
    devcontainertr.h
    substitute.cpp
    substitute.h
)
