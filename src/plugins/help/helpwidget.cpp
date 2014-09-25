/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "helpwidget.h"

#include "bookmarkmanager.h"
#include "helpconstants.h"
#include "helpplugin.h"
#include "helpviewer.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/helpmanager.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QHBoxLayout>
#include <QHelpEngine>
#include <QHelpSearchEngine>
#include <QHelpSearchQuery>
#include <QMenu>
#include <QPrinter>
#include <QPrintDialog>
#include <QStackedWidget>
#include <QToolButton>

static QToolButton *toolButton(QAction *action, Core::Command *cmd = 0)
{
    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);
    button->setPopupMode(QToolButton::DelayedPopup);
    if (cmd) {
        action->setToolTip(cmd->stringWithAppendedShortcut(action->text()));
        QObject::connect(cmd, &Core::Command::keySequenceChanged, action, [cmd, action]() {
            action->setToolTip(cmd->stringWithAppendedShortcut(action->text()));
        });
    }
    return button;
}

namespace Help {
namespace Internal {

HelpWidget::HelpWidget(const Core::Context &context, WidgetStyle style, QWidget *parent) :
    QWidget(parent),
    m_style(style),
    m_switchToHelp(0),
    m_filterComboBox(0),
    m_closeAction(0),
    m_scaleUp(0),
    m_scaleDown(0),
    m_resetScale(0),
    m_printer(0)
{
    Utils::StyledBar *toolBar = new Utils::StyledBar();
    QHBoxLayout *layout = new QHBoxLayout(toolBar);
    layout->setSpacing(0);
    layout->setMargin(0);
    Core::Command *cmd;

    if (style != ModeWidget) {
        m_switchToHelp = new QAction(tr("Go to Help Mode"), toolBar);
        cmd = Core::ActionManager::registerAction(m_switchToHelp, Constants::CONTEXT_HELP, context);
        connect(m_switchToHelp, SIGNAL(triggered()), this, SLOT(helpModeButtonClicked()));
        layout->addWidget(toolButton(m_switchToHelp, cmd));
    }

    m_homeAction = new QAction(QIcon(QLatin1String(":/help/images/home.png")),
        tr("Home"), this);
    cmd = Core::ActionManager::registerAction(m_homeAction, Constants::HELP_HOME, context);
    connect(m_homeAction, &QAction::triggered, this, &HelpWidget::goHome);
    layout->addWidget(toolButton(m_homeAction, cmd));

    m_backAction = new QAction(QIcon(QLatin1String(":/help/images/previous.png")),
        tr("Back"), toolBar);
    connect(m_backAction, &QAction::triggered, this, &HelpWidget::backward);
    m_backMenu = new QMenu(toolBar);
    connect(m_backMenu, SIGNAL(aboutToShow()), this, SLOT(updateBackMenu()));
    m_backAction->setMenu(m_backMenu);
    cmd = Core::ActionManager::registerAction(m_backAction, Constants::HELP_PREVIOUS, context);
    cmd->setDefaultKeySequence(QKeySequence::Back);
    layout->addWidget(toolButton(m_backAction, cmd));

    m_forwardAction = new QAction(QIcon(QLatin1String(":/help/images/next.png")),
        tr("Forward"), toolBar);
    connect(m_forwardAction, &QAction::triggered, this, &HelpWidget::forward);
    m_forwardMenu = new QMenu(toolBar);
    connect(m_forwardMenu, SIGNAL(aboutToShow()), this, SLOT(updateForwardMenu()));
    m_forwardAction->setMenu(m_forwardMenu);
    cmd = Core::ActionManager::registerAction(m_forwardAction, Constants::HELP_NEXT, context);
    cmd->setDefaultKeySequence(QKeySequence::Forward);
    layout->addWidget(toolButton(m_forwardAction, cmd));

    m_addBookmarkAction = new QAction(QIcon(QLatin1String(":/help/images/bookmark.png")),
        tr("Add Bookmark"), this);
    cmd = Core::ActionManager::registerAction(m_addBookmarkAction, Constants::HELP_BOOKMARK, context);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+M") : tr("Ctrl+M")));
    connect(m_addBookmarkAction, &QAction::triggered, this, &HelpWidget::addBookmark);
    layout->addWidget(new Utils::StyledSeparator(toolBar));
    layout->addWidget(toolButton(m_addBookmarkAction, cmd));

    if (style == ModeWidget) {
        layout->addWidget(new Utils::StyledSeparator(toolBar));
        layout->addWidget(OpenPagesManager::instance().openPagesComboBox(), 10);
        m_filterComboBox = new QComboBox;
        m_filterComboBox->setMinimumContentsLength(15);
        m_filterComboBox->setModel(LocalHelpManager::filterModel());
        layout->addWidget(m_filterComboBox);
        connect(m_filterComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                LocalHelpManager::instance(), &LocalHelpManager::setFilterIndex);
        connect(LocalHelpManager::instance(), &LocalHelpManager::filterIndexChanged,
                m_filterComboBox, &QComboBox::setCurrentIndex);
    }

    layout->addStretch();

    m_viewerStack = new QStackedWidget;

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);
    vLayout->addWidget(toolBar);
    vLayout->addWidget(m_viewerStack);
    Core::FindToolBarPlaceHolder *fth = new Core::FindToolBarPlaceHolder(this);
    vLayout->addWidget(fth);

    setFocusProxy(m_viewerStack);

    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(m_viewerStack);
    Core::ICore::addContextObject(m_context);

    m_printAction = new QAction(this);
    Core::ActionManager::registerAction(m_printAction, Core::Constants::PRINT, context);
    connect(m_printAction, &QAction::triggered, this, [this]() { print(currentViewer()); });

    m_copy = new QAction(this);
    Core::ActionManager::registerAction(m_copy, Core::Constants::COPY, context);
    connect(m_copy, &QAction::triggered, this, &HelpWidget::copy);

    Core::ActionContainer *advancedMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT_ADVANCED);
    QTC_CHECK(advancedMenu);
    if (advancedMenu) {
        // reuse TextEditor constants to avoid a second pair of menu actions
        m_scaleUp = new QAction(tr("Increase Font Size"), this);
        cmd = Core::ActionManager::registerAction(m_scaleUp, TextEditor::Constants::INCREASE_FONT_SIZE,
                                                  context);
        connect(m_scaleUp, &QAction::triggered, this, &HelpWidget::scaleUp);
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        m_scaleDown = new QAction(tr("Decrease Font Size"), this);
        cmd = Core::ActionManager::registerAction(m_scaleDown, TextEditor::Constants::DECREASE_FONT_SIZE,
                                                  context);
        connect(m_scaleDown, &QAction::triggered, this, &HelpWidget::scaleDown);
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);

        m_resetScale = new QAction(tr("Reset Font Size"), this);
        cmd = Core::ActionManager::registerAction(m_resetScale, TextEditor::Constants::RESET_FONT_SIZE,
                                                  context);
        connect(m_resetScale, &QAction::triggered, this, &HelpWidget::resetScale);
        advancedMenu->addAction(cmd, Core::Constants::G_EDIT_FONT);
    }

    if (style == ModeWidget) {
        m_closeAction = new QAction(QIcon(QLatin1String(Core::Constants::ICON_BUTTON_CLOSE)),
            QString(), toolBar);
        connect(m_closeAction, SIGNAL(triggered()), this, SIGNAL(closeButtonClicked()));
        layout->addWidget(toolButton(m_closeAction));
    } else if (style == SideBarWidget) {
        m_closeAction = new QAction(QIcon(QLatin1String(Core::Constants::ICON_BUTTON_CLOSE)),
            QString(), toolBar);
        connect(m_closeAction, SIGNAL(triggered()), this, SIGNAL(closeButtonClicked()));
        layout->addWidget(toolButton(m_closeAction));
    } else if (style == ExternalWindow) {
        static int windowId = 0;
        Core::ICore::registerWindow(this,
                                    Core::Context(Core::Id("Help.Window.").withSuffix(++windowId)));
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_QuitOnClose, false); // don't prevent Qt Creator from closing
    }

    if (style != ModeWidget) {
        HelpViewer *viewer = HelpPlugin::createHelpViewer(qreal(0.0));
        addViewer(viewer);
        setCurrentViewer(viewer);
    }
}

HelpWidget::~HelpWidget()
{
    Core::ICore::removeContextObject(m_context);
    Core::ActionManager::unregisterAction(m_copy, Core::Constants::COPY);
    if (m_switchToHelp)
        Core::ActionManager::unregisterAction(m_switchToHelp, Constants::CONTEXT_HELP);
    Core::ActionManager::unregisterAction(m_homeAction, Constants::HELP_HOME);
    Core::ActionManager::unregisterAction(m_forwardAction, Constants::HELP_NEXT);
    Core::ActionManager::unregisterAction(m_backAction, Constants::HELP_PREVIOUS);
    Core::ActionManager::unregisterAction(m_addBookmarkAction, Constants::HELP_BOOKMARK);
    if (m_scaleUp)
        Core::ActionManager::unregisterAction(m_scaleUp, TextEditor::Constants::INCREASE_FONT_SIZE);
    if (m_scaleDown)
        Core::ActionManager::unregisterAction(m_scaleDown, TextEditor::Constants::DECREASE_FONT_SIZE);
    if (m_resetScale)
        Core::ActionManager::unregisterAction(m_resetScale, TextEditor::Constants::RESET_FONT_SIZE);
}

HelpViewer *HelpWidget::currentViewer() const
{
    return qobject_cast<HelpViewer *>(m_viewerStack->currentWidget());
}

void HelpWidget::setCurrentViewer(HelpViewer *viewer)
{
    m_viewerStack->setCurrentWidget(viewer);
    m_backAction->setEnabled(viewer->isBackwardAvailable());
    m_forwardAction->setEnabled(viewer->isForwardAvailable());
    if (m_style == ExternalWindow)
        updateWindowTitle();
    emit sourceChanged(viewer->source());
}

int HelpWidget::currentIndex() const
{
    return m_viewerStack->currentIndex();
}

void HelpWidget::addViewer(HelpViewer *viewer, bool highlightSearchTerms)
{
    m_viewerStack->addWidget(viewer);
    viewer->setFocus(Qt::OtherFocusReason);
    if (m_style == SideBarWidget || m_style == ExternalWindow)
        viewer->setOpenInNewPageActionVisible(false);
    connect(viewer, &HelpViewer::sourceChanged, this, [viewer, this](const QUrl &url) {
        if (currentViewer() == viewer)
            emit sourceChanged(url);
    });
    connect(viewer, &HelpViewer::forwardAvailable, this, [viewer, this](bool available) {
        if (currentViewer() == viewer)
            m_forwardAction->setEnabled(available);
    });
    connect(viewer, &HelpViewer::backwardAvailable, this, [viewer, this](bool available) {
        if (currentViewer() == viewer)
            m_backAction->setEnabled(available);
    });
    connect(viewer, &HelpViewer::printRequested, this, [viewer, this]() {
        print(viewer);
    });
    if (m_style == ExternalWindow)
        connect(viewer, SIGNAL(titleChanged()), this, SLOT(updateWindowTitle()));

    if (highlightSearchTerms)
        connect(viewer, &HelpViewer::loadFinished, this, &HelpWidget::highlightSearchTerms);

    updateCloseButton();
}

void HelpWidget::removeViewerAt(int index)
{
    QWidget *viewerWidget = m_viewerStack->widget(index);
    QTC_ASSERT(viewerWidget, return);
    m_viewerStack->removeWidget(viewerWidget);
    // do not delete, that is done in the model
    // delete viewerWidget;
    if (m_viewerStack->currentWidget())
        setCurrentViewer(qobject_cast<HelpViewer *>(m_viewerStack->currentWidget()));
    updateCloseButton();
}

void HelpWidget::setViewerFont(const QFont &font)
{
    for (int i = 0; i < m_viewerStack->count(); ++i) {
        HelpViewer *viewer = qobject_cast<HelpViewer *>(m_viewerStack->widget(i));
        QTC_ASSERT(viewer, continue);
        viewer->setFont(font);
    }
}

int HelpWidget::viewerCount() const
{
    return m_viewerStack->count();
}

HelpViewer *HelpWidget::viewerAt(int index) const
{
    return qobject_cast<HelpViewer *>(m_viewerStack->widget(index));
}

void HelpWidget::setSource(const QUrl &url)
{
    HelpViewer* viewer = currentViewer();
    QTC_ASSERT(viewer, return);
    viewer->setSource(url);
    viewer->setFocus(Qt::OtherFocusReason);
}

void HelpWidget::setSourceFromSearch(const QUrl &url)
{
    HelpViewer* viewer = currentViewer();
    QTC_ASSERT(viewer, return);
    connect(viewer, &HelpViewer::loadFinished, this, &HelpWidget::highlightSearchTerms);
    viewer->setSource(url);
    viewer->setFocus(Qt::OtherFocusReason);
}

void HelpWidget::closeEvent(QCloseEvent *)
{
    emit aboutToClose();
}

void HelpWidget::updateBackMenu()
{
    m_backMenu->clear();
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->addBackHistoryItems(m_backMenu);
}

void HelpWidget::updateForwardMenu()
{
    m_forwardMenu->clear();
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->addForwardHistoryItems(m_forwardMenu);
}

void HelpWidget::updateWindowTitle()
{
    QTC_ASSERT(currentViewer(), return);
    const QString pageTitle = currentViewer()->title();
    if (pageTitle.isEmpty())
        setWindowTitle(tr("Help"));
    else
        setWindowTitle(tr("Help - %1").arg(pageTitle));
}

void HelpWidget::helpModeButtonClicked()
{
    QTC_ASSERT(currentViewer(), return);
    emit openHelpMode(currentViewer()->source());
    if (m_style == ExternalWindow)
        close();
}

void HelpWidget::updateCloseButton()
{
    if (m_style == ModeWidget) {
        const bool closeOnReturn = Core::HelpManager::customValue(QLatin1String("ReturnOnClose"),
            false).toBool();
        m_closeAction->setEnabled(closeOnReturn || m_viewerStack->count() > 1);
    }
}

void HelpWidget::goHome()
{
    if (HelpViewer *viewer = currentViewer())
        viewer->home();
}

void HelpWidget::addBookmark()
{
    HelpViewer *viewer = currentViewer();
    QTC_ASSERT(viewer, return);

    const QString &url = viewer->source().toString();
    if (url.isEmpty() || url == Help::Constants::AboutBlank)
        return;

    BookmarkManager *manager = &LocalHelpManager::bookmarkManager();
    manager->showBookmarkDialog(this, viewer->title(), url);
}

void HelpWidget::copy()
{
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->copy();
}

void HelpWidget::forward()
{
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->forward();
}

void HelpWidget::backward()
{
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->backward();
}

void HelpWidget::scaleUp()
{
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->scaleUp();
}

void HelpWidget::scaleDown()
{
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->scaleDown();
}

void HelpWidget::resetScale()
{
    QTC_ASSERT(currentViewer(), return);
    currentViewer()->resetScale();
}

void HelpWidget::print(HelpViewer *viewer)
{
    QTC_ASSERT(viewer, return);
    if (!m_printer)
        m_printer = new QPrinter(QPrinter::HighResolution);
    QPrintDialog dlg(m_printer, this);
    dlg.setWindowTitle(tr("Print Documentation"));
    if (!viewer->selectedText().isEmpty())
        dlg.addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg.addEnabledOption(QAbstractPrintDialog::PrintPageRange);
    dlg.addEnabledOption(QAbstractPrintDialog::PrintCollateCopies);

    if (dlg.exec() == QDialog::Accepted)
        viewer->print(m_printer);
}

void HelpWidget::highlightSearchTerms()
{
    if (HelpViewer *viewer = qobject_cast<HelpViewer *>(sender())) {
        QHelpSearchEngine *searchEngine =
            LocalHelpManager::helpEngine().searchEngine();
        QList<QHelpSearchQuery> queryList = searchEngine->query();

        QStringList terms;
        foreach (const QHelpSearchQuery &query, queryList) {
            switch (query.fieldName) {
                default: break;
                case QHelpSearchQuery::ALL: {
                case QHelpSearchQuery::PHRASE:
                case QHelpSearchQuery::DEFAULT:
                case QHelpSearchQuery::ATLEAST:
                    foreach (QString term, query.wordList)
                        terms.append(term.remove(QLatin1Char('"')));
                }
            }
        }

        foreach (const QString& term, terms)
            viewer->findText(term, 0, false, true);
        disconnect(viewer, &HelpViewer::loadFinished, this, &HelpWidget::highlightSearchTerms);
    }
}

} // Internal
} // Help
