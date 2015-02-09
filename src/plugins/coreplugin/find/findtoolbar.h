/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FINDTOOLBAR_H
#define FINDTOOLBAR_H

#include "ui_findwidget.h"
#include "currentdocumentfind.h"

#include <coreplugin/id.h>
#include <utils/styledbar.h>

#include <QTimer>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Core {

class FindToolBarPlaceHolder;
class FindPlugin;

namespace Internal {

class OptionsPopup : public QWidget
{
    Q_OBJECT

public:
    explicit OptionsPopup(QWidget *parent);

protected:
    bool event(QEvent *ev);
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
    void actionChanged();

private:
    QCheckBox *createCheckboxForCommand(Id id);

    QMap<QAction *, QCheckBox *> m_checkboxMap;
};

class FindToolBar : public Utils::StyledBar
{
    Q_OBJECT

public:
    enum OpenFlag {
        UpdateFocusAndSelect = 0x01,
        UpdateFindScope = 0x02,
        UpdateFindText = 0x04,
        UpdateHighlight = 0x08,
        UpdateAll = 0x0F
    };
    Q_DECLARE_FLAGS(OpenFlags, OpenFlag)

    explicit FindToolBar(FindPlugin *plugin, CurrentDocumentFind *currentDocumentFind);
    ~FindToolBar();

    void readSettings();
    void writeSettings();

    void openFindToolBar(OpenFlags flags = UpdateAll);
    void setUseFakeVim(bool on);

    void setLightColoredIcon(bool lightColored);
public slots:
    void setBackward(bool backward);

private slots:
    void invokeFindNext();
    void invokeGlobalFindNext();
    void invokeFindPrevious();
    void invokeGlobalFindPrevious();
    void invokeFindStep();
    void invokeReplace();
    void invokeGlobalReplace();
    void invokeReplaceNext();
    void invokeGlobalReplaceNext();
    void invokeReplacePrevious();
    void invokeGlobalReplacePrevious();
    void invokeReplaceStep();
    void invokeReplaceAll();
    void invokeGlobalReplaceAll();
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
    void updateGlobalActions();
    void updateToolBar();
    void findFlagsChanged();
    void findEditButtonClicked();

    void setCaseSensitive(bool sensitive);
    void setWholeWord(bool wholeOnly);
    void setRegularExpressions(bool regexp);
    void setPreserveCase(bool preserveCase);

    void adaptToCandidate();

    void setFocusToCurrentFindSupport();

protected:
    bool focusNextPrevChild(bool next);
    void resizeEvent(QResizeEvent *event);

private:
    void installEventFilters();
    void invokeClearResults();
    void setFindFlag(FindFlag flag, bool enabled);
    bool hasFindFlag(FindFlag flag);
    FindFlags effectiveFindFlags();
    FindToolBarPlaceHolder *findToolBarPlaceHolder() const;
    bool toolBarHasFocus() const;
    bool canShowAllControls(bool replaceIsVisible) const;
    void acceptCandidateAndMoveToolBar();
    void indicateSearchState(IFindSupport::Result searchState);

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
    QAction *m_goToCurrentFindAction;
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

    QAction *m_localFindNextAction;
    QAction *m_localFindPreviousAction;
    QAction *m_localReplaceAction;
    QAction *m_localReplaceNextAction;
    QAction *m_localReplacePreviousAction;
    QAction *m_localReplaceAllAction;

    FindFlags m_findFlags;

    QTimer m_findIncrementalTimer;
    QTimer m_findStepTimer;
    bool m_useFakeVim;
    bool m_eventFiltersInstalled;
};

} // namespace Internal
} // namespace Core

#endif // FINDTOOLBAR_H
