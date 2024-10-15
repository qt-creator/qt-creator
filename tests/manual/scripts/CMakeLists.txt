if(WITH_TESTS)
    qtc_copy_to_builddir(copy_test_scripts_to_share_builddir
        DIRECTORIES lua/scripts
        DESTINATION "${IDE_DATA_PATH}"
        CREATE_SUBDIRS
    )
endif()
