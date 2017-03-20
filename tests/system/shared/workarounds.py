############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

import urllib2
import re

############ functions not related to issues tracked inside jira ############

def __checkWithoutWidget__(*args):
    return className(args[0]) == 'QMenu' and args[0].visible

def __checkWithWidget__(*args):
    return (__checkWithoutWidget__(args[0])
            and widgetContainsPoint(waitForObject(args[1]), args[0].mapToGlobal(QPoint(0 ,0))))

# hack for activating context menus on Mac because of Squish5/Qt5.2 problems
# param item a string holding the menu item to invoke (just the label)
# param widget an object; if provided there will be an additional check if the menu's top left
#              corner is placed on this widget
def macHackActivateContextMenuItem(item, widget=None):
    if widget:
        func = __checkWithWidget__
    else:
        func = __checkWithoutWidget__
    for obj in object.topLevelObjects():
        try:
            if func(obj, widget):
                activateItem(waitForObjectItem(obj, item))
                return True
        except:
            pass
    return False

################ workarounds for issues tracked inside jira #################

JIRA_URL='https://bugreports.qt.io/browse'

class JIRA:
    __instance__ = None

    # internal exception to be used inside workaround functions (lack of having return values)
    class JiraException(Exception):
        def __init__(self, value):
            self.value = value
        def __str__(self):
            return repr(self.value)

    # Helper class
    class Bug:
        CREATOR = 'QTCREATORBUG'
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
            JIRA.__instance__.__fetchResolutionFromJira__()

    # overridden to make it possible to use JIRA just like the
    # underlying implementation (__impl)
    def __getattr__(self, attr):
        return getattr(self.__instance__, attr)

    # overridden to make it possible to use JIRA just like the
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

    # function that performs the workaround (function) for the given bug
    # if the function needs additional arguments pass them as 3rd parameter
    @staticmethod
    def performWorkaroundForBug(number, bugType=Bug.CREATOR, *args):
        if not JIRA.isBugStillOpen(number, bugType):
            test.warning("Bug %s-%d is closed for version %s." %
                         (bugType, number, JIRA(number, bugType)._fix),
                         "You should probably remove potential code inside workarounds.py")
        functionToCall = JIRA.getInstance().__bugs__.get("%s-%d" % (bugType, number), None)
        if functionToCall:
            test.warning("Using workaround for %s-%d" % (bugType, number))
            try:
                functionToCall(*args)
            except:
                t, v = sys.exc_info()[0:2]
                if t == JIRA.JiraException:
                    raise JIRA.JiraException(v)
                else:
                    test.warning("Exception caught while executing workaround function.",
                                 "%s (%s)" % (str(t), str(v)))
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
            self._fix = None
            self._localOnly = os.getenv("SYSTEST_JIRA_NO_LOOKUP")=="1"
            self.__initBugDict__()
            self._fetchResults_ = {}
            self.__fetchResolutionFromJira__()

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

        # this function tries to fetch the resolution from JIRA for the given bug
        # if this isn't possible or the lookup is disabled it does only check the internal
        # dict whether a function for the given bug is deposited or not
        def __fetchResolutionFromJira__(self):
            global JIRA_URL
            bug = "%s-%d" % (self._bugType, self._number)
            if bug in self._fetchResults_:
                result = self._fetchResults_[bug]
                self._resolution = result[0]
                self._fix = result[1]
                return
            data = None
            proxy = os.getenv("SYSTEST_PROXY", None)
            if not self._localOnly:
                try:
                    if proxy:
                        proxy = urllib2.ProxyHandler({'https': proxy})
                        opener = urllib2.build_opener(proxy)
                        urllib2.install_opener(opener)
                    bugReport = urllib2.urlopen('%s/%s' % (JIRA_URL, bug))
                    data = bugReport.read()
                except:
                    data = self.__tryExternalTools__(proxy)
                    if data == None:
                        test.warning("Sorry, ssl module missing - cannot fetch data via HTTPS",
                                     "Try to install the ssl module by yourself, or set the python "
                                     "path inside SQUISHDIR/etc/paths.ini to use a python version with "
                                     "ssl support OR install wget or curl to get rid of this warning!")
                        self._localOnly = True
            if data == None:
                if bug in self.__bugs__:
                    test.warning("Using internal dict - bug status could have changed already",
                                 "Please check manually!")
                    self._resolution = None
                else:
                    test.fatal("No workaround function deposited for %s" % bug)
                    self._resolution = 'Done'
            else:
                data = data.replace("\r", "").replace("\n", "")
                resPattern = re.compile('<span\s+id="resolution-val".*?>(?P<resolution>.*?)</span>')
                resolution = resPattern.search(data)
                fixVersion = 'None'
                fixPattern = re.compile('<span.*?id="fixfor-val".*?>(?P<fix>.*?)</span>')
                fix = fixPattern.search(data)
                titlePattern = re.compile('title="(?P<title>.*?)"')
                if fix:
                    fix = titlePattern.search(fix.group('fix').strip())
                    if fix:
                        fixVersion = fix.group('title').strip()
                self._fix = fixVersion
                if resolution:
                    self._resolution = resolution.group("resolution").strip()
                else:
                    test.fatal("FATAL: Cannot get resolution of bugreport %s" % bug,
                               "Looks like JIRA has changed.... Please verify!")
                    self._resolution = None
            if self._resolution == None:
                self.__cropAndLog__(data)
            self._fetchResults_.update({bug:[self._resolution, self._fix]})

        # simple helper function - used as fallback if python has no ssl support
        # tries to find curl or wget in PATH and fetches data with it instead of
        # using urllib2
        def __tryExternalTools__(self, proxy=None):
            global JIRA_URL
            if proxy:
                cmdAndArgs = { 'curl':['-k', '--proxy', proxy],
                               'wget':['-qO-']}
            else:
                cmdAndArgs = { 'curl':['-k'], 'wget':['-qO-']}
            for call in cmdAndArgs:
                prog = which(call)
                if prog:
                    if call == 'wget' and proxy and os.getenv("https_proxy", None) == None:
                        test.warning("Missing environment variable https_proxy for using wget with proxy!")
                    cmdline = [prog] + cmdAndArgs[call]
                    cmdline += ['%s/%s-%d' % (JIRA_URL, self._bugType, self._number)]
                    return getOutputFromCmdline(cmdline)
            return None

        # this function crops multiple whitespaces from fetched and searches for expected
        # ids without using regex
        def __cropAndLog__(self, fetched):
            if fetched == None:
                test.log("None passed to __cropAndLog__()")
                return
            fetched = " ".join(fetched.split())
            resoInd = fetched.find('resolution-val')
            if resoInd == -1:
                test.log("Resolution not found inside fetched data.",
                         "%s[...]" % fetched[:200])
            else:
                test.log("Fetched and cropped data: [...]%s[...]" % fetched[resoInd-20:resoInd+100])

        # this function initializes the bug dict for localOnly usage and
        # for later lookup which function to call for which bug
        # ALWAYS update this dict when adding a new function for a workaround!
        def __initBugDict__(self):
            self.__bugs__= {}
        # helper function - will be called if no workaround for the requested bug is deposited
        def _exitFatal_(self, bugType, number):
            test.fatal("No workaround found for bug %s-%d" % (bugType, number))

############### functions that hold workarounds #################################
