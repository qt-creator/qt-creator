add_qtc_executable(qtpromaker DEPENDS Qt5::Core SOURCES main.cpp)
install(TARGETS qtpromaker DESTINATION "${IDE_LIBEXEC_PATH}")
