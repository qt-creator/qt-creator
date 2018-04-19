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

source("../../shared/qtcreator.py")
import re

# test search in help mode and advanced search
searchKeywordDictionary={ "abundance":True, "deplmint":False, "QODBC":True, "bld":False }
urlDictionary = { "abundance":"qthelp://com.trolltech.qt.487/qdoc/gettingstarted-develop.html",
                  "QODBC":"qthelp://com.trolltech.qt.487/qdoc/sql-driver.html" }


def __getSelectedText__():
    hv = getHelpViewer()
    try:
        return hv.textCursor().selectedText()
    except:
        pass
    try:
        test.log("Falling back to searching for selection in HTML.")
        return getHighlightsInHtml(str(hv.toHtml()))
    except:
        test.warning("Could not get highlighted text.")
        return str("")

def __getUrl__():
    helpViewer = getHelpViewer()
    try:
        url = helpViewer.url
    except:
        try:
            url = helpViewer.source
        except:
            return ""
    return str(url.scheme) + "://" + str(url.host) + str(url.path)

def getHighlightsInHtml(htmlCode):
    pattern = re.compile('color:#ff0000;">(.*?)</span>')
    res = ""
    for curr in pattern.finditer(htmlCode):
        if curr.group(1) in res:
            continue
        res += "%s " % curr.group(1)
    return res

def verifySelection(expected):
    selText = str(__getSelectedText__())
    if test.verify(selText, "Verify that there is a selection"):
        # verify if search keyword is found in results
        test.verify(expected.lower() in selText.lower(),
                    "'%s' search result can be found" % expected)

def verifyUrl(expected):
    return test.compare(expected, __getUrl__(), "Expected URL loaded?")

def main():
    noMatch = "Your search did not match any documents."
    startApplication("qtcreator" + SettingsPath)
    if not startedWithoutPluginError():
        return
    addHelpDocumentation([os.path.join(qt4Path, "doc", "qch", "qt.qch")])
    # switch to help mode
    switchViewTo(ViewConstants.HELP)
    # verify that search widget is accessible
    mouseClick(waitForObjectItem(":Qt Creator_Core::Internal::CommandComboBox", "Search"))
    snooze(1) # Looks like searching is still available for an instant
    test.verify(checkIfObjectExists("{type='QHelpSearchQueryWidget' unnamed='1' visible='1' "
                                    "window=':Qt Creator_Core::Internal::MainWindow'}",
                                    timeout=600000),
                "Verifying search widget visibility.")
    # try to search empty string
    clickButton(waitForObject("{text='Search' type='QPushButton' unnamed='1' visible='1' "
                              "window=':Qt Creator_Core::Internal::MainWindow'}"))
    try:
        # Creator built with Qt <= 5.8.0
        resultWidget = waitForObject(':Hits_QCLuceneResultWidget', 5000)
        olderThan59 = True
    except:
        # Creator built with Qt >= 5.9.0
        resultWidget = waitForObject(':Hits_QResultWidget', 5000)
        olderThan59 = False
    if olderThan59 or not JIRA.isBugStillOpen(67737, JIRA.Bug.QT):
        test.verify(waitFor("noMatch in "
                            "str(resultWidget.plainText)", 2000),
                            "Verifying if search did not match anything.")
    # workaround for "endless waiting cursor"
    mouseClick(waitForObject("{column='0' container=':Qt Creator_QHelpContentWidget' "
                             "text='Qt Reference Documentation' type='QModelIndex'}"))
    # try to search keyword from list
    searchLineEdit = getChildByClass(waitForObject("{type='QHelpSearchQueryWidget' unnamed='1' visible='1' "
                                                   "window=':Qt Creator_Core::Internal::MainWindow'}"),
                                     "QLineEdit")
    for searchKeyword,shouldFind in searchKeywordDictionary.items():
        mouseClick(searchLineEdit)
        replaceEditorContent(searchLineEdit, searchKeyword)
        type(searchLineEdit, "<Return>")
        progressBarWait(warn=False)
        if shouldFind:
            test.verify(waitFor("re.match('[1-9]\d* - [1-9]\d* of [1-9]\d* Hits',"
                                "str(findObject(':Hits_QLabel').text))", 2000),
                                "Verifying if search results found with 1+ hits for: " + searchKeyword)
            selText = __getSelectedText__()
            url = __getUrl__()
            # click in the widget, tab to first item and press enter
            mouseClick(resultWidget)
            type(resultWidget, "<Tab>")
            type(resultWidget, "<Return>")
            waitFor("__getUrl__() != url or selText != __getSelectedText__()", 20000)
            verifySelection(searchKeyword)
            verifyUrl(urlDictionary[searchKeyword])
        else:
            if olderThan59 or not JIRA.isBugStillOpen(67737, JIRA.Bug.QT):
                test.verify(waitFor("noMatch in "
                                    "str(resultWidget.plainText)", 1000),
                                    "Verifying if search did not match anything for: " + searchKeyword)
    if olderThan59:
        # advanced search - setup
        clickButton(waitForObject("{text='+' type='QToolButton' unnamed='1' visible='1' "
                                  "window=':Qt Creator_Core::Internal::MainWindow'}"))
        label = ("{text='%s' type='QLabel' unnamed='1' visible='1' "
                 "window=':Qt Creator_Core::Internal::MainWindow'}")
        lineEdit = ("{leftWidget=%s type='QLineEdit' unnamed='1' visible='1' "
                    "window=':Qt Creator_Core::Internal::MainWindow'}")
        labelTextsToSearchStr = {"words <B>similar</B> to:":"deploy",
                                 "<B>without</B> the words:":"bookmark",
                                 "with <B>exact phrase</B>:":"sql in qt",
                                 "with <B>all</B> of the words:":"designer sql",
                                 "with <B>at least one</B> of the words:":"printing"}
        for labelText,searchStr in labelTextsToSearchStr.items():
            type(waitForObject(lineEdit % (label % labelText)), searchStr)
        # advanced search - do search
        clickButton(waitForObject("{text='Search' type='QPushButton' unnamed='1' visible='1' "
                                  "window=':Qt Creator_Core::Internal::MainWindow'}"))
        progressBarWait(warn=False)
        # verify that advanced search results found
        test.verify(waitFor("re.search('1 - 2 of 2 Hits',"
                            "str(findObject(':Hits_QLabel').text))", 3000),
                            "Verifying if 2 search results found")
        resultsView = waitForObject(":Hits_QCLuceneResultWidget")
        mouseClick(resultsView, 1, 1, 0, Qt.LeftButton)
        type(resultsView, "<Tab>")
        type(resultsView, "<Return>")
        verifySelection("printing")
        verifyUrl("qthelp://com.trolltech.qt.487/qdoc/overviews.html")
        for i in range(2):
            type(resultsView, "<Tab>")
        type(resultsView, "<Return>")
        verifyUrl("qthelp://com.trolltech.qt.487/qdoc/best-practices.html")
        # verify if simple search is properly disabled
        test.verify(not searchLineEdit.enabled,
                    "Verifying if simple search is not active in advanced mode.")
    # exit
    invokeMenuItem("File", "Exit")
