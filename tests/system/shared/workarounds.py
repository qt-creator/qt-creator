import urllib2
import re

JIRA_URL='https://bugreports.qt-project.org/browse'

class JIRA:
    __instance__ = None

    # Helper class
    class Bug:
        CREATOR = 'QTCREATORBUG'
        SIMULATOR = 'QTSIM'
        SDK = 'QTSDK'
        QT = 'QTBUG'
        QT_QUICKCOMPONENTS = 'QTCOMPONENTS'

    # constructor of JIRA
    def __init__(self, number, bugType=Bug.CREATOR):
        if JIRA.__instance__ == None:
            JIRA.__instance__ = JIRA.__impl(number, bugType)
            JIRA.__dict__['_JIRA__instance__'] = JIRA.__instance__
        else:
            JIRA.__instance__._bugType = bugType
            JIRA.__instance__._number = number
            JIRA.__instance__.__fetchStatusAndResolutionFromJira__()

    # overriden to make it possible to use JIRA just like the
    # underlying implementation (__impl)
    def __getattr__(self, attr):
        return getattr(self.__instance__, attr)

    # overriden to make it possible to use JIRA just like the
    # underlying implementation (__impl)
    def __setattr__(self, attr, value):
        return setattr(self.__instance__, attr, value)

    # function to get an instance of the singleton
    @staticmethod
    def getInstance():
        if '_JIRA__instance__' in JIRA.__dict__:
            return JIRA.__instance__
        else:
            return JIRA.__impl(0, Bug.CREATOR)

    # function to check if the given bug is open or not
    @staticmethod
    def isBugStillOpen(number, bugType=Bug.CREATOR):
        tmpJIRA = JIRA(number, bugType)
        return tmpJIRA.isOpen()

    # function similar to performWorkaroundForBug - but it will execute the
    # workaround (function) only if the bug is still open
    # returns True if the workaround function has been executed, False otherwise
    @staticmethod
    def performWorkaroundIfStillOpen(number, bugType=Bug.CREATOR, *args):
        if JIRA.isBugStillOpen(number, bugType):
            return JIRA.performWorkaroundForBug(number, bugType, *args)
        else:
            test.warning("Bug is closed... skipping workaround!",
                         "You should remove potential code inside performWorkaroundForBug()")
            return False

    # function that performs the workaround (function) for the given bug
    # if the function needs additional arguments pass them as 3rd parameter
    @staticmethod
    def performWorkaroundForBug(number, bugType=Bug.CREATOR, *args):
        functionToCall = JIRA.getInstance().__bugs__.get("%s-%d" % (bugType, number), None)
        if functionToCall:
            test.warning("Using workaround for %s-%d" % (bugType, number))
            functionToCall(*args)
            return True
        else:
            JIRA.getInstance()._exitFatal_(bugType, number)
            return False

    # implementation of JIRA singleton
    class __impl:
        # constructor of __impl
        def __init__(self, number, bugType):
            self._number = number
            self._bugType = bugType
            self._localOnly = os.getenv("SYSTEST_JIRA_NO_LOOKUP")=="1"
            self.__initBugDict__()
            self.__fetchStatusAndResolutionFromJira__()

        # function to retrieve the status of the current bug
        def getStatus(self):
            return self._status

        # function to retrieve the resolution of the current bug
        def getResolution(self):
            return self._resolution

        # this function checks the resolution of the given bug
        # and returns True if the bug can still be assumed as 'Open' and False otherwise
        def isOpen(self):
            # handle special cases
            if self._resolution == None:
                return True
            if self._resolution in ('Duplicate', 'Moved', 'Incomplete', 'Cannot Reproduce', 'Invalid'):
                test.warning("Resolution of bug is '%s' - assuming 'Open' for now." % self._resolution,
                             "Please check the bugreport manually and update this test.")
                return True
            return self._resolution != 'Done'

        # this function tries to fetch the status and resolution from JIRA for the given bug
        # if this isn't possible or the lookup is disabled it does only check the internal
        # dict whether a function for the given bug is deposited or not
        def __fetchStatusAndResolutionFromJira__(self):
            global JIRA_URL
            data = None
            if not self._localOnly:
                try:
                    bugReport = urllib2.urlopen('%s/%s-%d' % (JIRA_URL, self._bugType, self._number))
                    data = bugReport.read()
                except:
                    data = self.__tryExternalTools__()
                    if data == None:
                        test.warning("Sorry, ssl module missing - cannot fetch data via HTTPS",
                                     "Try to install the ssl module by yourself, or set the python "
                                     "path inside SQUISHDIR/etc/paths.ini to use a python version with "
                                     "ssl support OR install wget or curl to get rid of this warning!")
                        self._localOnly = True
            if data == None:
                if '%s-%d' % (self._bugType, self._number) in self.__bugs__:
                    test.warning("Using internal dict - bug status could have changed already",
                                 "Please check manually!")
                    self._status = None
                    self._resolution = None
                    return
                else:
                    test.fatal("No workaround function deposited for %s-%d" % (self._bugType, self._number))
                    self._resolution = 'Done'
                    return
            else:
                data = data.replace("\r", "").replace("\n", "")
                resPattern = re.compile('<span\s+id="resolution-val".*?>(?P<resolution>.*?)</span>')
                statPattern = re.compile('<span\s+id="status-val".*?>(.*?<img.*?>)?(?P<status>.*?)</span>')
                status = statPattern.search(data)
                resolution = resPattern.search(data)
            if status:
                self._status = status.group("status").strip()
            else:
                test.fatal("FATAL: Cannot get status of bugreport %s-%d" % (self._bugType, self._number),
                           "Looks like JIRA has changed.... Please verify!")
                self._status = None
            if resolution:
                self._resolution = resolution.group("resolution").strip()
            else:
                test.fatal("FATAL: Cannot get resolution of bugreport %s-%d" % (self._bugType, self._number),
                           "Looks like JIRA has changed.... Please verify!")
                self._resolution = None

        # simple helper function - used as fallback if python has no ssl support
        # tries to find curl or wget in PATH and fetches data with it instead of
        # using urllib2
        def __tryExternalTools__(self):
            global JIRA_URL
            cmdAndArgs = { 'curl':'-k', 'wget':'-qO-' }
            for call in cmdAndArgs:
                prog = which(call)
                if prog:
                    return getOutputFromCmdline('"%s" %s %s/%s-%d' % (prog, cmdAndArgs[call], JIRA_URL, self._bugType, self._number))
            return None

        # this function initializes the bug dict for localOnly usage and
        # for later lookup which function to call for which bug
        # ALWAYS update this dict when adding a new function for a workaround!
        def __initBugDict__(self):
            self.__bugs__= {
                            'QTCREATORBUG-6853':self._workaroundCreator6853_,
                            'QTCREATORBUG-6918':self._workaroundCreator_MacEditorFocus_,
                            'QTCREATORBUG-6953':self._workaroundCreator_MacEditorFocus_,
                            'QTCREATORBUG-6994':self._workaroundCreator6994_,
                            'QTCREATORBUG-7002':self._workaroundCreator7002_
                            }
        # helper function - will be called if no workaround for the requested bug is deposited
        def _exitFatal_(self, bugType, number):
            test.fatal("No workaround found for bug %s-%d" % (bugType, number))

############### functions that hold workarounds #################################

        def _workaroundCreator6994_(self, *args):
            if args[0] in ('Mobile Qt Application', 'Qt Gui Application', 'Qt Custom Designer Widget'):
                args[1].remove('Harmattan')
                test.xverify(False, "Removed Harmattan from expected targets.")

        def _workaroundCreator6853_(self, *args):
            if "Release" in args[0] and platform.system() == "Linux":
                snooze(1)

        def _workaroundCreator_MacEditorFocus_(self, *args):
            editor = args[0]
            nativeMouseClick(editor.mapToGlobal(QPoint(50, 50)).x, editor.mapToGlobal(QPoint(50, 50)).y, Qt.LeftButton)

        def _workaroundCreator7002_(self, *args):
            if platform.system() in ("Linux", "Darwin"):
                result = args[0]
                result.append(QtQuickConstants.Targets.EMBEDDED_LINUX)
