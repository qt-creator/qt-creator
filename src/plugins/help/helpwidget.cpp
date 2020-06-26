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
#include <coreplugin/icore.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/locator/locatormanager.h>
#include <coreplugin/minisplitter.h>
#include <coreplugin/sidebar.h>
#include <coreplugin/minisplitter.h>
#include <texteditor/texteditorconstants.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QPrintDialog>
#include <QPrinter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QToolButton>

#ifdef HELP_NEW_FILTER_ENGINE
#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpFilterEngine>
#endif

static const char kWindowSideBarSettingsKey[] = "Help/WindowSideBar";
static const char kModeSideBarSettingsKey[] = "Help/ModeSideBar";

namespace Help {
namespace Internal {

OpenPagesModel::OpenPagesModel(HelpWidget *parent)
    : m_parent(parent)
{}

int OpenPagesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_parent->viewerCount();
}

int OpenPagesModel::columnCount(const QModelIndex &parent) const
{
    // page title + funky close button
    return parent.isValid() ? 0 : 2;
}

QVariant OpenPagesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() >= columnCount() - 1)
        return QVariant();

    switch (role) {
    case Qt::ToolTipRole:
        return m_parent->viewerAt(index.row())->source().toString();
    case Qt::DisplayRole: {
        const QString title = m_parent->viewerAt(index.row())->title();
        return title.isEmpty() ? HelpWidget::tr("(Untitled)") : title;
    }
    default:
        break;
    }
    return QVariant();
}

static bool isBookmarkable(const QUrl &url)
{
    return !url.isEmpty() && url != QUrl(Help::Constants::AboutBlank);
}

static bool isTargetOfContextHelp(HelpWidget::WidgetStyle style)
{
    const Core::HelpManager::HelpViewerLocation option = LocalHelpManager::contextHelpOption();
    switch (style) {
    case HelpWidget::ModeWidget:
        return option == Core::HelpManager::HelpModeAlways;
    case HelpWidget::ExternalWindow:
        return option == Core::HelpManager::ExternalHelpAlways;
    case HelpWidget::SideBarWidget:
        return option == Core::HelpManager::SideBySideAlways
               || option == Core::HelpManager::SideBySideIfPossible;
    }
    QTC_CHECK(false);
    return false;
}

static QString helpTargetActionText(Core::HelpManager::HelpViewerLocation option)
{
    switch (option) {
    case Core::HelpManager::SideBySideIfPossible:
        return HelpWidget::tr("Show Context Help Side-by-Side if Possible");
    case Core::HelpManager::SideBySideAlways:
        return HelpWidget::tr("Always Show Context Help Side-by-Side");
    case Core::HelpManager::HelpModeAlways:
        return HelpWidget::tr("Always Show Context Help in Help Mode");
    case Core::HelpManager::ExternalHelpAlways:
        return HelpWidget::tr("Always Show Context Help in External Window");
    }
    QTC_CHECK(false);
    return {};
}

static Core::HelpManager::HelpViewerLocation optionForStyle(HelpWidget::WidgetStyle style)
{
    switch (style) {
    case HelpWidget::ModeWidget:
        return Core::HelpManager::HelpModeAlways;
    case HelpWidget::ExternalWindow:
        return Core::HelpManager::ExternalHelpAlways;
    case HelpWidget::SideBarWidget:
        return Core::HelpManager::SideBySideIfPossible;
    }
    QTC_CHECK(false);
    return Core::HelpManager::SideBySideIfPossible;
}

static QString helpTargetActionToolTip(HelpWidget::WidgetStyle style)
{
    return helpTargetActionText(optionForStyle(style));
}

static QMenu *createHelpTargetMenu(QWidget *parent)
{
    auto menu = new QMenu(parent);

    const auto addAction = [menu](Core::HelpManager::HelpViewerLocation option) {
        QAction *action = menu->addAction(helpTargetActionText(option));
        action->setCheckable(true);
        action->setChecked(LocalHelpManager::contextHelpOption() == option);
        QObject::connect(action, &QAction::triggered, menu, [option] {
            LocalHelpManager::setContextHelpOption(option);
        });
        QObject::connect(LocalHelpManager::instance(),
                         &LocalHelpManager::contextHelpOptionChanged,
                         menu,
                         [action, option](Core::HelpManager::HelpViewerLocation newOption) {
                             action->setChecked(newOption == option);
                         });
    };
    addAction(Core::HelpManager::SideBySideIfPossible);
    addAction(Core::HelpManager::SideBySideAlways);
    addAction(Core::HelpManager::HelpModeAlways);
    addAction(Core::HelpManager::ExternalHelpAlways);
    return menu;
}

HelpWidget::HelpWidget(const Core::Context &context, WidgetStyle style, QWidget *parent)
    : QWidget(parent)
    , m_model(this)
    , m_style(style)
{
    m_viewerStack = new QStackedWidget;

    if (supportsPages())
        m_openPagesManager = new OpenPagesManager(this);

    auto topLayout = new QVBoxLayout;
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);
    setLayout(topLayout);

    auto hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);
    topLayout->addLayout(hLayout, 10);

    m_sideBarSplitter = new Core::MiniSplitter(this);
    m_sideBarSplitter->setOpaqueResize(false);
    hLayout->addWidget(m_sideBarSplitter);

    auto toolBar = new Utils::StyledBar();
    auto layout = new QHBoxLayout(toolBar);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    auto rightSide = new QWidget(this);
    m_sideBarSplitter->insertWidget(1, rightSide);
    auto vLayout = new QVBoxLayout(rightSide);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(0);
    vLayout->addWidget(toolBar);
    vLayout->addWidget(m_viewerStack);
    auto fth = new Core::FindToolBarPlaceHolder(this);
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
                                    Core::Context(Utils::Id("Help.Window.").withSuffix(++windowId)));
        setAttribute(Qt::WA_QuitOnClose, false); // don't prevent Qt Creator from closing
    }
    if (style != SideBarWidget) {
        m_toggleSideBarAction = new QAction(Utils::Icons::TOGGLE_LEFT_SIDEBAR_TOOLBAR.icon(),
                                            QCoreApplication::translate("Core", Core::Constants::TR_SHOW_LEFT_SIDEBAR),
                                            toolBar);
        m_toggleSideBarAction->setCheckable(true);
        m_toggleSideBarAction->setChecked(false);
        cmd = Core::ActionManager::registerAction(m_toggleSideBarAction,
                                                  Core::Constants::TOGGLE_LEFT_SIDEBAR, context);
        connect(m_toggleSideBarAction, &QAction::toggled, m_toggleSideBarAction,
                [this](bool checked) {
                    m_toggleSideBarAction->setText(
                        QCoreApplication::translate("Core",
                                                    checked ? Core::Constants::TR_HIDE_LEFT_SIDEBAR
                                                            : Core::Constants::TR_SHOW_LEFT_SIDEBAR));
                });
        addSideBar();
        m_toggleSideBarAction->setChecked(m_sideBar->isVisibleTo(this));
        connect(m_toggleSideBarAction, &QAction::triggered, m_sideBar, &Core::SideBar::setVisible);
        connect(m_sideBar, &Core::SideBar::sideBarClosed, m_toggleSideBarAction, [this]() {
            m_toggleSideBarAction->setChecked(false);
        });
        if (style == ExternalWindow) {
            auto statusBar = new QStatusBar;
            topLayout->addWidget(statusBar);
            auto splitter = new Core::NonResizingSplitter(statusBar);
            statusBar->addPermanentWidget(splitter, 10);
            auto statusBarWidget = new QWidget;
            auto statusBarWidgetLayout = new QHBoxLayout;
            statusBarWidgetLayout->setContentsMargins(0, 0, 3, 0);
            statusBarWidget->setLayout(statusBarWidgetLayout);
            splitter->addWidget(statusBarWidget);
            splitter->addWidget(new QWidget);
            auto locatorWidget = Core::LocatorManager::createLocatorInputWidget(window());
            statusBarWidgetLayout->addWidget(Core::Command::toolButtonWithAppendedShortcut(
                                                 m_toggleSideBarAction, cmd));
            statusBarWidgetLayout->addWidget(locatorWidget);
        }
    }

    if (style != ModeWidget) {
        m_switchToHelp = new QAction(tr("Open in Help Mode"), toolBar);
        cmd = Core::ActionManager::registerAction(m_switchToHelp, Constants::CONTEXT_HELP, context);
        connect(m_switchToHelp, &QAction::triggered, this, [this] {
            postRequestShowHelpUrl(Core::HelpManager::HelpModeAlways);
        });
        layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_switchToHelp, cmd));
    }

    m_homeAction = new QAction(Utils::Icons::HOME_TOOLBAR.icon(), tr("Home"), this);
    cmd = Core::ActionManager::registerAction(m_homeAction, Constants::HELP_HOME, context);
    connect(m_homeAction, &QAction::triggered, this, &HelpWidget::goHome);
    layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_homeAction, cmd));

    m_backAction = new QAction(Utils::Icons::PREV_TOOLBAR.icon(), tr("Back"), toolBar);
    connect(m_backAction, &QAction::triggered, this, &HelpWidget::backward);
    m_backMenu = new QMenu(toolBar);
    connect(m_backMenu, &QMenu::aboutToShow, this, &HelpWidget::updateBackMenu);
    m_backAction->setMenu(m_backMenu);
    cmd = Core::ActionManager::registerAction(m_backAction, Constants::HELP_PREVIOUS, context);
    cmd->setDefaultKeySequence(QKeySequence::Back);
    button = Core::Command::toolButtonWithAppendedShortcut(m_backAction, cmd);
    button->setPopupMode(QToolButton::DelayedPopup);
    layout->addWidget(button);

    m_forwardAction = new QAction(Utils::Icons::NEXT_TOOLBAR.icon(), tr("Forward"), toolBar);
    connect(m_forwardAction, &QAction::triggered, this, &HelpWidget::forward);
    m_forwardMenu = new QMenu(toolBar);
    connect(m_forwardMenu, &QMenu::aboutToShow, this, &HelpWidget::updateForwardMenu);
    m_forwardAction->setMenu(m_forwardMenu);
    cmd = Core::ActionManager::registerAction(m_forwardAction, Constants::HELP_NEXT, context);
    cmd->setDefaultKeySequence(QKeySequence::Forward);
    button = Core::Command::toolButtonWithAppendedShortcut(m_forwardAction, cmd);
    button->setPopupMode(QToolButton::DelayedPopup);
    layout->addWidget(button);

    m_addBookmarkAction = new QAction(Utils::Icons::BOOKMARK_TOOLBAR.icon(), tr("Add Bookmark"), this);
    cmd = Core::ActionManager::registerAction(m_addBookmarkAction, Constants::HELP_ADDBOOKMARK, context);
    cmd->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+M") : tr("Ctrl+M")));
    connect(m_addBookmarkAction, &QAction::triggered, this, &HelpWidget::addBookmark);
    layout->addWidget(new Utils::StyledSeparator(toolBar));
    layout->addWidget(Core::Command::toolButtonWithAppendedShortcut(m_addBookmarkAction, cmd));

    m_openOnlineDocumentationAction = new QAction(Utils::Icons::ONLINE_TOOLBAR.icon(), tr("Open Online Documentation..."), this);
    cmd = Core::ActionManager::registerAction(m_openOnlineDocumentationAction, Constants::HELP_OPENONLINE, context);
    connect(m_openOnlineDocumentationAction, &QAction::triggered, this, &HelpWidget::openOnlineDocumentation);
    layout->addWidget(
        Core::Command::toolButtonWithAppendedShortcut(m_openOnlineDocumentationAction, cmd));

    auto helpTargetAction = new QAction(Utils::Icons::LINK_TOOLBAR.icon(),
                                        helpTargetActionToolTip(style),
                                        this);
    helpTargetAction->setCheckable(true);
    helpTargetAction->setChecked(isTargetOfContextHelp(style));
    cmd = Core::ActionManager::registerAction(helpTargetAction, "Help.OpenContextHelpHere", context);
    QToolButton *helpTargetButton = Core::Command::toolButtonWithAppendedShortcut(helpTargetAction,
                                                                                  cmd);
    helpTargetButton->setProperty("noArrow", true);
    helpTargetButton->setPopupMode(QToolButton::DelayedPopup);
    helpTargetButton->setMenu(createHelpTargetMenu(helpTargetButton));
    connect(LocalHelpManager::instance(),
            &LocalHelpManager::contextHelpOptionChanged,
            [this, helpTargetAction] {
                helpTargetAction->setChecked(isTargetOfContextHelp(m_style));
            });
    connect(helpTargetAction,
            &QAction::triggered,
            this,
            [this, helpTargetAction, helpTargetButton](bool checked) {
                if (checked) {
                    LocalHelpManager::setContextHelpOption(optionForStyle(m_style));
                } else {
                    helpTargetAction->setChecked(true);
                    helpTargetButton->showMenu();
                }
            });
    layout->addWidget(helpTargetButton);

    if (supportsPages()) {
        layout->addWidget(new Utils::StyledSeparator(toolBar));
        layout->addWidget(m_openPagesManager->openPagesComboBox(), 10);

        m_filterComboBox = new QComboBox;
        m_filterComboBox->setMinimumContentsLength(15);
        layout->addWidget(m_filterComboBox);
#ifndef HELP_NEW_FILTER_ENGINE
        m_filterComboBox->setModel(LocalHelpManager::filterModel());
        m_filterComboBox->setCurrentIndex(LocalHelpManager::filterIndex());
        connect(m_filterComboBox, QOverload<int>::of(&QComboBox::activated),
                LocalHelpManager::instance(), &LocalHelpManager::setFilterIndex);
        connect(LocalHelpManager::instance(),
                &LocalHelpManager::filterIndexChanged,
                m_filterComboBox,
                &QComboBox::setCurrentIndex);
#else
        connect(&LocalHelpManager::helpEngine(), &QHelpEngine::setupFinished,
                this, &HelpWidget::setupFilterCombo, Qt::QueuedConnection);
        connect(m_filterComboBox, QOverload<int>::of(&QComboBox::activated),
                this, &HelpWidget::filterDocumentation);
        connect(LocalHelpManager::filterEngine(), &QHelpFilterEngine::filterActivated,
                this, &HelpWidget::currentFilterChanged);
#endif

        Core::ActionContainer *windowMenu = Core::ActionManager::actionContainer(
            Core::Constants::M_WINDOW);
        if (QTC_GUARD(windowMenu)) {
            // reuse EditorManager constants to avoid a second pair of menu actions
            m_gotoPrevious = new QAction(this);
            cmd = Core::ActionManager::registerAction(m_gotoPrevious,
                                                      Core::Constants::GOTOPREVINHISTORY,
                                                      context);
            windowMenu->addAction(cmd, Core::Constants::G_WINDOW_NAVIGATE);
            connect(m_gotoPrevious,
                    &QAction::triggered,
                    openPagesManager(),
                    &OpenPagesManager::gotoPreviousPage);

            m_gotoNext = new QAction(this);
            cmd = Core::ActionManager::registerAction(m_gotoNext,
                                                      Core::Constants::GOTONEXTINHISTORY,
                                                      context);
            windowMenu->addAction(cmd, Core::Constants::G_WINDOW_NAVIGATE);
            connect(m_gotoNext,
                    &QAction::triggered,
                    openPagesManager(),
                    &OpenPagesManager::gotoNextPage);
        }
    } else {
        layout->addWidget(new QLabel(), 10);
    }

    layout->addStretch();

    m_printAction = new QAction(this);
    Core::ActionManager::registerAction(m_printAction, Core::Constants::PRINT, context);
    connect(m_printAction, &QAction::triggered, this, [this]() { print(currentViewer()); });

    m_copy = new QAction(this);
    Core::ActionManager::registerAction(m_copy, Core::Constants::COPY, context);
    connect(m_copy, &QAction::triggered, this, &HelpWidget::copy);

    Core::ActionContainer *advancedMenu = Core::ActionManager::actionContainer(Core::Constants::M_EDIT_ADVANCED);
    if (QTC_GUARD(advancedMenu)) {
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

    auto openButton = new QToolButton;
    openButton->setIcon(Utils::Icons::SPLIT_HORIZONTAL_TOOLBAR.icon());
    openButton->setPopupMode(QToolButton::InstantPopup);
    openButton->setProperty("noArrow", true);
    layout->addWidget(openButton);
    auto openMenu = new QMenu(openButton);
    openButton->setMenu(openMenu);

    if (m_switchToHelp)
        openMenu->addAction(m_switchToHelp);
    if (style != SideBarWidget) {
        QAction *openSideBySide = openMenu->addAction(tr("Open in Edit Mode"));
        connect(openSideBySide, &QAction::triggered, this, [this]() {
            postRequestShowHelpUrl(Core::HelpManager::SideBySideAlways);
        });
    }
    if (supportsPages()) {
        QAction *openPage = openMenu->addAction(tr("Open in New Page"));
        connect(openPage, &QAction::triggered, this, [this]() {
            if (HelpViewer *viewer = currentViewer())
                openNewPage(viewer->source());
        });
    }
    if (style != ExternalWindow) {
        QAction *openExternal = openMenu->addAction(tr("Open in Window"));
        connect(openExternal, &QAction::triggered, this, [this]() {
            postRequestShowHelpUrl(Core::HelpManager::ExternalHelpAlways);
        });
    }
    openMenu->addSeparator();

    const Utils::Icon &icon = style == SideBarWidget ? Utils::Icons::CLOSE_SPLIT_RIGHT
                                                     : Utils::Icons::CLOSE_TOOLBAR;
    m_closeAction = new QAction(icon.icon(), QString(), toolBar);
    connect(m_closeAction, &QAction::triggered, this, &HelpWidget::closeButtonClicked);
    connect(m_closeAction, &QAction::triggered, this, [this] {
        if (viewerCount() > 1)
            closeCurrentPage();
    });
    button = new QToolButton;
    button->setDefaultAction(m_closeAction);
    layout->addWidget(button);

    QAction *reload = openMenu->addAction(tr("Reload"));
    connect(reload, &QAction::triggered, this, [this]() {
        const int index = m_viewerStack->currentIndex();
        HelpViewer *previous = currentViewer();
        insertViewer(index, previous->source(), previous->scale());
        removeViewerAt(index + 1);
        setCurrentIndex(index);
    });

    if (style != ModeWidget) {
        addViewer({});
        setCurrentIndex(0);
    }
}

#ifdef HELP_NEW_FILTER_ENGINE

void HelpWidget::setupFilterCombo()
{
    const QString currentFilter = LocalHelpManager::filterEngine()->activeFilter();
    m_filterComboBox->clear();
    m_filterComboBox->addItem(tr("Unfiltered"));
    const QStringList allFilters = LocalHelpManager::filterEngine()->filters();
    if (!allFilters.isEmpty())
        m_filterComboBox->insertSeparator(1);
    for (const QString &filter : allFilters)
        m_filterComboBox->addItem(filter, filter);

    int idx = m_filterComboBox->findData(currentFilter);
    if (idx < 0)
        idx = 0;
    m_filterComboBox->setCurrentIndex(idx);
}

void HelpWidget::filterDocumentation(int filterIndex)
{
    const QString filter = m_filterComboBox->itemData(filterIndex).toString();
    LocalHelpManager::filterEngine()->setActiveFilter(filter);
}

void HelpWidget::currentFilterChanged(const QString &filter)
{
    int index = m_filterComboBox->findData(filter);
    if (index < 0)
        index = 0;
    m_filterComboBox->setCurrentIndex(index);
}

#endif

HelpWidget::~HelpWidget()
{
    saveState();
    if (m_sideBar) {
        m_sideBar->saveSettings(Core::ICore::settings(), sideBarSettingsKey());
        Core::ActionManager::unregisterAction(m_contentsAction, Constants::HELP_CONTENTS);
        Core::ActionManager::unregisterAction(m_indexAction, Constants::HELP_INDEX);
        Core::ActionManager::unregisterAction(m_bookmarkAction, Constants::HELP_BOOKMARKS);
        Core::ActionManager::unregisterAction(m_searchAction, Constants::HELP_SEARCH);
        if (m_openPagesAction)
            Core::ActionManager::unregisterAction(m_openPagesAction, Constants::HELP_OPENPAGES);
    }
    Core::ActionManager::unregisterAction(m_copy, Core::Constants::COPY);
    Core::ActionManager::unregisterAction(m_printAction, Core::Constants::PRINT);
    if (m_toggleSideBarAction)
        Core::ActionManager::unregisterAction(m_toggleSideBarAction, Core::Constants::TOGGLE_LEFT_SIDEBAR);
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
    delete m_openPagesManager;
}

QAbstractItemModel *HelpWidget::model()
{
    return &m_model;
}

HelpWidget::WidgetStyle HelpWidget::widgetStyle() const
{
    return m_style;
}

void HelpWidget::addSideBar()
{
    QMap<QString, Core::Command *> shortcutMap;
    Core::Command *cmd;

    auto contentWindow = new ContentWindow;
    auto contentItem = new Core::SideBarItem(contentWindow, Constants::HELP_CONTENTS);
    contentWindow->setOpenInNewPageActionVisible(supportsPages());
    contentWindow->setWindowTitle(HelpPlugin::tr(Constants::SB_CONTENTS));
    connect(contentWindow, &ContentWindow::linkActivated,
            this, &HelpWidget::open);
    m_contentsAction = new QAction(HelpPlugin::tr(Constants::SB_CONTENTS), this);
    cmd = Core::ActionManager::registerAction(m_contentsAction, Constants::HELP_CONTENTS, m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+Shift+C")
                                                                  : tr("Ctrl+Shift+C")));
    shortcutMap.insert(Constants::HELP_CONTENTS, cmd);

    auto indexWindow = new IndexWindow();
    auto indexItem = new Core::SideBarItem(indexWindow, Constants::HELP_INDEX);
    indexWindow->setOpenInNewPageActionVisible(supportsPages());
    indexWindow->setWindowTitle(HelpPlugin::tr(Constants::SB_INDEX));
    connect(indexWindow, &IndexWindow::linksActivated,
        this, &HelpWidget::showLinks);
    m_indexAction = new QAction(HelpPlugin::tr(Constants::SB_INDEX), this);
    cmd = Core::ActionManager::registerAction(m_indexAction, Constants::HELP_INDEX, m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+I")
                                                                  : tr("Ctrl+Shift+I")));
    shortcutMap.insert(Constants::HELP_INDEX, cmd);

    auto bookmarkWidget = new BookmarkWidget(&LocalHelpManager::bookmarkManager());
    bookmarkWidget->setWindowTitle(HelpPlugin::tr(Constants::SB_BOOKMARKS));
    bookmarkWidget->setOpenInNewPageActionVisible(supportsPages());
    auto bookmarkItem = new Core::SideBarItem(bookmarkWidget, Constants::HELP_BOOKMARKS);
    connect(bookmarkWidget, &BookmarkWidget::linkActivated, this, &HelpWidget::setSource);
    m_bookmarkAction = new QAction(tr("Activate Help Bookmarks View"), this);
    cmd = Core::ActionManager::registerAction(m_bookmarkAction, Constants::HELP_BOOKMARKS,
                                              m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Alt+Meta+M")
                                                                  : tr("Ctrl+Shift+B")));
    shortcutMap.insert(Constants::HELP_BOOKMARKS, cmd);

    auto searchItem = new SearchSideBarItem;
    connect(searchItem, &SearchSideBarItem::linkActivated, this, &HelpWidget::openFromSearch);
    m_searchAction = new QAction(tr("Activate Help Search View"), this);
    cmd = Core::ActionManager::registerAction(m_searchAction, Constants::HELP_SEARCH,
                                              m_context->context());
    cmd->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+/")
                                                                  : tr("Ctrl+Shift+/")));
    shortcutMap.insert(Constants::HELP_SEARCH, cmd);

    Core::SideBarItem *openPagesItem = nullptr;
    QWidget *openPagesWidget = m_openPagesManager->openPagesWidget();
    openPagesWidget->setWindowTitle(HelpPlugin::tr(Constants::SB_OPENPAGES));
    openPagesItem = new Core::SideBarItem(openPagesWidget, Constants::HELP_OPENPAGES);
    m_openPagesAction = new QAction(tr("Activate Open Help Pages View"), this);
    cmd = Core::ActionManager::registerAction(m_openPagesAction,
                                              Constants::HELP_OPENPAGES,
                                              m_context->context());
    cmd->setDefaultKeySequence(
        QKeySequence(Core::useMacShortcuts ? tr("Meta+O") : tr("Ctrl+Shift+O")));
    shortcutMap.insert(Constants::HELP_OPENPAGES, cmd);

    QList<Core::SideBarItem *> itemList = {contentItem, indexItem, bookmarkItem, searchItem};
    if (openPagesItem)
         itemList << openPagesItem;
    m_sideBar = new Core::SideBar(itemList, {contentItem, (openPagesItem ? openPagesItem : indexItem)});
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
        m_sideBar->activateItem(Constants::HELP_CONTENTS);
    });
    connect(m_indexAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(Constants::HELP_INDEX);
    });
    connect(m_bookmarkAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(Constants::HELP_BOOKMARKS);
    });
    connect(m_searchAction, &QAction::triggered, m_sideBar, [this]() {
        m_sideBar->activateItem(Constants::HELP_SEARCH);
    });
    if (m_openPagesAction) {
        connect(m_openPagesAction, &QAction::triggered, m_sideBar, [this]() {
            m_sideBar->activateItem(Constants::HELP_OPENPAGES);
        });
    }
}

QString HelpWidget::sideBarSettingsKey() const
{
    switch (m_style) {
    case ModeWidget:
        return QString(kModeSideBarSettingsKey);
    case ExternalWindow:
        return QString(kWindowSideBarSettingsKey);
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

int HelpWidget::currentIndex() const
{
    return m_viewerStack->currentIndex();
}

void HelpWidget::setCurrentIndex(int index)
{
    HelpViewer *viewer = viewerAt(index);
    m_viewerStack->setCurrentIndex(index);
    m_backAction->setEnabled(viewer->isBackwardAvailable());
    m_forwardAction->setEnabled(viewer->isForwardAvailable());
    m_addBookmarkAction->setEnabled(isBookmarkable(viewer->source()));
    m_openOnlineDocumentationAction->setEnabled(
        LocalHelpManager::canOpenOnlineHelp(viewer->source()));
    if (m_style == ExternalWindow)
        updateWindowTitle();
    emit currentIndexChanged(index);
}

HelpViewer *HelpWidget::addViewer(const QUrl &url, qreal zoom)
{
    return insertViewer(m_viewerStack->count(), url, zoom);
}

HelpViewer *HelpWidget::insertViewer(int index, const QUrl &url, qreal zoom)
{
    m_model.beginInsertRows({}, index, index);
    HelpViewer *viewer = HelpPlugin::createHelpViewer(zoom);
    m_viewerStack->insertWidget(index, viewer);
    viewer->setFocus(Qt::OtherFocusReason);
    viewer->setActionVisible(HelpViewer::Action::NewPage, supportsPages());
    viewer->setActionVisible(HelpViewer::Action::ExternalWindow, m_style != ExternalWindow);
    connect(viewer, &HelpViewer::sourceChanged, this, [viewer, this](const QUrl &url) {
        if (currentViewer() == viewer) {
            m_addBookmarkAction->setEnabled(isBookmarkable(url));
            m_openOnlineDocumentationAction->setEnabled(LocalHelpManager::canOpenOnlineHelp(url));
        }
    });
    connect(viewer, &HelpViewer::forwardAvailable, this, [viewer, this](bool available) {
        if (currentViewer() == viewer)
            m_forwardAction->setEnabled(available);
    });
    connect(viewer, &HelpViewer::backwardAvailable, this, [viewer, this](bool available) {
        if (currentViewer() == viewer)
            m_backAction->setEnabled(available);
    });
    connect(viewer, &HelpViewer::printRequested, this, [viewer, this]() { print(viewer); });
    if (m_style == ExternalWindow)
        connect(viewer, &HelpViewer::titleChanged, this, &HelpWidget::updateWindowTitle);
    connect(viewer, &HelpViewer::titleChanged, &m_model, [this, viewer] {
        const int i = indexOf(viewer);
        QTC_ASSERT(i >= 0, return );
        m_model.dataChanged(m_model.index(i, 0), m_model.index(i, 0));
    });

    connect(viewer, &HelpViewer::loadFinished, this, &HelpWidget::highlightSearchTerms);
    connect(viewer, &HelpViewer::newPageRequested, this, &HelpWidget::openNewPage);
    connect(viewer, &HelpViewer::externalPageRequested, this, [this](const QUrl &url) {
        emit requestShowHelpUrl(url, Core::HelpManager::ExternalHelpAlways);
    });
    updateCloseButton();
    m_model.endInsertRows();
    if (url.isValid())
        viewer->setSource(url);
    return viewer;
}

void HelpWidget::removeViewerAt(int index)
{
    HelpViewer *viewerWidget = viewerAt(index);
    QTC_ASSERT(viewerWidget, return);
    m_model.beginRemoveRows(QModelIndex(), index, index);
    viewerWidget->stop();
    m_viewerStack->removeWidget(viewerWidget);
    m_model.endRemoveRows();
    delete viewerWidget;
    if (m_viewerStack->currentWidget())
        setCurrentIndex(m_viewerStack->currentIndex());
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
        openNewPage(url);
    else
        setSource(url);
}

HelpViewer *HelpWidget::openNewPage(const QUrl &url)
{
    if (url.isValid() && HelpViewer::launchWithExternalApp(url))
        return nullptr;

    HelpViewer *page = addViewer(url);
    setCurrentIndex(viewerCount() - 1);
    return page;
}

void HelpWidget::showLinks(const QMultiMap<QString, QUrl> &links,
    const QString &keyword, bool newPage)
{
    if (links.size() < 1)
        return;
    if (links.size() == 1) {
        open(links.first(), newPage);
    } else {
        TopicChooser tc(this, keyword, links);
        if (tc.exec() == QDialog::Accepted)
            open(tc.link(), newPage);
    }
}

void HelpWidget::activateSideBarItem(const QString &id)
{
    QTC_ASSERT(m_sideBar, return);
    m_sideBar->activateItem(id);
}

OpenPagesManager *HelpWidget::openPagesManager() const
{
    return m_openPagesManager;
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
        openNewPage(url);
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

int HelpWidget::indexOf(HelpViewer *viewer) const
{
    for (int i = 0; i < viewerCount(); ++i)
        if (viewerAt(i) == viewer)
            return i;
    return -1;
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

void HelpWidget::postRequestShowHelpUrl(Core::HelpManager::HelpViewerLocation location)
{
    QTC_ASSERT(currentViewer(), return);
    emit requestShowHelpUrl(currentViewer()->source(), location);
    closeWindow();
}

void HelpWidget::closeCurrentPage()
{
    removeViewerAt(currentIndex());
}

void HelpWidget::saveState() const
{
    // TODO generalize
    if (m_style == ModeWidget) {
        QList<qreal> zoomFactors;
        QStringList currentPages;
        for (int i = 0; i < viewerCount(); ++i) {
            const HelpViewer *const viewer = viewerAt(i);
            const QUrl &source = viewer->source();
            if (source.isValid()) {
                currentPages.append(source.toString());
                zoomFactors.append(viewer->scale());
            }
        }

        LocalHelpManager::setLastShownPages(currentPages);
        LocalHelpManager::setLastShownPagesZoom(zoomFactors);
        LocalHelpManager::setLastSelectedTab(currentIndex());
    }
}

bool HelpWidget::supportsPages() const
{
    return m_style != SideBarWidget;
}

void HelpWidget::closeWindow()
{
    if (m_style == SideBarWidget)
        emit closeButtonClicked();
    else if (m_style == ExternalWindow)
        close();
}

void HelpWidget::updateCloseButton()
{
    if (supportsPages()) {
        const bool closeOnReturn = LocalHelpManager::returnOnClose() && m_style == ModeWidget;
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
    if (!isBookmarkable(url))
        return;

    BookmarkManager *manager = &LocalHelpManager::bookmarkManager();
    manager->showBookmarkDialog(this, viewer->title(), url);
}

void HelpWidget::openOnlineDocumentation()
{
    HelpViewer *viewer = currentViewer();
    QTC_ASSERT(viewer, return);
    LocalHelpManager::openOnlineHelp(viewer->source());
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
    auto viewer = qobject_cast<HelpViewer *>(sender());
    QTC_ASSERT(viewer, return);
    foreach (const QString& term, m_searchTerms)
        viewer->findText(term, {}, false, true);
    m_searchTerms.clear();
}

} // Internal
} // Help
