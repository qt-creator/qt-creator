/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "findtoolbar.h"
#include "ifindfilter.h"
#include "findplugin.h"

#include <coreplugin/coreicons.h>
#include <coreplugin/coreplugin.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/findplaceholder.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QDebug>
#include <QSettings>

#include <QCheckBox>
#include <QClipboard>
#include <QCompleter>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QStringListModel>

Q_DECLARE_METATYPE(QStringList)
Q_DECLARE_METATYPE(Core::IFindFilter*)

static const int MINIMUM_WIDTH_FOR_COMPLEX_LAYOUT = 150;
static const int FINDBUTTON_SPACER_WIDTH = 20;

namespace Core {
namespace Internal {

FindToolBar::FindToolBar(CurrentDocumentFind *currentDocumentFind)
    : m_currentDocumentFind(currentDocumentFind),
      m_findCompleter(new QCompleter(this)),
      m_replaceCompleter(new QCompleter(this)),
      m_findIncrementalTimer(this),
      m_findStepTimer(this)
{
    //setup ui
    m_ui.setupUi(this);
    // compensate for a vertically expanding spacer below the label
    m_ui.replaceLabel->setMinimumHeight(m_ui.replaceEdit->sizeHint().height());
    m_ui.mainLayout->setColumnStretch(1, 10);

    setFocusProxy(m_ui.findEdit);
    setProperty("topBorder", true);
    setSingleRow(false);
    m_ui.findEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_ui.replaceEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_ui.replaceEdit->setFiltering(true);

    connect(m_ui.findEdit, &Utils::FancyLineEdit::editingFinished,
            this, &FindToolBar::invokeResetIncrementalSearch);

    setLightColoredIcon(false);
    connect(m_ui.close, &QToolButton::clicked,
            this, &FindToolBar::hideAndResetFocus);

    m_findCompleter->setModel(Find::findCompletionModel());
    m_replaceCompleter->setModel(Find::replaceCompletionModel());
    m_ui.findEdit->setSpecialCompleter(m_findCompleter);
    m_ui.replaceEdit->setSpecialCompleter(m_replaceCompleter);

    m_ui.findEdit->setButtonVisible(Utils::FancyLineEdit::Left, true);
    m_ui.findEdit->setFiltering(true);
    m_ui.findEdit->setPlaceholderText(QString());
    m_ui.findEdit->setOkColor(Utils::creatorTheme()->color(Utils::Theme::TextColorNormal));
    m_ui.findEdit->setErrorColor(Utils::creatorTheme()->color(Utils::Theme::TextColorError));
    m_ui.findEdit->button(Utils::FancyLineEdit::Left)->setFocusPolicy(Qt::TabFocus);
    m_ui.findEdit->setValidationFunction([this](Utils::FancyLineEdit *, QString *) {
                                             return m_lastResult != IFindSupport::NotFound;
                                         });
    m_ui.replaceEdit->setPlaceholderText(QString());

    connect(m_ui.findEdit, &Utils::FancyLineEdit::textChanged,
            this, &FindToolBar::invokeFindIncremental);
    connect(m_ui.findEdit, &Utils::FancyLineEdit::leftButtonClicked,
            this, &FindToolBar::findEditButtonClicked);

    // invoke{Find,Replace}Helper change the completion model. QueuedConnection is used to perform these
    // changes only after the completer's activated() signal is handled (QTCREATORBUG-8408)
    connect(m_ui.findEdit, &Utils::FancyLineEdit::returnPressed,
            this, &FindToolBar::invokeFindEnter, Qt::QueuedConnection);
    connect(m_ui.replaceEdit, &Utils::FancyLineEdit::returnPressed,
            this, &FindToolBar::invokeReplaceEnter, Qt::QueuedConnection);

    QAction *shiftEnterAction = new QAction(m_ui.findEdit);
    shiftEnterAction->setShortcut(QKeySequence(tr("Shift+Enter")));
    shiftEnterAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftEnterAction, &QAction::triggered,
            this, &FindToolBar::invokeFindPrevious);
    m_ui.findEdit->addAction(shiftEnterAction);
    QAction *shiftReturnAction = new QAction(m_ui.findEdit);
    shiftReturnAction->setShortcut(QKeySequence(tr("Shift+Return")));
    shiftReturnAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftReturnAction, &QAction::triggered,
            this, &FindToolBar::invokeFindPrevious);
    m_ui.findEdit->addAction(shiftReturnAction);

    QAction *shiftEnterReplaceAction = new QAction(m_ui.replaceEdit);
    shiftEnterReplaceAction->setShortcut(QKeySequence(tr("Shift+Enter")));
    shiftEnterReplaceAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftEnterReplaceAction, &QAction::triggered,
            this, &FindToolBar::invokeReplacePrevious);
    m_ui.replaceEdit->addAction(shiftEnterReplaceAction);
    QAction *shiftReturnReplaceAction = new QAction(m_ui.replaceEdit);
    shiftReturnReplaceAction->setShortcut(QKeySequence(tr("Shift+Return")));
    shiftReturnReplaceAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftReturnReplaceAction, &QAction::triggered,
            this, &FindToolBar::invokeReplacePrevious);
    m_ui.replaceEdit->addAction(shiftReturnReplaceAction);

    // need to make sure QStringList is registered as metatype
    QMetaTypeId<QStringList>::qt_metatype_id();

    // register actions
    Context findcontext(Constants::C_FINDTOOLBAR);
    ActionContainer *mfind = ActionManager::actionContainer(Constants::M_FIND);
    Command *cmd;

    m_ui.advancedButton->setDefaultAction(ActionManager::command(Constants::ADVANCED_FIND)->action());

    m_goToCurrentFindAction = new QAction(this);
    ActionManager::registerAction(m_goToCurrentFindAction, Constants::S_RETURNTOEDITOR,
                                        findcontext);
    connect(m_goToCurrentFindAction, &QAction::triggered,
            this, &FindToolBar::setFocusToCurrentFindSupport);

    QIcon icon = QIcon::fromTheme(QLatin1String("edit-find-replace"));
    m_findInDocumentAction = new QAction(icon, tr("Find/Replace"), this);
    cmd = ActionManager::registerAction(m_findInDocumentAction, Constants::FIND_IN_DOCUMENT);
    cmd->setDefaultKeySequence(QKeySequence::Find);
    mfind->addAction(cmd, Constants::G_FIND_CURRENTDOCUMENT);
    connect(m_findInDocumentAction, &QAction::triggered, this, [this]() { openFind(); });

    // Pressing the find shortcut while focus is in the tool bar should not change the search text,
    // so register a different find action for the tool bar
    auto localFindAction = new QAction(this);
    ActionManager::registerAction(localFindAction, Constants::FIND_IN_DOCUMENT, findcontext);
    connect(localFindAction, &QAction::triggered, this, [this]() {
        openFindToolBar(FindToolBar::OpenFlags(UpdateAll & ~UpdateFindText));
    });

    if (QApplication::clipboard()->supportsFindBuffer()) {
        m_enterFindStringAction = new QAction(tr("Enter Find String"), this);
        cmd = ActionManager::registerAction(m_enterFindStringAction, "Find.EnterFindString");
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E")));
        mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
        connect(m_enterFindStringAction, &QAction::triggered,
                this, &FindToolBar::putSelectionToFindClipboard);
        connect(QApplication::clipboard(), &QClipboard::findBufferChanged, this, &FindToolBar::updateFromFindClipboard);
    }

    m_findNextAction = new QAction(tr("Find Next"), this);
    cmd = ActionManager::registerAction(m_findNextAction, Constants::FIND_NEXT);
    cmd->setDefaultKeySequence(QKeySequence::FindNext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findNextAction, &QAction::triggered, this, &FindToolBar::invokeGlobalFindNext);
    m_localFindNextAction = new QAction(m_findNextAction->text(), this);
    cmd = ActionManager::registerAction(m_localFindNextAction, Constants::FIND_NEXT, findcontext);
    cmd->augmentActionWithShortcutToolTip(m_localFindNextAction);
    connect(m_localFindNextAction, &QAction::triggered, this, &FindToolBar::invokeFindNext);
    m_ui.findNextButton->setDefaultAction(m_localFindNextAction);

    m_findPreviousAction = new QAction(tr("Find Previous"), this);
    cmd = ActionManager::registerAction(m_findPreviousAction, Constants::FIND_PREVIOUS);
    cmd->setDefaultKeySequence(QKeySequence::FindPrevious);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findPreviousAction, &QAction::triggered, this, &FindToolBar::invokeGlobalFindPrevious);
    m_localFindPreviousAction = new QAction(m_findPreviousAction->text(), this);
    cmd = ActionManager::registerAction(m_localFindPreviousAction, Constants::FIND_PREVIOUS, findcontext);
    cmd->augmentActionWithShortcutToolTip(m_localFindPreviousAction);
    connect(m_localFindPreviousAction, &QAction::triggered, this, &FindToolBar::invokeFindPrevious);
    m_ui.findPreviousButton->setDefaultAction(m_localFindPreviousAction);

    m_findNextSelectedAction = new QAction(tr("Find Next (Selected)"), this);
    cmd = ActionManager::registerAction(m_findNextSelectedAction, Constants::FIND_NEXT_SELECTED);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F3")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findNextSelectedAction, &QAction::triggered, this, &FindToolBar::findNextSelected);

    m_findPreviousSelectedAction = new QAction(tr("Find Previous (Selected)"), this);
    cmd = ActionManager::registerAction(m_findPreviousSelectedAction, Constants::FIND_PREV_SELECTED);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F3")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findPreviousSelectedAction, &QAction::triggered,
            this, &FindToolBar::findPreviousSelected);

    m_replaceAction = new QAction(tr("Replace"), this);
    cmd = ActionManager::registerAction(m_replaceAction, Constants::REPLACE);
    cmd->setDefaultKeySequence(QKeySequence());
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceAction, &QAction::triggered, this, &FindToolBar::invokeGlobalReplace);
    m_localReplaceAction = new QAction(m_replaceAction->text(), this);
    cmd = ActionManager::registerAction(m_localReplaceAction, Constants::REPLACE, findcontext);
    cmd->augmentActionWithShortcutToolTip(m_localReplaceAction);
    connect(m_localReplaceAction, &QAction::triggered, this, &FindToolBar::invokeReplace);
    m_ui.replaceButton->setDefaultAction(m_localReplaceAction);

    m_replaceNextAction = new QAction(tr("Replace && Find"), this);
    cmd = ActionManager::registerAction(m_replaceNextAction, Constants::REPLACE_NEXT);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+=")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceNextAction, &QAction::triggered, this, &FindToolBar::invokeGlobalReplaceNext);
    m_localReplaceNextAction = new QAction(m_replaceNextAction->text(), this);
    m_localReplaceNextAction->setIconText(m_replaceNextAction->text()); // Workaround QTBUG-23396
    cmd = ActionManager::registerAction(m_localReplaceNextAction, Constants::REPLACE_NEXT, findcontext);
    cmd->augmentActionWithShortcutToolTip(m_localReplaceNextAction);
    connect(m_localReplaceNextAction, &QAction::triggered, this, &FindToolBar::invokeReplaceNext);
    m_ui.replaceNextButton->setDefaultAction(m_localReplaceNextAction);

    m_replacePreviousAction = new QAction(tr("Replace && Find Previous"), this);
    cmd = ActionManager::registerAction(m_replacePreviousAction, Constants::REPLACE_PREVIOUS);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replacePreviousAction, &QAction::triggered,
            this, &FindToolBar::invokeGlobalReplacePrevious);
    m_localReplacePreviousAction = new QAction(m_replacePreviousAction->text(), this);
    cmd = ActionManager::registerAction(m_localReplacePreviousAction, Constants::REPLACE_PREVIOUS, findcontext);
    cmd->augmentActionWithShortcutToolTip(m_localReplacePreviousAction);
    connect(m_localReplacePreviousAction, &QAction::triggered,
            this, &FindToolBar::invokeReplacePrevious);

    m_replaceAllAction = new QAction(tr("Replace All"), this);
    cmd = ActionManager::registerAction(m_replaceAllAction, Constants::REPLACE_ALL);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceAllAction, &QAction::triggered, this, &FindToolBar::invokeGlobalReplaceAll);
    m_localReplaceAllAction = new QAction(m_replaceAllAction->text(), this);
    cmd = ActionManager::registerAction(m_localReplaceAllAction, Constants::REPLACE_ALL, findcontext);
    cmd->augmentActionWithShortcutToolTip(m_localReplaceAllAction);
    connect(m_localReplaceAllAction, &QAction::triggered, this, &FindToolBar::invokeReplaceAll);
    m_ui.replaceAllButton->setDefaultAction(m_localReplaceAllAction);

    m_caseSensitiveAction = new QAction(tr("Case Sensitive"), this);
    m_caseSensitiveAction->setIcon(Icons::FIND_CASE_INSENSITIVELY.icon());
    m_caseSensitiveAction->setCheckable(true);
    m_caseSensitiveAction->setChecked(false);
    cmd = ActionManager::registerAction(m_caseSensitiveAction, Constants::CASE_SENSITIVE);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_caseSensitiveAction, &QAction::toggled, this, &FindToolBar::setCaseSensitive);

    m_wholeWordAction = new QAction(tr("Whole Words Only"), this);
    m_wholeWordAction->setIcon(Icons::FIND_WHOLE_WORD.icon());
    m_wholeWordAction->setCheckable(true);
    m_wholeWordAction->setChecked(false);
    cmd = ActionManager::registerAction(m_wholeWordAction, Constants::WHOLE_WORDS);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_wholeWordAction, &QAction::toggled, this, &FindToolBar::setWholeWord);

    m_regularExpressionAction = new QAction(tr("Use Regular Expressions"), this);
    m_regularExpressionAction->setIcon(Icons::FIND_REGEXP.icon());
    m_regularExpressionAction->setCheckable(true);
    m_regularExpressionAction->setChecked(false);
    cmd = ActionManager::registerAction(m_regularExpressionAction, Constants::REGULAR_EXPRESSIONS);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_regularExpressionAction, &QAction::toggled, this, &FindToolBar::setRegularExpressions);

    m_preserveCaseAction = new QAction(tr("Preserve Case when Replacing"), this);
    m_preserveCaseAction->setIcon(Icons::FIND_PRESERVE_CASE.icon());
    m_preserveCaseAction->setCheckable(true);
    m_preserveCaseAction->setChecked(false);
    cmd = ActionManager::registerAction(m_preserveCaseAction, Constants::PRESERVE_CASE);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_preserveCaseAction, &QAction::toggled, this, &FindToolBar::setPreserveCase);

    connect(m_currentDocumentFind, &CurrentDocumentFind::candidateChanged,
            this, &FindToolBar::adaptToCandidate);
    connect(m_currentDocumentFind, &CurrentDocumentFind::changed,
            this, &FindToolBar::updateGlobalActions);
    connect(m_currentDocumentFind, &CurrentDocumentFind::changed, this, &FindToolBar::updateToolBar);
    updateGlobalActions();
    updateToolBar();

    m_findIncrementalTimer.setSingleShot(true);
    m_findStepTimer.setSingleShot(true);
    connect(&m_findIncrementalTimer, &QTimer::timeout, this, &FindToolBar::invokeFindIncremental);
    connect(&m_findStepTimer, &QTimer::timeout, this, &FindToolBar::invokeFindStep);
}

FindToolBar::~FindToolBar()
{
}

void FindToolBar::installEventFilters()
{
    if (!m_eventFiltersInstalled) {
        m_findCompleter->popup()->installEventFilter(this);
        m_ui.findEdit->installEventFilter(this);
        m_ui.replaceEdit->installEventFilter(this);
        this->installEventFilter(this);
        m_eventFiltersInstalled = true;
    }
}

bool FindToolBar::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Down) {
            if (obj == m_ui.findEdit) {
                if (m_ui.findEdit->text().isEmpty())
                    m_findCompleter->setCompletionPrefix(QString());
                m_findCompleter->complete();
            } else if (obj == m_ui.replaceEdit) {
                if (m_ui.replaceEdit->text().isEmpty())
                    m_replaceCompleter->setCompletionPrefix(QString());
                m_replaceCompleter->complete();
            }
        }
    }

    if ((obj == m_ui.findEdit || obj == m_findCompleter->popup())
               && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Space && (ke->modifiers() & Utils::HostOsInfo::controlModifier())) {
            QString completedText = m_currentDocumentFind->completedFindString();
            if (!completedText.isEmpty()) {
                setFindText(completedText);
                ke->accept();
                return true;
            }
        }
    } else if (obj == this && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Space && (ke->modifiers() & Utils::HostOsInfo::controlModifier())) {
            event->accept();
            return true;
        }
    } else if (obj == this && event->type() == QEvent::Hide) {
        invokeClearResults();
        if (m_currentDocumentFind->isEnabled())
            m_currentDocumentFind->clearFindScope();
    }
    return Utils::StyledBar::eventFilter(obj, event);
}

void FindToolBar::adaptToCandidate()
{
    updateGlobalActions();
    if (findToolBarPlaceHolder() == FindToolBarPlaceHolder::getCurrent()) {
        m_currentDocumentFind->acceptCandidate();
        if (isVisible() && m_currentDocumentFind->isEnabled())
            m_currentDocumentFind->highlightAll(getFindText(), effectiveFindFlags());
    }
}

void FindToolBar::updateGlobalActions()
{
    IFindSupport *candidate = m_currentDocumentFind->candidate();
    bool enabled = (candidate != 0);
    bool replaceEnabled = enabled && candidate->supportsReplace();
    m_findInDocumentAction->setEnabled(enabled || (toolBarHasFocus() && isEnabled()));
    m_findNextSelectedAction->setEnabled(enabled);
    m_findPreviousSelectedAction->setEnabled(enabled);
    if (m_enterFindStringAction)
        m_enterFindStringAction->setEnabled(enabled);
    m_findNextAction->setEnabled(enabled);
    m_findPreviousAction->setEnabled(enabled);
    m_replaceAction->setEnabled(replaceEnabled);
    m_replaceNextAction->setEnabled(replaceEnabled);
    m_replacePreviousAction->setEnabled(replaceEnabled);
    m_replaceAllAction->setEnabled(replaceEnabled);
}

void FindToolBar::updateToolBar()
{
    bool enabled = m_currentDocumentFind->isEnabled();
    bool replaceEnabled = enabled && m_currentDocumentFind->supportsReplace();
    bool showAllControls = canShowAllControls(replaceEnabled);

    m_localFindNextAction->setEnabled(enabled);
    m_localFindPreviousAction->setEnabled(enabled);

    m_localReplaceAction->setEnabled(replaceEnabled);
    m_localReplacePreviousAction->setEnabled(replaceEnabled);
    m_localReplaceNextAction->setEnabled(replaceEnabled);
    m_localReplaceAllAction->setEnabled(replaceEnabled);

    m_caseSensitiveAction->setEnabled(enabled);
    m_wholeWordAction->setEnabled(enabled);
    m_regularExpressionAction->setEnabled(enabled);
    m_preserveCaseAction->setEnabled(replaceEnabled && !hasFindFlag(FindRegularExpression));
    bool replaceFocus = m_ui.replaceEdit->hasFocus();

    m_ui.findLabel->setEnabled(enabled);
    m_ui.findLabel->setVisible(showAllControls);
    m_ui.findEdit->setEnabled(enabled);
    m_ui.findEdit->setPlaceholderText(showAllControls ? QString() : tr("Search for..."));
    m_ui.findPreviousButton->setVisible(showAllControls);
    m_ui.findNextButton->setVisible(showAllControls);
    m_ui.horizontalSpacer->changeSize((showAllControls ? FINDBUTTON_SPACER_WIDTH : 0), 0,
                                      QSizePolicy::Expanding, QSizePolicy::Ignored);
    m_ui.findButtonLayout->invalidate(); // apply spacer change

    m_ui.replaceLabel->setEnabled(replaceEnabled);
    m_ui.replaceLabel->setVisible(replaceEnabled && showAllControls);
    m_ui.replaceEdit->setEnabled(replaceEnabled);
    m_ui.replaceEdit->setPlaceholderText(showAllControls ? QString() : tr("Replace with..."));
    m_ui.replaceEdit->setVisible(replaceEnabled);
    m_ui.replaceButtonsWidget->setVisible(replaceEnabled && showAllControls);
    m_ui.advancedButton->setVisible(replaceEnabled && showAllControls);

    layout()->invalidate();

    if (!replaceEnabled && enabled && replaceFocus)
        m_ui.findEdit->setFocus();
    updateIcons();
    updateFlagMenus();
}

void FindToolBar::invokeFindEnter()
{
    if (m_currentDocumentFind->isEnabled()) {
        if (m_useFakeVim)
            setFocusToCurrentFindSupport();
        else
            invokeFindNext();
    }
}

void FindToolBar::invokeReplaceEnter()
{
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace())
        invokeReplaceNext();
}

void FindToolBar::invokeClearResults()
{
    if (m_currentDocumentFind->isEnabled())
        m_currentDocumentFind->clearHighlights();
}


void FindToolBar::invokeFindNext()
{
    setFindFlag(FindBackward, false);
    invokeFindStep();
}

void FindToolBar::invokeGlobalFindNext()
{
    if (getFindText().isEmpty()) {
        openFind();
    } else {
        acceptCandidateAndMoveToolBar();
        invokeFindNext();
    }
}

void FindToolBar::invokeFindPrevious()
{
    setFindFlag(FindBackward, true);
    invokeFindStep();
}

void FindToolBar::invokeGlobalFindPrevious()
{
    if (getFindText().isEmpty()) {
        openFind();
    } else {
        acceptCandidateAndMoveToolBar();
        invokeFindPrevious();
    }
}

QString FindToolBar::getFindText()
{
    return m_ui.findEdit->text();
}

QString FindToolBar::getReplaceText()
{
    return m_ui.replaceEdit->text();
}

void FindToolBar::setFindText(const QString &text)
{
    disconnect(m_ui.findEdit, &Utils::FancyLineEdit::textChanged,
               this, &FindToolBar::invokeFindIncremental);
    if (hasFindFlag(FindRegularExpression))
        m_ui.findEdit->setText(QRegExp::escape(text));
    else
        m_ui.findEdit->setText(text);
    connect(m_ui.findEdit, &Utils::FancyLineEdit::textChanged,
            this, &FindToolBar::invokeFindIncremental);
}

void FindToolBar::selectFindText()
{
    m_ui.findEdit->selectAll();
}

void FindToolBar::invokeFindStep()
{
    m_findStepTimer.stop();
    m_findIncrementalTimer.stop();
    if (m_currentDocumentFind->isEnabled()) {
        Find::updateFindCompletion(getFindText());
        IFindSupport::Result result =
            m_currentDocumentFind->findStep(getFindText(), effectiveFindFlags());
        indicateSearchState(result);
        if (result == IFindSupport::NotYetFound)
            m_findStepTimer.start(50);
    }
}

void FindToolBar::invokeFindIncremental()
{
    m_findIncrementalTimer.stop();
    m_findStepTimer.stop();
    if (m_currentDocumentFind->isEnabled()) {
        QString text = getFindText();
        IFindSupport::Result result =
            m_currentDocumentFind->findIncremental(text, effectiveFindFlags());
        indicateSearchState(result);
        if (result == IFindSupport::NotYetFound)
            m_findIncrementalTimer.start(50);
        if (text.isEmpty())
            m_currentDocumentFind->clearHighlights();
    }
}

void FindToolBar::invokeReplace()
{
    setFindFlag(FindBackward, false);
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        Find::updateFindCompletion(getFindText());
        Find::updateReplaceCompletion(getReplaceText());
        m_currentDocumentFind->replace(getFindText(), getReplaceText(), effectiveFindFlags());
    }
}

void FindToolBar::invokeGlobalReplace()
{
    acceptCandidateAndMoveToolBar();
    invokeReplace();
}

void FindToolBar::invokeReplaceNext()
{
    setFindFlag(FindBackward, false);
    invokeReplaceStep();
}

void FindToolBar::invokeGlobalReplaceNext()
{
    acceptCandidateAndMoveToolBar();
    invokeReplaceNext();
}

void FindToolBar::invokeReplacePrevious()
{
    setFindFlag(FindBackward, true);
    invokeReplaceStep();
}

void FindToolBar::invokeGlobalReplacePrevious()
{
    acceptCandidateAndMoveToolBar();
    invokeReplacePrevious();
}

void FindToolBar::invokeReplaceStep()
{
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        Find::updateFindCompletion(getFindText());
        Find::updateReplaceCompletion(getReplaceText());
        m_currentDocumentFind->replaceStep(getFindText(), getReplaceText(), effectiveFindFlags());
    }
}

void FindToolBar::invokeReplaceAll()
{
    Find::updateFindCompletion(getFindText());
    Find::updateReplaceCompletion(getReplaceText());
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace())
        m_currentDocumentFind->replaceAll(getFindText(), getReplaceText(), effectiveFindFlags());
}

void FindToolBar::invokeGlobalReplaceAll()
{
    acceptCandidateAndMoveToolBar();
    invokeReplaceAll();
}

void FindToolBar::invokeResetIncrementalSearch()
{
    m_findIncrementalTimer.stop();
    m_findStepTimer.stop();
    if (m_currentDocumentFind->isEnabled())
        m_currentDocumentFind->resetIncrementalSearch();
}


void FindToolBar::putSelectionToFindClipboard()
{
    openFind(false);
    const QString text = m_currentDocumentFind->currentFindString();
    QApplication::clipboard()->setText(text, QClipboard::FindBuffer);
}


void FindToolBar::updateFromFindClipboard()
{
    if (QApplication::clipboard()->supportsFindBuffer()) {
        const bool blocks = m_ui.findEdit->blockSignals(true);
        setFindText(QApplication::clipboard()->text(QClipboard::FindBuffer));
        m_ui.findEdit->blockSignals(blocks);
    }
}

void FindToolBar::findFlagsChanged()
{
    updateIcons();
    updateFlagMenus();
    invokeClearResults();
    if (isVisible())
        m_currentDocumentFind->highlightAll(getFindText(), effectiveFindFlags());
}

void FindToolBar::findEditButtonClicked()
{
    OptionsPopup *popup = new OptionsPopup(m_ui.findEdit);
    popup->show();
}

void FindToolBar::updateIcons()
{
    FindFlags effectiveFlags = effectiveFindFlags();
    bool casesensitive = effectiveFlags & FindCaseSensitively;
    bool wholewords = effectiveFlags & FindWholeWords;
    bool regexp = effectiveFlags & FindRegularExpression;
    bool preserveCase = effectiveFlags & FindPreserveCase;
    if (!casesensitive && !wholewords && !regexp && !preserveCase) {
        const QPixmap pixmap = Utils::Icons::MAGNIFIER.pixmap();
        m_ui.findEdit->setButtonPixmap(Utils::FancyLineEdit::Left, pixmap);
    } else {
        m_ui.findEdit->setButtonPixmap(Utils::FancyLineEdit::Left,
                                       IFindFilter::pixmapForFindFlags(effectiveFlags));
    }
}

FindFlags FindToolBar::effectiveFindFlags()
{
    FindFlags supportedFlags;
    bool supportsReplace = true;
    if (m_currentDocumentFind->isEnabled()) {
        supportedFlags = m_currentDocumentFind->supportedFindFlags();
        supportsReplace = m_currentDocumentFind->supportsReplace();
    } else {
        supportedFlags = (FindFlags)0xFFFFFF;
    }
    if (!supportsReplace || m_findFlags & FindRegularExpression)
        supportedFlags &= ~FindPreserveCase;
    return supportedFlags & m_findFlags;
}

void FindToolBar::updateFlagMenus()
{
    bool wholeOnly = ((m_findFlags & FindWholeWords));
    bool sensitive = ((m_findFlags & FindCaseSensitively));
    bool regexp = ((m_findFlags & FindRegularExpression));
    bool preserveCase = ((m_findFlags & FindPreserveCase));
    if (m_wholeWordAction->isChecked() != wholeOnly)
        m_wholeWordAction->setChecked(wholeOnly);
    if (m_caseSensitiveAction->isChecked() != sensitive)
        m_caseSensitiveAction->setChecked(sensitive);
    if (m_regularExpressionAction->isChecked() != regexp)
        m_regularExpressionAction->setChecked(regexp);
    if (m_preserveCaseAction->isChecked() != preserveCase)
        m_preserveCaseAction->setChecked(preserveCase);
    FindFlags supportedFlags;
    if (m_currentDocumentFind->isEnabled())
        supportedFlags = m_currentDocumentFind->supportedFindFlags();
    m_wholeWordAction->setEnabled(supportedFlags & FindWholeWords);
    m_caseSensitiveAction->setEnabled(supportedFlags & FindCaseSensitively);
    m_regularExpressionAction->setEnabled(supportedFlags & FindRegularExpression);
    bool replaceEnabled = m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace();
    m_preserveCaseAction->setEnabled((supportedFlags & FindPreserveCase) && !regexp && replaceEnabled);
}

void FindToolBar::setFocusToCurrentFindSupport()
{
    if (!m_currentDocumentFind->setFocusToCurrentFindSupport())
        if (QWidget *w = focusWidget())
            w->clearFocus();
}

void FindToolBar::hideAndResetFocus()
{
    m_currentDocumentFind->setFocusToCurrentFindSupport();
    hide();
}

FindToolBarPlaceHolder *FindToolBar::findToolBarPlaceHolder() const
{
    QList<FindToolBarPlaceHolder*> placeholders = ExtensionSystem::PluginManager::getObjects<FindToolBarPlaceHolder>();
    QWidget *candidate = QApplication::focusWidget();
    while (candidate) {
        foreach (FindToolBarPlaceHolder *ph, placeholders) {
            if (ph->owner() == candidate)
                return ph;
        }
        candidate = candidate->parentWidget();
    }
    return 0;
}

bool FindToolBar::toolBarHasFocus() const
{
    return qApp->focusWidget() == focusWidget();
}

bool FindToolBar::canShowAllControls(bool replaceIsVisible) const
{
    int fullWidth = width();
    int findFixedWidth = m_ui.findLabel->sizeHint().width()
            + m_ui.findNextButton->sizeHint().width()
            + m_ui.findPreviousButton->sizeHint().width()
            + FINDBUTTON_SPACER_WIDTH
            + m_ui.close->sizeHint().width();
    if (fullWidth - findFixedWidth < MINIMUM_WIDTH_FOR_COMPLEX_LAYOUT)
        return false;
    if (!replaceIsVisible)
        return true;
    int replaceFixedWidth = m_ui.replaceLabel->sizeHint().width()
            + m_ui.replaceButton->sizeHint().width()
            + m_ui.replaceNextButton->sizeHint().width()
            + m_ui.replaceAllButton->sizeHint().width()
            + m_ui.advancedButton->sizeHint().width();
    return fullWidth - replaceFixedWidth >= MINIMUM_WIDTH_FOR_COMPLEX_LAYOUT;
}

/*!
    Accepts the candidate find of the current focus widget (if any), and moves the tool bar
    there, if it was visible before.
*/
void FindToolBar::acceptCandidateAndMoveToolBar()
{
    if (!m_currentDocumentFind->candidate())
        return;
    if (isVisible()) {
        openFindToolBar(UpdateHighlight);
    } else {
        // Make sure we are really hidden, and not just because our parent was hidden.
        // Otherwise when the tool bar gets visible again, it will be in a different widget than
        // the current document find it acts on.
        // Test case: Open find in navigation side bar, hide side bar, click into editor,
        // trigger find next, show side bar
        hide();
        m_currentDocumentFind->acceptCandidate();
    }
}

void FindToolBar::indicateSearchState(IFindSupport::Result searchState)
{
    m_lastResult = searchState;
    m_ui.findEdit->validate();
}

void FindToolBar::openFind(bool focus)
{
    setBackward(false);
    OpenFlags flags = UpdateAll;
    if (!focus) // remove focus flag
        flags = flags & ~UpdateFocusAndSelect;
    openFindToolBar(flags);
}

void FindToolBar::openFindToolBar(OpenFlags flags)
{
    installEventFilters();
    FindToolBarPlaceHolder *holder = findToolBarPlaceHolder();
    if (!holder)
        return;
    FindToolBarPlaceHolder *previousHolder = FindToolBarPlaceHolder::getCurrent();
    if (previousHolder != holder) {
        if (previousHolder)
            previousHolder->setWidget(0);
        holder->setWidget(this);
        FindToolBarPlaceHolder::setCurrent(holder);
    }
    m_currentDocumentFind->acceptCandidate();
    holder->setVisible(true);
    setVisible(true);
//     We do not want to change the text when we currently have the focus and user presses the
//     find shortcut
//    if (!focus || !toolBarHasFocus()) {
    if (flags & UpdateFindText) {
        QString text = m_currentDocumentFind->currentFindString();
        if (!text.isEmpty())
            setFindText(text);
    }
    if (flags & UpdateFocusAndSelect)
        setFocus();
    if (flags & UpdateFindScope)
        m_currentDocumentFind->defineFindScope();
    if (flags & UpdateHighlight)
        m_currentDocumentFind->highlightAll(getFindText(), effectiveFindFlags());
    if (flags & UpdateFocusAndSelect)
        selectFindText();
}

void FindToolBar::findNextSelected()
{
    openFindToolBar(OpenFlags(UpdateAll & ~UpdateFocusAndSelect));
    invokeFindNext();
}

void FindToolBar::findPreviousSelected()
{
    openFindToolBar(OpenFlags(UpdateAll & ~UpdateFocusAndSelect));
    invokeFindPrevious();
}

bool FindToolBar::focusNextPrevChild(bool next)
{
    QAbstractButton *optionsButton = m_ui.findEdit->button(Utils::FancyLineEdit::Left);
    // close tab order
    if (next && m_ui.advancedButton->hasFocus())
        optionsButton->setFocus(Qt::TabFocusReason);
    else if (next && optionsButton->hasFocus())
        m_ui.findEdit->setFocus(Qt::TabFocusReason);
    else if (!next && optionsButton->hasFocus())
        m_ui.advancedButton->setFocus(Qt::TabFocusReason);
    else if (!next && m_ui.findEdit->hasFocus())
        optionsButton->setFocus(Qt::TabFocusReason);
    else
        return Utils::StyledBar::focusNextPrevChild(next);
    return true;
}

void FindToolBar::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    QTimer::singleShot(0, this, &FindToolBar::updateToolBar);
}

void FindToolBar::writeSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    settings->beginGroup(QLatin1String("FindToolBar"));
    settings->setValue(QLatin1String("Backward"), QVariant((m_findFlags & FindBackward) != 0));
    settings->setValue(QLatin1String("CaseSensitively"), QVariant((m_findFlags & FindCaseSensitively) != 0));
    settings->setValue(QLatin1String("WholeWords"), QVariant((m_findFlags & FindWholeWords) != 0));
    settings->setValue(QLatin1String("RegularExpression"), QVariant((m_findFlags & FindRegularExpression) != 0));
    settings->setValue(QLatin1String("PreserveCase"), QVariant((m_findFlags & FindPreserveCase) != 0));
    settings->endGroup();
    settings->endGroup();
}

void FindToolBar::readSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    settings->beginGroup(QLatin1String("FindToolBar"));
    FindFlags flags;
    if (settings->value(QLatin1String("Backward"), false).toBool())
        flags |= FindBackward;
    if (settings->value(QLatin1String("CaseSensitively"), false).toBool())
        flags |= FindCaseSensitively;
    if (settings->value(QLatin1String("WholeWords"), false).toBool())
        flags |= FindWholeWords;
    if (settings->value(QLatin1String("RegularExpression"), false).toBool())
        flags |= FindRegularExpression;
    if (settings->value(QLatin1String("PreserveCase"), false).toBool())
        flags |= FindPreserveCase;
    settings->endGroup();
    settings->endGroup();
    m_findFlags = flags;
    findFlagsChanged();
}

void FindToolBar::setUseFakeVim(bool on)
{
    m_useFakeVim = on;
}

void FindToolBar::setFindFlag(FindFlag flag, bool enabled)
{
    bool hasFlag = hasFindFlag(flag);
    if ((hasFlag && enabled) || (!hasFlag && !enabled))
        return;
    if (enabled)
        m_findFlags |= flag;
    else
        m_findFlags &= ~flag;
    if (flag != FindBackward)
        findFlagsChanged();
}

bool FindToolBar::hasFindFlag(FindFlag flag)
{
    return m_findFlags & flag;
}

void FindToolBar::setCaseSensitive(bool sensitive)
{
    setFindFlag(FindCaseSensitively, sensitive);
}

void FindToolBar::setWholeWord(bool wholeOnly)
{
    setFindFlag(FindWholeWords, wholeOnly);
}

void FindToolBar::setRegularExpressions(bool regexp)
{
    setFindFlag(FindRegularExpression, regexp);
}

void FindToolBar::setPreserveCase(bool preserveCase)
{
    setFindFlag(FindPreserveCase, preserveCase);
}

void FindToolBar::setBackward(bool backward)
{
    setFindFlag(FindBackward, backward);
}

void FindToolBar::setLightColoredIcon(bool lightColored)
{
    if (lightColored) {
        m_ui.findNextButton->setIcon(QIcon());
        m_ui.findNextButton->setArrowType(Qt::RightArrow);
        m_ui.findPreviousButton->setIcon(QIcon());
        m_ui.findPreviousButton->setArrowType(Qt::LeftArrow);
        m_ui.close->setIcon(Utils::Icons::CLOSE_FOREGROUND.icon());
    } else {
        m_ui.findNextButton->setIcon(Utils::Icons::NEXT_TOOLBAR.icon());
        m_ui.findNextButton->setArrowType(Qt::NoArrow);
        m_ui.findPreviousButton->setIcon(Utils::Icons::PREV_TOOLBAR.icon());
        m_ui.findPreviousButton->setArrowType(Qt::NoArrow);
        m_ui.close->setIcon(Utils::Icons::CLOSE_TOOLBAR.icon());
    }
}

OptionsPopup::OptionsPopup(QWidget *parent)
    : QWidget(parent, Qt::Popup)
{
    setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    setLayout(layout);
    QCheckBox *firstCheckBox = createCheckboxForCommand(Constants::CASE_SENSITIVE);
    layout->addWidget(firstCheckBox);
    layout->addWidget(createCheckboxForCommand(Constants::WHOLE_WORDS));
    layout->addWidget(createCheckboxForCommand(Constants::REGULAR_EXPRESSIONS));
    layout->addWidget(createCheckboxForCommand(Constants::PRESERVE_CASE));
    firstCheckBox->setFocus();
    move(parent->mapToGlobal(QPoint(0, -sizeHint().height())));
}

bool OptionsPopup::event(QEvent *ev)
{
    if (ev->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ev->accept();
            return true;
        }
    }
    return QWidget::event(ev);
}

bool OptionsPopup::eventFilter(QObject *obj, QEvent *ev)
{
    QCheckBox *checkbox = qobject_cast<QCheckBox *>(obj);
    if (ev->type() == QEvent::KeyPress && checkbox) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(ev);
        if (!ke->modifiers() && (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return)) {
            checkbox->click();
            ev->accept();
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void OptionsPopup::actionChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    QCheckBox *checkbox = m_checkboxMap.value(action);
    QTC_ASSERT(checkbox, return);
    checkbox->setEnabled(action->isEnabled());
}

QCheckBox *OptionsPopup::createCheckboxForCommand(Id id)
{
    QAction *action = ActionManager::command(id)->action();
    QCheckBox *checkbox = new QCheckBox(action->text());
    checkbox->setToolTip(action->toolTip());
    checkbox->setChecked(action->isChecked());
    checkbox->setEnabled(action->isEnabled());
    checkbox->installEventFilter(this); // enter key handling
    QObject::connect(checkbox, &QCheckBox::clicked, action, &QAction::setChecked);
    QObject::connect(action, &QAction::changed, this, &OptionsPopup::actionChanged);
    m_checkboxMap.insert(action, checkbox);
    return checkbox;
}

} // namespace Internal
} // namespace Core
