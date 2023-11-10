add_qtc_library(Spinner OBJECT
# Never add dependencies to non-Qt libraries for this library
  DEPENDS Qt::Core Qt::Widgets
  SOURCES
    spinner.cpp spinner.h
    spinner.qrc
    spinner_global.h
)
