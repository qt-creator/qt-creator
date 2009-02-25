/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "findtoolbar.h"
#include "findplugin.h"
#include "textfindconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/stylehelper.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

#include <QtGui/QClipboard>
#include <QtGui/QCompleter>
#include <QtGui/QKeyEvent>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QPainter>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>

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
      m_replaceNextAction(0),
      m_widget(new QWidget)
{
    //setup ui
    m_ui.setupUi(m_widget);
    addWidget(m_widget);
    setFocusProxy(m_ui.findEdit);
    setProperty("topBorder", true);
    m_ui.findEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_ui.replaceEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(m_ui.findEdit, SIGNAL(editingFinished()), this, SLOT(invokeResetIncrementalSearch()));

    QWidget *spacerItem = new QWidget;
    spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    addWidget(spacerItem);
    QToolButton *close = new QToolButton;
    close->setProperty("type", QLatin1String("dockbutton"));
    close->setIcon(QIcon(":/core/images/closebutton.png"));
    connect(close, SIGNAL(clicked()), this, SLOT(hideAndResetFocus()));
    addWidget(close);

    m_ui.findPreviousButton->setProperty("type", QLatin1String("dockbutton"));
    m_ui.findNextButton->setProperty("type", QLatin1String("dockbutton"));
    m_ui.replacePreviousButton->setProperty("type", QLatin1String("dockbutton"));
    m_ui.replaceNextButton->setProperty("type", QLatin1String("dockbutton"));
    m_ui.replaceAllButton->setProperty("type", QLatin1String("dockbutton"));

    m_findCompleter->setModel(m_plugin->findCompletionModel());
    m_replaceCompleter->setModel(m_plugin->replaceCompletionModel());
    m_ui.findEdit->setCompleter(m_findCompleter);
    m_findCompleter->popup()->installEventFilter(this);
    m_ui.replaceEdit->setCompleter(m_replaceCompleter);

    m_ui.findEdit->setSide(qApp->layoutDirection() == Qt::LeftToRight ? Core::Utils::FancyLineEdit::Right : Core::Utils::FancyLineEdit::Left);
    QMenu *lineEditMenu = new QMenu(m_ui.findEdit);
    m_ui.findEdit->setMenu(lineEditMenu);

    m_ui.findEdit->installEventFilter(this);
    m_ui.replaceEdit->installEventFilter(this);
    m_widget->installEventFilter(this);

    connect(m_ui.findEdit, SIGNAL(textChanged(const QString&)), this, SLOT(invokeFindIncremental()));
    connect(m_ui.findEdit, SIGNAL(returnPressed()), this, SLOT(invokeFindEnter()));
    connect(m_ui.replaceEdit, SIGNAL(returnPressed()), this, SLOT(invokeReplaceEnter()));

    QAction *shiftEnterAction = new QAction(m_ui.findEdit);
    shiftEnterAction->setShortcut(QKeySequence("Shift+Enter"));
    shiftEnterAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftEnterAction, SIGNAL(triggered()), this, SLOT(invokeFindPrevious()));
    m_ui.findEdit->addAction(shiftEnterAction);
    QAction *shiftReturnAction = new QAction(m_ui.findEdit);
    shiftReturnAction->setShortcut(QKeySequence("Shift+Return"));
    shiftReturnAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftReturnAction, SIGNAL(triggered()), this, SLOT(invokeFindPrevious()));
    m_ui.findEdit->addAction(shiftReturnAction);

    QAction *shiftEnterReplaceAction = new QAction(m_ui.replaceEdit);
    shiftEnterReplaceAction->setShortcut(QKeySequence("Shift+Enter"));
    shiftEnterReplaceAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftEnterReplaceAction, SIGNAL(triggered()), this, SLOT(invokeReplacePrevious()));
    m_ui.replaceEdit->addAction(shiftEnterReplaceAction);
    QAction *shiftReturnReplaceAction = new QAction(m_ui.replaceEdit);
    shiftReturnReplaceAction->setShortcut(QKeySequence("Shift+Return"));
    shiftReturnReplaceAction->setShortcutContext(Qt::WidgetShortcut);
    connect(shiftReturnReplaceAction, SIGNAL(triggered()), this, SLOT(invokeReplacePrevious()));
    m_ui.replaceEdit->addAction(shiftReturnReplaceAction);

    // need to make sure QStringList is registered as metatype
    QMetaTypeId<QStringList>::qt_metatype_id();

    //register actions
    QList<int> globalcontext;
    globalcontext << Core::Constants::C_GLOBAL_ID;

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *mfind = am->actionContainer(Constants::M_FIND);
    Core::Command *cmd;

    m_findInDocumentAction = new QAction(tr("Current Document"), this);
    cmd = am->registerAction(m_findInDocumentAction, Constants::FIND_IN_DOCUMENT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence::Find);
    mfind->addAction(cmd, Constants::G_FIND_FILTERS);
    connect(m_findInDocumentAction, SIGNAL(triggered()), this, SLOT(openFind()));

    if (QApplication::clipboard()->supportsFindBuffer()) {
        m_enterFindStringAction = new QAction(tr("Enter Find String"), this);
        cmd = am->registerAction(m_enterFindStringAction, tr("Find.EnterFindString"), globalcontext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+E")));
        mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
        connect(m_enterFindStringAction, SIGNAL(triggered()), this, SLOT(putSelectionToFindClipboard()));
        connect(QApplication::clipboard(), SIGNAL(findBufferChanged()), this, SLOT(updateFromFindClipboard()));
    }

    m_findNextAction = new QAction(tr("Find Next"), this);
    cmd = am->registerAction(m_findNextAction, Constants::FIND_NEXT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence::FindNext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findNextAction, SIGNAL(triggered()), this, SLOT(invokeFindNext()));
    m_ui.findNextButton->setDefaultAction(cmd->action());

    m_findPreviousAction = new QAction(tr("Find Previous"), this);
    cmd = am->registerAction(m_findPreviousAction, Constants::FIND_PREVIOUS, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence::FindPrevious);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_findPreviousAction, SIGNAL(triggered()), this, SLOT(invokeFindPrevious()));
    m_ui.findPreviousButton->setDefaultAction(cmd->action());

    m_replaceNextAction = new QAction(tr("Replace && Find Next"), this);
    cmd = am->registerAction(m_replaceNextAction, Constants::REPLACE_NEXT, globalcontext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+=")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceNextAction, SIGNAL(triggered()), this, SLOT(invokeReplaceNext()));
    m_ui.replaceNextButton->setDefaultAction(cmd->action());

    m_replacePreviousAction = new QAction(tr("Replace && Find Previous"), this);
    cmd = am->registerAction(m_replacePreviousAction, Constants::REPLACE_PREVIOUS, globalcontext);
    // shortcut removed, clashes with Ctrl++ on many keyboard layouts
    //cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+=")));
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replacePreviousAction, SIGNAL(triggered()), this, SLOT(invokeReplacePrevious()));
    m_ui.replacePreviousButton->setDefaultAction(cmd->action());

    m_replaceAllAction = new QAction(tr("Replace All"), this);
    cmd = am->registerAction(m_replaceAllAction, Constants::REPLACE_ALL, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_ACTIONS);
    connect(m_replaceAllAction, SIGNAL(triggered()), this, SLOT(invokeReplaceAll()));
    m_ui.replaceAllButton->setDefaultAction(cmd->action());

    m_caseSensitiveAction = new QAction(tr("Case Sensitive"), this);
    m_caseSensitiveAction->setIcon(QIcon(":/find/images/casesensitively.png"));
    m_caseSensitiveAction->setCheckable(true);
    m_caseSensitiveAction->setChecked(false);
    cmd = am->registerAction(m_caseSensitiveAction, Constants::CASE_SENSITIVE, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_caseSensitiveAction, SIGNAL(triggered(bool)), m_plugin, SLOT(setCaseSensitive(bool)));
    lineEditMenu->addAction(m_caseSensitiveAction);

    m_wholeWordAction = new QAction(tr("Whole Words Only"), this);
    m_wholeWordAction->setIcon(QIcon(":/find/images/wholewords.png"));
    m_wholeWordAction->setCheckable(true);
    m_wholeWordAction->setChecked(false);
    cmd = am->registerAction(m_wholeWordAction, Constants::WHOLE_WORDS, globalcontext);
    mfind->addAction(cmd, Constants::G_FIND_FLAGS);
    connect(m_wholeWordAction, SIGNAL(triggered(bool)), m_plugin, SLOT(setWholeWord(bool)));
    lineEditMenu->addAction(m_wholeWordAction);

    connect(m_currentDocumentFind, SIGNAL(changed()), this, SLOT(updateActions()));
    updateActions();
    updateIcons();
    connect(m_plugin, SIGNAL(findFlagsChanged()), this, SLOT(findFlagsChanged()));
}

FindToolBar::~FindToolBar()
{
}

bool FindToolBar::eventFilter(QObject *obj, QEvent *event)
{
    if ((obj == m_ui.findEdit || obj == m_findCompleter->popup())
               && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
#ifdef Q_OS_MAC
        if (ke->key() == Qt::Key_Space && (ke->modifiers() & Qt::MetaModifier)) {
#else
        if (ke->key() == Qt::Key_Space && (ke->modifiers() & Qt::ControlModifier)) {
#endif
            QString completedText = m_currentDocumentFind->completedFindString();
            if (!completedText.isEmpty()) {
                setFindText(completedText);
                ke->accept();
                return true;
            }
        }
    } else if (obj == m_widget && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()
                && !m_findCompleter->popup()->isVisible()
                && !m_replaceCompleter->popup()->isVisible()) {
            if (setFocusToCurrentFindSupport()) {
                event->accept();
                return true;
            }
#ifdef Q_OS_MAC
        } else if (ke->key() == Qt::Key_Space && (ke->modifiers() & Qt::MetaModifier)) {
#else
        } else if (ke->key() == Qt::Key_Space && (ke->modifiers() & Qt::ControlModifier)) {
#endif
            event->accept();
            return true;
        }
    } else if (obj == m_widget && event->type() == QEvent::Hide) {
        invokeClearResults();
        if (m_currentDocumentFind->isEnabled()) {
            m_currentDocumentFind->clearFindScope();
        }
    }
    return QToolBar::eventFilter(obj, event);
}

void FindToolBar::updateActions()
{
    bool enabled = m_currentDocumentFind->isEnabled();
    bool replaceEnabled = enabled && m_currentDocumentFind->supportsReplace();
    m_findInDocumentAction->setEnabled(enabled);
    m_findNextAction->setEnabled(enabled);
    m_findPreviousAction->setEnabled(enabled);
    m_replaceNextAction->setEnabled(replaceEnabled);
    m_replacePreviousAction->setEnabled(replaceEnabled);
    m_replaceAllAction->setEnabled(replaceEnabled);
    m_caseSensitiveAction->setEnabled(enabled);
    m_wholeWordAction->setEnabled(enabled);
    if (QApplication::clipboard()->supportsFindBuffer())
        m_enterFindStringAction->setEnabled(enabled);
    bool replaceFocus = m_ui.replaceEdit->hasFocus();
    m_ui.findEdit->setEnabled(enabled);
    m_ui.findLabel->setEnabled(enabled);
    m_ui.replaceEdit->setEnabled(replaceEnabled);
    m_ui.replaceLabel->setEnabled(replaceEnabled);
    if (!replaceEnabled && enabled && replaceFocus)
        m_ui.findEdit->setFocus();
}

void FindToolBar::invokeFindEnter()
{
    if (m_currentDocumentFind->isEnabled()) {
        invokeFindNext();
    }
}

void FindToolBar::invokeReplaceEnter()
{
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        invokeReplaceNext();
    }
}

void FindToolBar::invokeClearResults()
{
    if (m_currentDocumentFind->isEnabled()) {
        m_currentDocumentFind->clearResults();
    }
}


void FindToolBar::invokeFindNext()
{
    m_plugin->setBackward(false);
    invokeFindStep();
}

void FindToolBar::invokeFindPrevious()
{
    m_plugin->setBackward(true);
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
    disconnect(m_ui.findEdit, SIGNAL(textChanged(const QString&)), this, SLOT(invokeFindIncremental()));
    m_ui.findEdit->setText(text);
    connect(m_ui.findEdit, SIGNAL(textChanged(const QString&)), this, SLOT(invokeFindIncremental()));
}

void FindToolBar::selectFindText()
{
    m_ui.findEdit->selectAll();
}

void FindToolBar::invokeFindStep()
{
    if (m_currentDocumentFind->isEnabled()) {
        m_plugin->updateFindCompletion(getFindText());
        m_currentDocumentFind->findStep(getFindText(), m_plugin->findFlags());
    }
}

void FindToolBar::invokeFindIncremental()
{
    if (m_currentDocumentFind->isEnabled()) {
        QString text = getFindText();
        m_currentDocumentFind->findIncremental(text, m_plugin->findFlags());
        if (text.isEmpty())
            m_currentDocumentFind->clearResults();
    }
}

void FindToolBar::invokeReplaceNext()
{
    m_plugin->setBackward(false);
    invokeReplaceStep();
}

void FindToolBar::invokeReplacePrevious()
{
    m_plugin->setBackward(true);
    invokeReplaceStep();
}

void FindToolBar::invokeReplaceStep()
{
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        m_plugin->updateFindCompletion(getFindText());
        m_plugin->updateReplaceCompletion(getReplaceText());
        m_currentDocumentFind->replaceStep(getFindText(), getReplaceText(), m_plugin->findFlags());
    }
}

void FindToolBar::invokeReplaceAll()
{
    m_plugin->updateFindCompletion(getFindText());
    m_plugin->updateReplaceCompletion(getReplaceText());
    if (m_currentDocumentFind->isEnabled() && m_currentDocumentFind->supportsReplace()) {
        m_currentDocumentFind->replaceAll(getFindText(), getReplaceText(), m_plugin->findFlags());
    }
}

void FindToolBar::invokeResetIncrementalSearch()
{
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
}

void FindToolBar::updateIcons()
{
    bool casesensitive = m_plugin->findFlags() & QTextDocument::FindCaseSensitively;
    bool wholewords = m_plugin->findFlags() & QTextDocument::FindWholeWords;

    if (casesensitive && wholewords) {
        QPixmap image = QPixmap(":/find/images/wordandcase.png");
        m_ui.findEdit->setPixmap(image);
    } else if (casesensitive) {
        QPixmap image = QPixmap(":/find/images/casesensitively.png");
        m_ui.findEdit->setPixmap(image);
    } else if (wholewords) {
        QPixmap image = QPixmap(":/find/images/wholewords.png");
        m_ui.findEdit->setPixmap(image);
    } else {
        m_ui.findEdit->setPixmap(QPixmap(Core::Constants::ICON_MAGNIFIER));
    }
}

void FindToolBar::updateFlagMenus()
{
    bool wholeOnly = ((m_plugin->findFlags() & QTextDocument::FindWholeWords));
    bool sensitive = ((m_plugin->findFlags() & QTextDocument::FindCaseSensitively));
    if (m_wholeWordAction->isChecked() != wholeOnly)
        m_wholeWordAction->setChecked(wholeOnly);
    if (m_caseSensitiveAction->isChecked() != sensitive)
        m_caseSensitiveAction->setChecked(sensitive);
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

void FindToolBar::openFind()
{
    if (!m_currentDocumentFind->isEnabled())
        return;
    Core::FindToolBarPlaceHolder *holder = Core::FindToolBarPlaceHolder::getCurrent();
    QLayout *findContainerLayout = holder ? holder->layout() : 0;

    if (findContainerLayout) {
        findContainerLayout->addWidget(this);
        holder->setVisible(true);
        setVisible(true);
        setFocus();
    }
    QString text = m_currentDocumentFind->currentFindString();
    if (!text.isEmpty())
        setFindText(text);
    m_currentDocumentFind->defineFindScope();
    m_currentDocumentFind->highlightAll(getFindText(), m_plugin->findFlags());
    selectFindText();
}


bool FindToolBar::focusNextPrevChild(bool next)
{
    // close tab order change
    if (next && m_ui.replaceAllButton->hasFocus())
        m_ui.findEdit->setFocus(Qt::TabFocusReason);
    else if (!next && m_ui.findEdit->hasFocus())
        m_ui.replaceAllButton->setFocus(Qt::TabFocusReason);
    else
        return QToolBar::focusNextPrevChild(next);
    return true;
}
