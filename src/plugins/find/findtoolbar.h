/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FINDTOOLBAR_H
#define FINDTOOLBAR_H

#include "ui_findwidget.h"
#include "currentdocumentfind.h"

#include <utils/styledbar.h>

#include <QTimer>

namespace Core {
class FindToolBarPlaceHolder;
}

namespace Find {
class FindPlugin;

namespace Internal {

class FindToolBar : public Utils::StyledBar
{
    Q_OBJECT

public:
    explicit FindToolBar(FindPlugin *plugin, CurrentDocumentFind *currentDocumentFind);
    ~FindToolBar();

    void readSettings();
    void writeSettings();

    void openFindToolBar(bool focus = true);
    void setUseFakeVim(bool on);

public slots:
    void setBackward(bool backward);

private slots:
    void invokeFindNext();
    void invokeFindPrevious();
    void invokeFindStep();
    void invokeReplace();
    void invokeReplaceNext();
    void invokeReplacePrevious();
    void invokeReplaceStep();
    void invokeReplaceAll();
    void invokeResetIncrementalSearch();

    void invokeFindIncremental();
    void invokeFindEnter();
    void invokeReplaceEnter();
    void putSelectionToFindClipboard();
    void updateFromFindClipboard();

    void hideAndResetFocus();
    void openFind(bool focus = true);
    void findNextSelected();
    void findPreviousSelected();
    void updateFindAction();
    void updateToolBar();
    void findFlagsChanged();

    void setCaseSensitive(bool sensitive);
    void setWholeWord(bool wholeOnly);
    void setRegularExpressions(bool regexp);
    void setPreserveCase(bool preserveCase);

    void adaptToCandidate();

protected:
    bool focusNextPrevChild(bool next);
    void keyPressEvent(QKeyEvent *event);

private:
    void installEventFilters();
    void invokeClearResults();
    bool setFocusToCurrentFindSupport();
    void setFindFlag(Find::FindFlag flag, bool enabled);
    bool hasFindFlag(Find::FindFlag flag);
    Find::FindFlags effectiveFindFlags();
    Core::FindToolBarPlaceHolder *findToolBarPlaceHolder() const;

    bool eventFilter(QObject *obj, QEvent *event);
    void setFindText(const QString &text);
    QString getFindText();
    QString getReplaceText();
    void selectFindText();
    void updateIcons();
    void updateFlagMenus();

    bool shouldSetFocusOnKeyEvent(QKeyEvent *event);

    FindPlugin *m_plugin;
    CurrentDocumentFind *m_currentDocumentFind;
    Ui::FindWidget m_ui;
    QCompleter *m_findCompleter;
    QCompleter *m_replaceCompleter;
    QAction *m_findInDocumentAction;
    QAction *m_findNextSelectedAction;
    QAction *m_findPreviousSelectedAction;
    QAction *m_enterFindStringAction;
    QAction *m_findNextAction;
    QAction *m_findPreviousAction;
    QAction *m_replaceAction;
    QAction *m_replaceNextAction;
    QAction *m_replacePreviousAction;
    QAction *m_replaceAllAction;
    QAction *m_caseSensitiveAction;
    QAction *m_wholeWordAction;
    QAction *m_regularExpressionAction;
    QAction *m_preserveCaseAction;
    Find::FindFlags m_findFlags;

    QTimer m_findIncrementalTimer;
    QTimer m_findStepTimer;
    bool m_useFakeVim;
    bool m_eventFiltersInstalled;
};

} // namespace Internal
} // namespace Find

#endif // FINDTOOLBAR_H
