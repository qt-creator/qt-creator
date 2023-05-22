// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "currentdocumentfind.h"

#include <utils/id.h>
#include <utils/styledbar.h>

#include <QTimer>

QT_BEGIN_NAMESPACE
class QCompleter;
class QHBoxLayout;
class QLabel;
class QSpacerItem;
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {

class FindToolBarPlaceHolder;

namespace Internal {

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

    explicit FindToolBar(CurrentDocumentFind *currentDocumentFind);
    ~FindToolBar() override;

    void readSettings();
    void writeSettings();

    void openFindToolBar(OpenFlags flags = UpdateAll);
    void setUseFakeVim(bool on);

    void setLightColoredIcon(bool lightColored);

    QString getFindText();

public slots:
    void setBackward(bool backward);

protected:
    bool focusNextPrevChild(bool next) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    enum class ControlStyle {
        Text,
        Icon,
        Hidden
    };

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
    void selectAll();
    void updateActions();
    void updateToolBar();
    void findFlagsChanged();
    void findEditButtonClicked();
    void findCompleterActivated(const QModelIndex &);

    void setCaseSensitive(bool sensitive);
    void setWholeWord(bool wholeOnly);
    void setRegularExpressions(bool regexp);
    void setPreserveCase(bool preserveCase);

    void adaptToCandidate();

    void setFocusToCurrentFindSupport();

    void installEventFilters();
    void invokeClearResults();
    void setFindFlag(Utils::FindFlag flag, bool enabled);
    bool hasFindFlag(Utils::FindFlag flag);
    Utils::FindFlags effectiveFindFlags();
    static FindToolBarPlaceHolder *findToolBarPlaceHolder();
    bool toolBarHasFocus() const;
    ControlStyle controlStyle(bool replaceIsVisible);
    void setFindButtonStyle(Qt::ToolButtonStyle style);
    void acceptCandidateAndMoveToolBar();
    void indicateSearchState(IFindSupport::Result searchState);

    bool eventFilter(QObject *obj, QEvent *event) override;
    void setFindText(const QString &text);
    QString getReplaceText();
    void selectFindText();
    void updateIcons();
    void updateFlagMenus();
    void updateFindReplaceEnabled();
    void updateReplaceEnabled();

    CurrentDocumentFind *m_currentDocumentFind = nullptr;

    QCompleter *m_findCompleter = nullptr;
    QCompleter *m_replaceCompleter = nullptr;
    QAction *m_goToCurrentFindAction = nullptr;
    QAction *m_findInDocumentAction = nullptr;
    QAction *m_findNextSelectedAction = nullptr;
    QAction *m_findPreviousSelectedAction = nullptr;
    QAction *m_selectAllAction = nullptr;
    QAction *m_enterFindStringAction = nullptr;
    QAction *m_findNextAction = nullptr;
    QAction *m_findPreviousAction = nullptr;
    QAction *m_replaceAction = nullptr;
    QAction *m_replaceNextAction = nullptr;
    QAction *m_replacePreviousAction = nullptr;
    QAction *m_replaceAllAction = nullptr;
    QAction *m_caseSensitiveAction = nullptr;
    QAction *m_wholeWordAction = nullptr;
    QAction *m_regularExpressionAction = nullptr;
    QAction *m_preserveCaseAction = nullptr;

    QAction *m_localFindNextAction = nullptr;
    QAction *m_localFindPreviousAction = nullptr;
    QAction *m_localSelectAllAction = nullptr;
    QAction *m_localReplaceAction = nullptr;
    QAction *m_localReplaceNextAction = nullptr;
    QAction *m_localReplacePreviousAction = nullptr;
    QAction *m_localReplaceAllAction = nullptr;

    QLabel *m_findLabel;
    Utils::FancyLineEdit *m_findEdit;
    QHBoxLayout *m_findButtonLayout;
    QToolButton *m_findPreviousButton;
    QToolButton *m_findNextButton;
    QToolButton *m_selectAllButton;
    QSpacerItem *m_horizontalSpacer;
    QToolButton *m_close;
    QLabel *m_replaceLabel;
    Utils::FancyLineEdit *m_replaceEdit;
    QWidget *m_replaceButtonsWidget;
    QToolButton *m_replaceButton;
    QToolButton *m_replaceNextButton;
    QToolButton *m_replaceAllButton;
    QToolButton *m_advancedButton;
    Utils::FindFlags m_findFlags;

    QTimer m_findIncrementalTimer;
    QTimer m_findStepTimer;
    IFindSupport::Result m_lastResult = IFindSupport::NotYetFound;
    bool m_useFakeVim = false;
    bool m_eventFiltersInstalled = false;
    bool m_findEnabled = true;
};

} // namespace Internal
} // namespace Core
