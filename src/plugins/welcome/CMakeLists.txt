add_qtc_plugin(Welcome
  DEPENDS
  PLUGIN_DEPENDS Core
  SOURCES
    introductionwidget.cpp introductionwidget.h
    welcomeplugin.cpp
    welcometr.h
)

qtc_add_resources(Welcome "images"
  PREFIX "/welcome"
  BASE "."
  FILES
    images/border.png
    images/mode_welcome.png
    images/mode_welcome_mask.png
    images/mode_welcome_mask@2x.png
    images/mode_welcome@2x.png
    images/link.png
    images/link@2x.png
)
