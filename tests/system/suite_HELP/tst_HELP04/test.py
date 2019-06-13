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

# test search in help mode and advanced search
searchKeywordDictionary = { "abundance":True, "deplmint":False, "QODBC":True, "bldx":False }
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
    startQC()
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
    resultWidget = waitForObject(':Hits_QResultWidget', 5000)
    if os.getenv("SYSTEST_BUILT_WITH_QT_5_13_1_OR_NEWER", "0") == "1":
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
            if not (searchKeyword == "QODBC" and JIRA.isBugStillOpen(10331)):
                verifyUrl(urlDictionary[searchKeyword])
        else:
            if os.getenv("SYSTEST_BUILT_WITH_QT_5_13_1_OR_NEWER", "0") == "1":
                test.verify(waitFor("noMatch in "
                                    "str(resultWidget.plainText)", 1000),
                                    "Verifying if search did not match anything for: " + searchKeyword)
    # exit
    invokeMenuItem("File", "Exit")
