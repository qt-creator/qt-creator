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

#include "helpwidget.h"

#include "bookmarkmanager.h"
#include "contentwindow.h"
#include "helpconstants.h"
#include "helpicons.h"
#include "helpplugin.h"
#include "helpviewer.h"
#include "indexwindow.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"
#include "searchwidget.h"
#include "topicchooser.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QHelpEngine>
#include <QHelpSearchEngine>
#include <QHelpSearchQuery>
#include <QMenu>
#include <QPrinter>
#include <QPrintDialog>
#include <QStackedWidget>
#include <QToolButton>

static const char kWindowSideBarSettingsKey[] = "Help/WindowSideBar";
static const char kModeSideBarSettingsKey[] = "Help/ModeSideBar";

namespace Help {
namespace Internal {

HelpWidget::HelpWidget(const Core::Context &context, WidgetStyle style, QWidget *parent) :
    QWidget(parent),
    m_style(style),
    m_toggleSideBarAction(0),
    m_switchToHelp(0),
    m_filterComboBox(0),
    m_closeAction(0),
    m_scaleUp(0),
    m_scaleDown(0),
    m_resetScale(0),
    m_printer(0),
    m_sideBar(0),
    m_contentsAction(0),
    m_indexAction(0),
    m_bookmarkAction(0),
    m_searchAction(0),
    m_openPagesAction(0)
{
    m_viewerStack = new QStackedWidget;

    auto hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);

    m_sideBarSplitter = new Core::MiniSplitter(this);
    m_sideBarSplitter->setOpaqueResize(false);
    hLayout->addWidget(m_sideBarSplitter);

    Utils::StyledBar *toolBar = new Utils::StyledBar();
    QHBoxLayout *layout = new QHBoxLayout(toolBar);
    layout->setSpacing(0);
    layout->setMargin(0);

    auto rightSide = new QWidget(this);
    m_sideBarSplitter->insertWidget(1, rightSide);
    QVBoxLayout *vLayout = new QVBoxLayout(rightSide);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);
    vLayout->addWidget(toolBar);
    vLayout->addWidget(m_viewerStack);
    Core::FindToolBarPlaceHolder *fth = new Core::FindToolBarPlaceHolder(this);
    vLayout->addWidget(fth);

    setFocusProxy(m_viewerStack);

    m_context = new Core::IContext(this);
    m_context->setContext(context);
    m_context->setWidget(m_sideBarSplitter);
    Core::ICore::addContextObject(m_context);

    Core::Command *cmd;
    QToolButton *button;

    if (style == ExternalWindow) {
        static int windowId = 0;
        Core::ICore::registerWindow(this,
                                    Core::Context(Core::Id("Help.Window.").withSuffix(++windowId)));
        setAttribute(Qt::WA_DeleteOnClose);
        setAttribute(Qt::WA_QuitOnClose, false); // don't prevent Qt Creator from closing
    }
    if (style != SideBarWidget) {
        m_toggleSideBarAction = new QAction(Core::Icons::TOGGLE_SIDEBAR.icon(),
                                            QCoreApplication::translate("Core", Core::Constants::TR_SHOW_SIDEBAR),
                                            toolBar);
        m_toggleSideBarAction->setCheckable(true);
        m_toggleSideBarAction->setChecked(false);
        cmd = Core::ActionManager::registerAction(m_toggleSideBarAction,
                                                  Core::Constants::TOGGLE_SIDEBAR, context);
        connect(m_toggleSideBarAction, &QAction::toggled, m_toggleSideBarAction,
                [this](bool checked) {
                    m_toggleSideBarAction->setText(
                        QCoreApplication::translate("Core",
                                                    checked ? Core::Constants::TR_HIDE_SIDEBAR
                                                            : Core::Constants::TR_SHOW_SIDEBAR));
                });
        addSideBar();
        m_toggleSideBarAction->setChecked(m_sideBar->isVisibleTo(this));
        connect(m_toggleSideBarAction, &QAction::triggered, m_sideBar, &Core::SideBar::setVisible);
        connect(m_sideBar, &Core::SideBar::sideBarClosed, m_toggleSideBarAction, [this]() {
            m_toggleSideBarAction->setChecked(false);
        });
    }
    if (style == ExternalWindow)
        layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_toggleSideBarAction, cmd));

    if (style != ModeWidget) {
        m_switchToHelp = new QAction(tr("Go to Help Mode"), toolBar);
        cmd = Core::ActionManager::registerAction(m_switchToHelp, Constants::CONTEXT_HELP, context);
        connect(m_switchToHelp, &QAction::triggered, this, &HelpWidget::helpModeButtonClicked);
        layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_switchToHelp, cmd));
    }

    m_homeAction = new QAction(Icons::HOME.icon(), tr("Home"), this);
    cmd = Core::ActionManager::registerAction(m_homeAction, Constants::HELP_HOME, context);
    connect(m_homeAction, &QAction::triggered, this, &HelpWidget::goHome);
    layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_homeAction, cmd));

    m_backAction = new QAction(Core::Icons::PREV.icon(), tr("Back"), toolBar);
    connect(m_backAction, &QAction::triggered, this, &HelpWidget::backward);
    m_backMenu = new QMenu(toolBar);
    connect(m_backMenu, &QMenu::aboutToShow, this, &HelpWidget::updateBackMenu);
    m_backAction->setMenu(m_backMenu);
    cmd = Core::ActionManager::registerAction(m_backAction, Constants::HELP_PREVIOUS, context);
    cmd->setDefaultKeySequence(QKeySequence::Back);
    button = Core::Command::toolButtonWithAppendedShortcut(m_backAction, cmd);
    button->setPopupMode(QToolButton::DelayedPopup);
    layout->addWidget(button);

    m_forwardAction = new QAction(Core::Icons::NEXT.icon(), tr("Forward"), toolBar);
    connect(m_forwardAction, &QAction::triggered, this, &HelpWidget::forward);
    m_forwardMenu = new QMenu(toolBar);
    connect(m_forwardMenu, &QMenu::aboutToShow, this, &HelpWidget::updateForwardMenu);
    m_forwardAction->setMenu(m_forwardMenu);
    cmd = Core::ActionManager::registerAction(m_forwardAction, Constants::HELP_NEXT, context);
    cmd->setDefaultKeySequence(QKeySequence::Forward);
    button = Core::Command::toolButtonWithAppendedShortcut(m_forwardAction, cmd);
    button->setPopupMode(QToolButton::DelayedPopup);
    layout->addWidget(button);

    m_addBookmarkAction = new QAction(Icons::BOOKMARK.icon(), tr("Add Bookmark"), this);
    cmd = Core::ActionManager::registerAction(m_addBookmarkAction, Constants::HELP_ADDBOOKMARK, context);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+M") : tr("Ctrl+M")));
    connect(m_addBookmarkAction, &QAction::triggered, this, &HelpWidget::addBookmark);
    layout->addWidget(new Utils::StyledSeparator(toolBar));
    layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_addBookmarkAction, cmd));

    if (style == ModeWidget) {
        layout->addWidget(new Utils::StyledSeparator(toolBar));
        layout->addWidget(OpenPagesManager::instance().openPagesComboBox(), 10);
    } else {
        layout->addWidget(new QLabel(), 10);
    }
    if (style != SideBarWidget) {
        m_filterComboBox = new QComboBox;
        m_filterComboBox->setMinimumContentsLength(15);
        m_filterComboBox->setModel(LocalHelpManager::filterModel());
        m_filterComboBox->setCurrentIndex(LocalHelpManager::filterIndex());
        layout->addWidget(m_filterComboBox);
        connect(m_filterComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
                LocalHelpManager::instance(), &LocalHelpManager::setFilterIndex);
        connect(LocalHelpManager::instance(), &LocalHelpManager::filterIndexChanged,
                m_filterComboBox, &QComboBox::setCurrentIndex);
    }

    layout->addStretch();

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

    if (style != ExternalWindow) {
        m_closeAction = new QAction(Core::Icons::BUTTON_CLOSE.icon(), QString(), toolBar);
        connect(m_closeAction, SIGNAL(triggered()), this, SIGNAL(closeButtonClicked()));
        button = new QToolButton;
        button->setDefaultAction(m_closeAction);
        layout->addWidget(button);
    }

    if (style != ModeWidget) {
        HelpViewer *viewer = HelpPlugin::createHelpViewer(qreal(0.0));
        addViewer(viewer);
        setCurrentViewer(viewer);
    }
}

HelpWidget::~HelpWidget()
{
    if (m_sideBar) {
        m_sideBar->saveSettings(Core::ICore::settings(), sideBarSettingsKey());
        Core::ActionManager::unregisterAction(m_contentsAction, Constants::HELP_CONTENTS);
        Core::ActionManager::unregisterAction(m_indexAction, Constants::HELP_INDEX);
        Core::ActionManager::unregisterAction(m_bookmarkAction, Constants::HELP_BOOKMARKS);
        Core::ActionManager::unregisterAction(m_searchAction, Constants::HELP_SEARCH);
        if (m_openPagesAction)
            Core::ActionManager::unregisterAction(m_openPagesAction, Constants::HELP_OPENPAGES);
    }
    Core::ICore::removeContextObject(m_context);
    Core::ActionManager::unregisterAction(m_copy, Core::Constants::COPY);
    Core::ActionManager::unregisterAction(m_printAction, Core::Constants::PRINT);
    if (m_toggleSideBarAction)
        Core::ActionManager::unregisterAction(m_toggleSideBarAction, Core::Constants::TOGGLE_SIDEBAR);
    if (m_switchToHelp)
        Core::ActionManager::unregisterAction(m_switchToHelp, Constants::CONTEXT_HELP);
    Core::ActionManager::unregisterAction(m_homeAction, Constants::HELP_HOME);
    Core::ActionManager::unregisterAction(m_forwardAction, Constants::HELP_NEXT);
    Core::ActionManager::unregisterAction(m_backAction, Constants::HELP_PREVIOUS);
    Core::ActionManager::unregisterAction(m_addBookmarkAction, Constants::HELP_ADDBOOKMARK);
    if (m_scaleUp)
        Core::ActionManager::unregisterAction(m_scaleUp, TextEditor::Constants::INCREASE_FONT_SIZE);
    if (m_scaleDown)
        Core::ActionManager::unregisterAction(m_scaleDown, TextEditor::Constants::DECREASE_FONT_SIZE);
    if (m_resetScale)
        Core::ActionManager::unregisterAction(m_resetScale, TextEditor::Constants::RESET_FONT_SIZE);
}

void HelpWidget::addSideBar()
{
    QMap<QString, Core::Command *> shortcutMap;
    Core::Command *cmd;
    bool supportsNewPages = (m_style == ModeWidget);

    auto contentWindow = new ContentWindow;
    auto contentItem = new Core::SideBarItem(contentWindow, QLatin1String(Constants::HELP_CONTENTS));
    contentWindow->setOpenInNewPageActionVisible(supportsNewPages);
    contentWindow->setWindowTitle(HelpPlugin::tr(Constants::SB_CONTENTS));
    connect(contentWindow, &ContentWindow::linkActivated,
            this, &HelpWidget::open);
    m_contentsAction = new QAction(tr(Constants::SB_CONTENTS), this);
    cmd = Core::ActionManager::registerAction(m_contentsAction, Constants::HELP_CONTENTS, m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Shift+C")
                                                                  : tr("Ctrl+Shift+C")));
    shortcutMap.insert(QLatin1String(Constants::HELP_CONTENTS), cmd);

    auto indexWindow = new IndexWindow();
    auto indexItem = new Core::SideBarItem(indexWindow, QLatin1String(Constants::HELP_INDEX));
    indexWindow->setOpenInNewPageActionVisible(supportsNewPages);
    indexWindow->setWindowTitle(HelpPlugin::tr(Constants::SB_INDEX));
    connect(indexWindow, &IndexWindow::linkActivated,
            this, &HelpWidget::open);
    connect(indexWindow, &IndexWindow::linksActivated,
        this, &HelpWidget::showTopicChooser);
    m_indexAction = new QAction(tr(Constants::SB_INDEX), this);
    cmd = Core::ActionManager::registerAction(m_indexAction, Constants::HELP_INDEX, m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+I")
                                                                  : tr("Ctrl+Shift+I")));
    shortcutMap.insert(QLatin1String(Constants::HELP_INDEX), cmd);

    auto bookmarkWidget = new BookmarkWidget(&LocalHelpManager::bookmarkManager());
    bookmarkWidget->setWindowTitle(HelpPlugin::tr(Constants::SB_BOOKMARKS));
    bookmarkWidget->setOpenInNewPageActionVisible(supportsNewPages);
    auto bookmarkItem = new Core::SideBarItem(bookmarkWidget,
                                              QLatin1String(Constants::HELP_BOOKMARKS));
    connect(bookmarkWidget, &BookmarkWidget::linkActivated, this, &HelpWidget::setSource);
    m_bookmarkAction = new QAction(tr("Activate Help Bookmarks View"), this);
    cmd = Core::ActionManager::registerAction(m_bookmarkAction, Constants::HELP_BOOKMARKS,
                                              m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Alt+Meta+M")
                                                                  : tr("Ctrl+Shift+B")));
    shortcutMap.insert(QLatin1String(Constants::HELP_BOOKMARKS), cmd);

    auto searchItem = new SearchSideBarItem;
    connect(searchItem, &SearchSideBarItem::linkActivated, this, &HelpWidget::openFromSearch);
    m_searchAction = new QAction(tr("Activate Help Search View"), this);
    cmd = Core::ActionManager::registerAction(m_searchAction, Constants::HELP_SEARCH,
                                              m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+/")
                                                                  : tr("Ctrl+Shift+/")));
    shortcutMap.insert(QLatin1String(Constants::HELP_SEARCH), cmd);

    Core::SideBarItem *openPagesItem = 0;
    if (m_style == ModeWidget) {
        QWidget *openPagesWidget = OpenPagesManager::instance().openPagesWidget();
        openPagesWidget->setWindowTitle(HelpPlugin::tr(Constants::SB_OPENPAGES));
        openPagesItem = new Core::SideBarItem(openPagesWidget,
                                              QLatin1String(Constants::HELP_OPENPAGES));
        m_openPagesAction = new QAction(tr("Activate Open Help Pages View"), this);
        cmd = Core::ActionManager::registerAction(m_openPagesAction, Constants::HELP_OPENPAGES,
                                                  m_context->context());
        cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+O")
                                                                      : tr("Ctrl+Shift+O")));
        shortcutMap.insert(QLatin1String(Constants::HELP_OPENPAGES), cmd);
    }

    QList<Core::SideBarItem *> itemList;
    itemList << contentItem << indexItem << bookmarkItem << searchItem;
    if (openPagesItem)
         itemList << openPagesItem;
    m_sideBar = new Core::SideBar(itemList,
                                  QList<Core::SideBarItem *>() << contentItem
                                  << (openPagesItem ? openPagesItem : indexItem));
    m_sideBar->setShortcutMap(shortcutMap);
    m_sideBar->setCloseWhenEmpty(true);
    m_sideBarSplitter->insertWidget(0, m_sideBar);
    m_sideBarSplitter->setStretchFactor(0, 0);
    m_sideBarSplitter->setStretchFactor(1, 1);
    if (m_style != ModeWidget)
        m_sideBar->setVisible(false);
    m_sideBar->resize(250, size().height());
    m_sideBar->readSettings(Core::ICore::settings(), sideBarSettingsKey());
    m_sideBarSplitter->setSizes(QList<int>() << m_sideBar->size().width() << 300);

    connect(m_contentsAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(QLatin1String(Constants::HELP_CONTENTS));
    });
    connect(m_indexAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(QLatin1String(Constants::HELP_INDEX));
    });
    connect(m_bookmarkAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(QLatin1String(Constants::HELP_BOOKMARKS));
    });
    connect(m_searchAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(QLatin1String(Constants::HELP_SEARCH));
    });
    if (m_openPagesAction) {
        connect(m_openPagesAction, &QAction::triggered, m_sideBar, [this]() {
            m_sideBar->activateItem(QLatin1String(Constants::HELP_OPENPAGES));
        });
    }
}

QString HelpWidget::sideBarSettingsKey() const
{
    switch (m_style) {
    case ModeWidget:
        return QLatin1String(kModeSideBarSettingsKey);
    case ExternalWindow:
        return QLatin1String(kWindowSideBarSettingsKey);
    case SideBarWidget:
        QTC_CHECK(false);
        break;
    }
    return QString();
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

void HelpWidget::addViewer(HelpViewer *viewer)
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
        connect(viewer, &HelpViewer::titleChanged, this, &HelpWidget::updateWindowTitle);

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

int HelpWidget::viewerCount() const
{
    return m_viewerStack->count();
}

HelpViewer *HelpWidget::viewerAt(int index) const
{
    return qobject_cast<HelpViewer *>(m_viewerStack->widget(index));
}

void HelpWidget::open(const QUrl &url, bool newPage)
{
    if (newPage)
        OpenPagesManager::instance().createPage(url);
    else
        setSource(url);
}

void HelpWidget::showTopicChooser(const QMap<QString, QUrl> &links,
    const QString &keyword, bool newPage)
{
    TopicChooser tc(this, keyword, links);
    if (tc.exec() == QDialog::Accepted)
        open(tc.link(), newPage);
}

void HelpWidget::activateSideBarItem(const QString &id)
{
    QTC_ASSERT(m_sideBar, return);
    m_sideBar->activateItem(id);
}

void HelpWidget::setSource(const QUrl &url)
{
    HelpViewer* viewer = currentViewer();
    QTC_ASSERT(viewer, return);
    viewer->setSource(url);
    viewer->setFocus(Qt::OtherFocusReason);
}

void HelpWidget::openFromSearch(const QUrl &url, const QStringList &searchTerms, bool newPage)
{
    m_searchTerms = searchTerms;
    if (newPage)
        OpenPagesManager::instance().createPage(url);
    else {
        HelpViewer* viewer = currentViewer();
        QTC_ASSERT(viewer, return);
        viewer->setSource(url);
        viewer->setFocus(Qt::OtherFocusReason);
    }
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
        const bool closeOnReturn = LocalHelpManager::returnOnClose();
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
    if (m_searchTerms.isEmpty())
        return;
    HelpViewer *viewer = qobject_cast<HelpViewer *>(sender());
    QTC_ASSERT(viewer, return);
    foreach (const QString& term, m_searchTerms)
        viewer->findText(term, 0, false, true);
    m_searchTerms.clear();
}

} // Internal
} // Help
