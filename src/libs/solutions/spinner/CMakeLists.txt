add_qtc_library(Spinner OBJECT
# Never add dependencies to non-Qt libraries for this library
  DEPENDS Qt::Core Qt::Widgets
  SOURCES
    spinner.cpp spinner.h
    spinner_global.h
)

qtc_add_resources(Spinner "spinner_images"
  FILES
    icons/spinner_large.png
    icons/spinner_large@2x.png
    icons/spinner_medium.png
    icons/spinner_medium@2x.png
    icons/spinner_small.png
    icons/spinner_small@2x.png
)
