add_qtc_library(DevContainer
  DEPENDS Qt::Network
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
    featuredownloader.cpp
    featuredownloader.h
    substitute.cpp
    substitute.h
)
