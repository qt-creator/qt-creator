qtc_output_binary_dir(_output_binary_dir)
add_qtc_test(tst_sdktool
  DEPENDS sdktoolLib
  DEFINES SDKTOOL_DIR="${_output_binary_dir}/${IDE_LIBEXEC_PATH}"
  SOURCES tst_sdktool.cpp
)
