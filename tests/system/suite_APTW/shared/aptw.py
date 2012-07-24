# shared script for APTW suite
# helping to run and close app
# verification

# verify if building and running of project was successful
def verifyBuildAndRun():
    # check compile output if build successful
    checkCompile()
    # check application output log
    appOutput = logApplicationOutput()
    if appOutput:
        test.verify(re.search(".*([Pp]rogram).*(unexpectedly).*([Ff]inished).*", str(appOutput)) and
                    re.search('[Ss]tarting.*', str(appOutput)),
                    "Verifying if built app started and closed successfully.")

# pick version 4.7.4 and then run project for debug and release
def pickVersion474runVerify():
    availableConfigs = iterateBuildConfigs(1, 0)
    if not availableConfigs:
        test.fatal("Haven't found needed Qt version (Qt 4.7.4), quitting")
        invokeMenuItem("File", "Save All")
        invokeMenuItem("File", "Exit")
    # select debug configuration
    for config in availableConfigs:
        selectBuildConfig(1, 0, config)
        test.log("Using build config '%s'" % config)
        runAndCloseApp()
        verifyBuildAndRun()
        mouseClick(waitForObject(":*Qt Creator.Clear_QToolButton"))
