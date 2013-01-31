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

#include "findtoolbar.h"
#include "findplugin.h"
#include "textfindconstants.h"
#include "ifindfilter.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/id.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>
#include <utils/flowlayout.h>

#include <QDebug>
#include <QSettings>

#include <QClipboard>
#include <QCompleter>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QPainter>
#include <QPixmapCache>
#include <QStringListModel>

Q_DECLARE_METATYPE(QStringList)
Q_DECLARE_METATYPE(Find::IFindFilter*)

using namespace Find;
using namespace Find::Internal;

FindToolBar::FindToolBar(FindPlugin *plugin, CurrentDocumentFind *currentDocumentFind)
    : m_plugin(plugin),
      m_currentDocumentFind(currentDocumentFind),
      m_findCompleter(new QCompleter(this)),
      m_replaceCompleter(new QCompleter(this)),
      m_enterFindStringAction(0),
      m_findNextAction(0),
      m_findPreviousAction(0),
      m_replaceAction(0),
      m_replaceNextAction(0),
      m_replacePreviousAction(0),
      m_findIncrementalTimer(this), m_findStepTimer(this),
      m_useFakeVim(false),
      m_eventFiltersInstalled(false)
{
    //setup ui
    m_ui.setupUi(this);
    // compensate for a vertically expanding spacer below the label
    m_ui.replaceLabel->setMinimumHeight(m_ui.replaceEdit->sizeHint().height());
    delete m_ui.replaceButtonsWidget->layout();
    Utils::FlowLayout *flowlayout = new Utils::FlowLayout(m_ui.replaceButtonsWidget, 0, 3, 3);
    flowlayout->addWidget(m_ui.replaceButton);
    flowlayout->addWidget(m_ui.replaceNextButton);
    flowlayout->addWidget(m_ui.replaceAllButton);
    m_ui.replaceButtonsWidget->setLayout(flowlayout);
    setFocusProxy(m_ui.findEdit);
    setProperty("topBorder", true);
    setSingleRow(false);
    m_ui.findEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_ui.replaceEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(m_ui.findEdit, SIGNAL(editingFinished()), this, SLOT(invokeResetIncrementalSearch()));

    m_ui.close->setIcon(QIcon(QLatin1String(Core::Constants::ICON_CLOSE_DOCUMENT)));
    connect(m_ui.close, SIGNAL(clicked()), this, SLOT(hideAndResetFocus()));

    m_findCompleter->setModel(m_plugin->findCompletionModel());
    m_replaceCompleter->setModel(m_plugin->replaceCompletionModel());
    m_ui.findEdit->setSpecialCompleter(m_findCompleter);
    m_ui.replaceEdit->setSpecialCompleter(m_replaceCompleter);

    QMenu *lineEditMenu = new QMenu(m_ui.findEdit);
    m_ui.findEdit->setButtonMenu(Utils::FancyLineEdit::Left, lineEditMenu);
    m_ui.findEdit->setButtonVisible(Utils::FancyLineEdit::Left, true);
    m_ui.findEdit->setPlaceholderText(QString());
    m_ui.replaceEdit->setPlaceholderText(QString());

    connect(m_ui.findEdit, SIGNAL(textChanged(QString)), this, SLOT(invokeFindIncremental()));

    // invoke{Find,Replace}Helper change the completion model. QueuedConnection is used to perform these
    // changes only after the completer's activated() signal is handled (QTCREATORBUG-8408)
    connect(m_ui.findEdit, SIGNAL(returnPressed()), this, SLOT(invokeFindEnter()), Qt::QueuedConnection);
    connect(m_ui.replaceEdit, SIGNAL(returnPressed()), this, SLOT(invokeReplaceEnter()), Qt::QueuedConnection);

    QAction *shiftEnterAction = new QAction(m_ui.findEdit);
    shiftEnterAction->setShortcut(QKeySequence(tr("Shift+Enter")));
    shiftEnterAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftEnterAction, SIGNAL(triggered()), this, SLOT(invokeFindPrevious()));
    m_ui.findEdit->addAction(shiftEnterAction);
    QAction *shiftReturnAction = new QAction(m_ui.findEdit);
    shiftReturnAction->setShortcut(QKeySequence(tr("Shift+Return")));
    shiftReturnAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftReturnAction, SIGNAL(triggered()), this, SLOT(invokeFindPrevious()));
    m_ui.findEdit->addAction(shiftReturnAction);

    QAction *shiftEnterReplaceAction = new QAction(m_ui.replaceEdit);
    shiftEnterReplaceAction->setShortcut(QKeySequence(tr("Shift+Enter")));
    shiftEnterReplaceAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftEnterReplaceAction, SIGNAL(triggered()), this, SLOT(invokeReplacePrevious()));
    m_ui.replaceEdit->addAction(shiftEnterReplaceAction);
    QAction *shiftReturnReplaceAction = new QAction(m_ui.replaceEdit);
    shiftReturnReplaceAction->setShortcut(QKeySequence(tr("Shift+Return")));
    shiftReturnReplaceAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftReturnReplaceAction, SIGNAL(triggered()), this, SLOT(invokeReplacePrevious()));
    m_ui.replaceEdit->addAction(shiftReturnReplaceAction);

    // need to make sure QStringList is registered as metatype
    QMetaTypeId<QStringList>::qt_metatype_id();

    // register actions
    Core::Context globalcontext(Core::Constants::C_GLOBAL);
    Core::ActionContainer *mfind = Core::ActionManager::actionContainer(Constants::M_FIND);
    Core::Command *cmd;

    m_ui.advancedButton->setDefaultAction(Core::ActionManager::command(Constants::ADVANCED_FIND)->action());

    QIcon icon = QIcon::fromTheme(QLatin1String("edit-find-replace"));
    m_findInDocumentAction = new QAction(icon, tr("Find/Replace"), this);
    cmd = Core::ActionManager::registerAction(m_findInDocumentAction, Constants::FIND_IN_DOCUMENT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence::Find);
    mfind->addAction(cmd, Constants::G_FIND_CURRENTDOCUMENT);
    connect(m_findInDocumentAction, SIGNAL(triggered()), this, SLOT(openFind()));

    if (QApplication::clipboard()->supportsFindBuffer()) {
        m_enterFindStringAction = new QAction(tr("Enter Find String"), this);
        cmd = Core::ActionManager::registerAction(m_enterFindStringAction, "Find.EnterFindString", globalcontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E")));
        mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
        connect(m_enterFindStringAction, SIGNAL(triggered()), this, SLOT(putSelectionToFindClipboard()));
        connect(QApplication::clipboard(), SIGNAL(findBufferChanged()), this, SLOT(updateFromFindClipboard()));
    }

    m_findNextAction = new QAction(tr("Find Next"), this);
    cmd = Core::ActionManager::registerAction(m_findNextAction, Constants::FIND_NEXT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence::FindNext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findNextAction, SIGNAL(triggered()), this, SLOT(invokeFindNext()));
    m_ui.findNextButton->setDefaultAction(cmd->action());

    m_findPreviousAction = new QAction(tr("Find Previous"), this);
    cmd = Core::ActionManager::registerAction(m_findPreviousAction, Constants::FIND_PREVIOUS, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence::FindPrevious);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findPreviousAction, SIGNAL(triggered()), this, SLOT(invokeFindPrevious()));
    m_ui.findPreviousButton->setDefaultAction(cmd->action());

    m_findNextSelectedAction = new QAction(tr("Find Next (Selected)"), this);
    cmd = Core::ActionManager::registerAction(m_findNextSelectedAction, Constants::FIND_NEXT_SELECTED, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+F3")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findNextSelectedAction, SIGNAL(triggered()), this, SLOT(findNextSelected()));

    m_findPreviousSelectedAction = new QAction(tr("Find Previous (Selected)"), this);
    cmd = Core::ActionManager::registerAction(m_findPreviousSelectedAction, Constants::FIND_PREV_SELECTED, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+F3")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findPreviousSelectedAction, SIGNAL(triggered()), this, SLOT(findPreviousSelected()));

    m_replaceAction = new QAction(tr("Replace"), this);
    cmd = Core::ActionManager::registerAction(m_replaceAction, Constants::REPLACE, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence());
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceAction, SIGNAL(triggered()), this, SLOT(invokeReplace()));
    m_ui.replaceButton->setDefaultAction(cmd->action());

    m_replaceNextAction = new QAction(tr("Replace && Find"), this);
    m_replaceNextAction->setIconText(tr("Replace && Find")); // work around bug in Qt that kills ampersands in tool button
    cmd = Core::ActionManager::registerAction(m_replaceNextAction, Constants::REPLACE_NEXT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+=")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceNextAction, SIGNAL(triggered()), this, SLOT(invokeReplaceNext()));
    m_ui.replaceNextButton->setDefaultAction(cmd->action());

    m_replacePreviousAction = new QAction(tr("Replace && Find Previous"), this);
    cmd = Core::ActionManager::registerAction(m_replacePreviousAction, Constants::REPLACE_PREVIOUS, globalcontext);
    // shortcut removed, clashes with Ctrl++ on many keyboard layouts
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+=")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replacePreviousAction, SIGNAL(triggered()), this, SLOT(invokeReplacePrevious()));

    m_replaceAllAction = new QAction(tr("Replace All"), this);
    cmd = Core::ActionManager::registerAction(m_replaceAllAction, Constants::REPLACE_ALL, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceAllAction, SIGNAL(triggered()), this, SLOT(invokeReplaceAll()));
    m_ui.replaceAllButton->setDefaultAction(cmd->action());

    m_caseSensitiveAction = new QAction(tr("Case Sensitive"), this);
    m_caseSensitiveAction->setIcon(QIcon(QLatin1String(":/find/images/casesensitively.png")));
    m_caseSensitiveAction->setCheckable(true);
    m_caseSensitiveAction->setChecked(false);
    cmd = Core::ActionManager::registerAction(m_caseSensitiveAction, Constants::CASE_SENSITIVE, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_caseSensitiveAction, SIGNAL(triggered(bool)), this, SLOT(setCaseSensitive(bool)));
    lineEditMenu->addAction(m_caseSensitiveAction);

    m_wholeWordAction = new QAction(tr("Whole Words Only"), this);
    m_wholeWordAction->setIcon(QIcon(QLatin1String(":/find/images/wholewords.png")));
    m_wholeWordAction->setCheckable(true);
    m_wholeWordAction->setChecked(false);
    cmd = Core::ActionManager::registerAction(m_wholeWordAction, Constants::WHOLE_WORDS, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_wholeWordAction, SIGNAL(triggered(bool)), this, SLOT(setWholeWord(bool)));
    lineEditMenu->addAction(m_wholeWordAction);

    m_regularExpressionAction = new QAction(tr("Use Regular Expressions"), this);
    m_regularExpressionAction->setIcon(QIcon(QLatin1String(":/find/images/regexp.png")));
    m_regularExpressionAction->setCheckable(true);
    m_regularExpressionAction->setChecked(false);
    cmd = Core::ActionManager::registerAction(m_regularExpressionAction, Constants::REGULAR_EXPRESSIONS, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_regularExpressionAction, SIGNAL(triggered(bool)), this, SLOT(setRegularExpressions(bool)));
    lineEditMenu->addAction(m_regularExpressionAction);

    m_preserveCaseAction = new QAction(tr("Preserve Case when Replacing"), this);
    m_preserveCaseAction->setIcon(QPixmap(QLatin1String(":/find/images/preservecase.png")));
    m_preserveCaseAction->setCheckable(true);
    m_preserveCaseAction->setChecked(false);
    cmd = Core::ActionManager::registerAction(m_preserveCaseAction, Constants::PRESERVE_CASE, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_preserveCaseAction, SIGNAL(triggered(bool)), this, SLOT(setPreserveCase(bool)));
    lineEditMenu->addAction(m_preserveCaseAction);

    connect(m_currentDocumentFind, SIGNAL(candidateChanged()), this, SLOT(adaptToCandidate()));
    connect(m_currentDocumentFind, SIGNAL(changed()), this, SLOT(updateToolBar()));
    updateToolBar();

    m_findIncrementalTimer.setSingleShot(true);
    m_findStepTimer.setSingleShot(true);
    connect(&m_findIncrementalTimer, SIGNAL(timeout()),
            this, SLOT(invokeFindIncremental()));
    connect(&m_findStepTimer, SIGNAL(timeout()), this, SLOT(invokeFindStep()));
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
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()
                && !m_findCompleter->popup()->isVisible()
                && !m_replaceCompleter->popup()->isVisible()) {
            if (setFocusToCurrentFindSupport()) {
                event->accept();
                return true;
            }
        } else if (ke->key() == Qt::Key_Space && (ke->modifiers() & Utils::HostOsInfo::controlModifier())) {
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
    updateFindAction();
    if (findToolBarPlaceHolder() == Core::FindToolBarPlaceHolder::getCurrent())
        m_currentDocumentFind->acceptCandidate();
}

void FindToolBar::updateFindAction()
{
    bool enabled = m_currentDocumentFind->candidateIsEnabled();
    m_findInDocumentAction->setEnabled(enabled);
    m_findNextSelectedAction->setEnabled(enabled);
    m_findPreviousSelectedAction->setEnabled(enabled);
}

void FindToolBar::updateToolBar()
{
    bool enabled = m_currentDocumentFind->isEnabled();
    bool replaceEnabled = enabled && m_currentDocumentFind->supportsReplace();
    m_findNextAction->setEnabled(enabled);
    m_findPreviousAction->setEnabled(enabled);

    m_replaceAction->setEnabled(replaceEnabled);
    m_replaceNextAction->setEnabled(replaceEnabled);
    m_replacePreviousAction->setEnabled(replaceEnabled);
    m_replaceAllAction->setEnabled(replaceEnabled);

    m_caseSensitiveAction->setEnabled(enabled);
    m_wholeWordAction->setEnabled(enabled);
    m_regularExpressionAction->setEnabled(enabled);
    m_preserveCaseAction->setEnabled(replaceEnabled && !hasFindFlag(Find::FindRegularExpression));
    if (QApplication::clipboard()->supportsFindBuffer())
        m_enterFindStringAction->setEnabled(enabled);
    bool replaceFocus = m_ui.replaceEdit->hasFocus();
    m_ui.findEdit->setEnabled(enabled);
    m_ui.findLabel->setEnabled(enabled);

    m_ui.replaceEdit->setEnabled(replaceEnabled);
    m_ui.replaceLabel->setEnabled(replaceEnabled);
    m_ui.replaceEdit->setVisible(replaceEnabled);
    m_ui.replaceLabel->setVisible(replaceEnabled);
    m_ui.replaceButtonsWidget->setVisible(replaceEnabled);
    m_ui.advancedButton->setVisible(replaceEnabled);
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
        m_currentDocumentFind->clearResults();
}


void FindToolBar::invokeFindNext()
{
    setFindFlag(Find::FindBackward, false);
    invokeFindStep();
}

void FindToolBar::invokeFindPrevious()
{
    setFindFlag(Find::FindBackward, true);
    invokeFindStep();
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
    disconnect(m_ui.findEdit, SIGNAL(textChanged(QString)), this, SLOT(invokeFindIncremental()));
    if (hasFindFlag(Find::FindRegularExpression))
        m_ui.findEdit->setText(QRegExp::escape(text));
    else
        m_ui.findEdit->setText(text);
    connect(m_ui.findEdit, SIGNAL(textChanged(QString)), this, SLOT(invokeFindIncremental()));
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
        m_plugin->updateFindCompletion(getFindText());
        IFindSupport::Result result =
            m_currentDocumentFind->findStep(getFindText(), effectiveFindFlags());
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
        if (result == IFindSupport::NotYetFound)
            m_findIncrementalTimer.start(50);
        if (text.isEmpty())
            m_currentDocumentFind->clearResults();
    }
}

void FindToolBar::invokeReplace()
{
    setFindFlag(Find::FindBackward, false);
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        m_plugin->updateFindCompletion(getFindText());
        m_plugin->updateReplaceCompletion(getReplaceText());
        m_currentDocumentFind->replace(getFindText(), getReplaceText(), effectiveFindFlags());
    }
}

void FindToolBar::invokeReplaceNext()
{
    setFindFlag(Find::FindBackward, false);
    invokeReplaceStep();
}

void FindToolBar::invokeReplacePrevious()
{
    setFindFlag(Find::FindBackward, true);
    invokeReplaceStep();
}

void FindToolBar::invokeReplaceStep()
{
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        m_plugin->updateFindCompletion(getFindText());
        m_plugin->updateReplaceCompletion(getReplaceText());
        m_currentDocumentFind->replaceStep(getFindText(), getReplaceText(), effectiveFindFlags());
    }
}

void FindToolBar::invokeReplaceAll()
{
    m_plugin->updateFindCompletion(getFindText());
    m_plugin->updateReplaceCompletion(getReplaceText());
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace())
        m_currentDocumentFind->replaceAll(getFindText(), getReplaceText(), effectiveFindFlags());
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
    const QString text = m_currentDocumentFind->currentFindString();
    QApplication::clipboard()->setText(text, QClipboard::FindBuffer);
    setFindText(text);
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

void FindToolBar::updateIcons()
{
    Find::FindFlags effectiveFlags = effectiveFindFlags();
    bool casesensitive = effectiveFlags & Find::FindCaseSensitively;
    bool wholewords = effectiveFlags & Find::FindWholeWords;
    bool regexp = effectiveFlags & Find::FindRegularExpression;
    bool preserveCase = effectiveFlags & Find::FindPreserveCase;
    if (!casesensitive && !wholewords && !regexp && !preserveCase) {
        QPixmap pixmap(17, 17);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        const QPixmap mag = QPixmap(QLatin1String(Core::Constants::ICON_MAGNIFIER));
        painter.drawPixmap(0, (pixmap.height() - mag.height()) / 2, mag);
        m_ui.findEdit->setButtonPixmap(Utils::FancyLineEdit::Left, pixmap);
    } else {
        m_ui.findEdit->setButtonPixmap(Utils::FancyLineEdit::Left,
                                       IFindFilter::pixmapForFindFlags(effectiveFlags));
    }
}

Find::FindFlags FindToolBar::effectiveFindFlags()
{
    Find::FindFlags supportedFlags;
    bool supportsReplace = true;
    if (m_currentDocumentFind->isEnabled()) {
        supportedFlags = m_currentDocumentFind->supportedFindFlags();
        supportsReplace = m_currentDocumentFind->supportsReplace();
    } else {
        supportedFlags = (Find::FindFlags)0xFFFFFF;
    }
    if (!supportsReplace || m_findFlags & Find::FindRegularExpression)
        supportedFlags &= ~Find::FindPreserveCase;
    return supportedFlags & m_findFlags;
}

void FindToolBar::updateFlagMenus()
{
    bool wholeOnly = ((m_findFlags & Find::FindWholeWords));
    bool sensitive = ((m_findFlags & Find::FindCaseSensitively));
    bool regexp = ((m_findFlags & Find::FindRegularExpression));
    bool preserveCase = ((m_findFlags & Find::FindPreserveCase));
    if (m_wholeWordAction->isChecked() != wholeOnly)
        m_wholeWordAction->setChecked(wholeOnly);
    if (m_caseSensitiveAction->isChecked() != sensitive)
        m_caseSensitiveAction->setChecked(sensitive);
    if (m_regularExpressionAction->isChecked() != regexp)
        m_regularExpressionAction->setChecked(regexp);
    if (m_preserveCaseAction->isChecked() != preserveCase)
        m_preserveCaseAction->setChecked(preserveCase);
    Find::FindFlags supportedFlags;
    if (m_currentDocumentFind->isEnabled())
        supportedFlags = m_currentDocumentFind->supportedFindFlags();
    m_wholeWordAction->setEnabled(supportedFlags & Find::FindWholeWords);
    m_caseSensitiveAction->setEnabled(supportedFlags & Find::FindCaseSensitively);
    m_regularExpressionAction->setEnabled(supportedFlags & Find::FindRegularExpression);
    bool replaceEnabled = m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace();
    m_preserveCaseAction->setEnabled((supportedFlags & Find::FindPreserveCase) && !regexp && replaceEnabled);
}

bool FindToolBar::setFocusToCurrentFindSupport()
{
    return m_currentDocumentFind->setFocusToCurrentFindSupport();
}

void FindToolBar::hideAndResetFocus()
{
    m_currentDocumentFind->setFocusToCurrentFindSupport();
    hide();
}

Core::FindToolBarPlaceHolder *FindToolBar::findToolBarPlaceHolder() const
{
    QList<Core::FindToolBarPlaceHolder*> placeholders = ExtensionSystem::PluginManager::getObjects<Core::FindToolBarPlaceHolder>();
    QWidget *candidate = QApplication::focusWidget();
    while (candidate) {
        foreach (Core::FindToolBarPlaceHolder *ph, placeholders) {
            if (ph->owner() == candidate)
                return ph;
        }
        candidate = candidate->parentWidget();
    }
    return 0;
}

void FindToolBar::openFind(bool focus)
{
    setBackward(false);
    openFindToolBar(focus);
}

void FindToolBar::openFindToolBar(bool focus)
{
    installEventFilters();
    if (!m_currentDocumentFind->candidateIsEnabled())
        return;
    Core::FindToolBarPlaceHolder *holder = findToolBarPlaceHolder();
    if (!holder)
        return;
    Core::FindToolBarPlaceHolder *previousHolder = Core::FindToolBarPlaceHolder::getCurrent();
    if (previousHolder)
        previousHolder->setWidget(0);
    Core::FindToolBarPlaceHolder::setCurrent(holder);
    m_currentDocumentFind->acceptCandidate();
    holder->setWidget(this);
    holder->setVisible(true);
    setVisible(true);
    if (focus)
        setFocus();
    QString text = m_currentDocumentFind->currentFindString();
    if (!text.isEmpty())
        setFindText(text);
    m_currentDocumentFind->defineFindScope();
    m_currentDocumentFind->highlightAll(getFindText(), effectiveFindFlags());
    if (focus)
        selectFindText();
}

void FindToolBar::findNextSelected()
{
    openFind(false);
    invokeFindNext();
}

void FindToolBar::findPreviousSelected()
{
    openFind(false);
    invokeFindPrevious();
}

bool FindToolBar::focusNextPrevChild(bool next)
{
    // close tab order change
    if (next && m_ui.replaceAllButton->hasFocus())
        m_ui.findEdit->setFocus(Qt::TabFocusReason);
    else if (!next && m_ui.findEdit->hasFocus())
        m_ui.replaceAllButton->setFocus(Qt::TabFocusReason);
    else
        return Utils::StyledBar::focusNextPrevChild(next);
    return true;
}

void FindToolBar::writeSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    settings->beginGroup(QLatin1String("FindToolBar"));
    settings->setValue(QLatin1String("Backward"), QVariant((m_findFlags & Find::FindBackward) != 0));
    settings->setValue(QLatin1String("CaseSensitively"), QVariant((m_findFlags & Find::FindCaseSensitively) != 0));
    settings->setValue(QLatin1String("WholeWords"), QVariant((m_findFlags & Find::FindWholeWords) != 0));
    settings->setValue(QLatin1String("RegularExpression"), QVariant((m_findFlags & Find::FindRegularExpression) != 0));
    settings->setValue(QLatin1String("PreserveCase"), QVariant((m_findFlags & Find::FindPreserveCase) != 0));
    settings->endGroup();
    settings->endGroup();
}

void FindToolBar::readSettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    settings->beginGroup(QLatin1String("FindToolBar"));
    Find::FindFlags flags;
    if (settings->value(QLatin1String("Backward"), false).toBool())
        flags |= Find::FindBackward;
    if (settings->value(QLatin1String("CaseSensitively"), false).toBool())
        flags |= Find::FindCaseSensitively;
    if (settings->value(QLatin1String("WholeWords"), false).toBool())
        flags |= Find::FindWholeWords;
    if (settings->value(QLatin1String("RegularExpression"), false).toBool())
        flags |= Find::FindRegularExpression;
    if (settings->value(QLatin1String("PreserveCase"), false).toBool())
        flags |= Find::FindPreserveCase;
    settings->endGroup();
    settings->endGroup();
    m_findFlags = flags;
    findFlagsChanged();
}

void FindToolBar::setUseFakeVim(bool on)
{
    m_useFakeVim = on;
}

void FindToolBar::setFindFlag(Find::FindFlag flag, bool enabled)
{
    bool hasFlag = hasFindFlag(flag);
    if ((hasFlag && enabled) || (!hasFlag && !enabled))
        return;
    if (enabled)
        m_findFlags |= flag;
    else
        m_findFlags &= ~flag;
    if (flag != Find::FindBackward)
        findFlagsChanged();
}

bool FindToolBar::hasFindFlag(Find::FindFlag flag)
{
    return m_findFlags & flag;
}

void FindToolBar::setCaseSensitive(bool sensitive)
{
    setFindFlag(Find::FindCaseSensitively, sensitive);
}

void FindToolBar::setWholeWord(bool wholeOnly)
{
    setFindFlag(Find::FindWholeWords, wholeOnly);
}

void FindToolBar::setRegularExpressions(bool regexp)
{
    setFindFlag(Find::FindRegularExpression, regexp);
}

void FindToolBar::setPreserveCase(bool preserveCase)
{
    setFindFlag(Find::FindPreserveCase, preserveCase);
}

void FindToolBar::setBackward(bool backward)
{
    setFindFlag(Find::FindBackward, backward);
}
