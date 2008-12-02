/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef FINDTOOLBAR_H
#define FINDTOOLBAR_H

#include "ui_findwidget.h"
#include "ifindfilter.h"
#include "currentdocumentfind.h"

#include <QtGui/QStringListModel>
#include <QtGui/QWidget>
#include <QtGui/QToolBar>
#include <QtGui/QLabel>

namespace Find {
namespace Internal {

class FindPlugin;

class FindToolBar : public QToolBar
{
    Q_OBJECT

public:
    FindToolBar(FindPlugin *plugin, CurrentDocumentFind *currentDocumentFind);
    ~FindToolBar();

    void invokeClearResults();

private slots:
    void invokeFindNext();
    void invokeFindPrevious();
    void invokeFindStep();
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
    void openFind();
    void updateActions();
    void findFlagsChanged();

private:
    bool setFocusToCurrentFindSupport();

    bool eventFilter(QObject *obj, QEvent *event);
    void setFindText(const QString &text);
    QString getFindText();
    QString getReplaceText();
    void selectFindText();
    void updateIcons();
    void updateFlagMenus();

    FindPlugin *m_plugin;
    CurrentDocumentFind *m_currentDocumentFind;
    Ui::FindWidget m_ui;
    QCompleter *m_findCompleter;
    QCompleter *m_replaceCompleter;
    QAction *m_findInDocumentAction;
    QAction *m_enterFindStringAction;
    QAction *m_findNextAction;
    QAction *m_findPreviousAction;
    QAction *m_replaceNextAction;
    QAction *m_replacePreviousAction;
    QAction *m_replaceAllAction;
    QAction *m_caseSensitiveAction;
    QAction *m_wholeWordAction;
    QWidget *m_widget;
};

} // namespace Internal
} // namespace Find

#endif // FINDTOOLBAR_H
