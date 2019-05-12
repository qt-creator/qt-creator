set(EXT_TOOLS_DIR "${IDE_DATA_PATH}/externaltools")

install(FILES lrelease.xml lupdate.xml qmlscene.xml qmlviewer.xml
  DESTINATION "${EXT_TOOLS_DIR}"
)

if (WIN32)
  install(FILES notepad_win.xml DESTINATION "${EXT_TOOLS_DIR}")
elseif (APPLE)
  install(FILES vi_mac.xml DESTINATION "${EXT_TOOLS_DIR}")
else()
  install(FILES vi.xml DESTINATION "${EXT_TOOLS_DIR}")
endif()
