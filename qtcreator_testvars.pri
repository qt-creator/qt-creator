isEmpty(QTCREATOR_TESTVARS_PRI_INCLUDED) {

    QTCREATOR_TESTVARS_PRI_INCLUDED = 1

    QTC_BUILD_TESTS = $$(QTC_BUILD_TESTS)
    !isEmpty(QTC_BUILD_TESTS):TEST = $$QTC_BUILD_TESTS

    !isEmpty(BUILD_TESTS):TEST = 1

    isEmpty(TEST):CONFIG(debug, debug|release) {
        !debug_and_release|build_pass {
            TEST = 1
        }
    }

}
