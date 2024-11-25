// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsupport.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "coreplugintr.h"
#include "icore.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QWidget>
#include <QWindowStateChangeEvent>

using namespace Utils;

namespace Core {
namespace Internal {

Q_GLOBAL_STATIC(WindowList, m_windowList)

WindowSupport::WindowSupport(QWidget *window, const Context &context, const Context &actionContext)
    : QObject(window)
    , m_window(window)
{
    m_window->installEventFilter(this);

    IContext::attach(window, context);
    const Context ac = actionContext.isEmpty() ? context : actionContext;

    if (useMacShortcuts) {
        m_minimizeAction = new QAction(this);
        ActionManager::registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, ac);
        connect(m_minimizeAction, &QAction::triggered, m_window, &QWidget::showMinimized);

        m_zoomAction = new QAction(this);
        ActionManager::registerAction(m_zoomAction, Constants::ZOOM_WINDOW, ac);
        connect(m_zoomAction, &QAction::triggered, m_window, [this] {
            if (m_window->isMaximized()) {
                // similar to QWidget::showMaximized
                m_window->ensurePolished();
                m_window->setWindowState(m_window->windowState() & ~Qt::WindowMaximized);
                m_window->setVisible(true);
            } else {
                m_window->showMaximized();
            }
        });

        m_closeAction = new QAction(this);
        ActionManager::registerAction(m_closeAction, Constants::CLOSE_WINDOW, ac);
        connect(m_closeAction, &QAction::triggered, m_window, &QWidget::close, Qt::QueuedConnection);
    }

    auto cmd = ActionManager::command(Constants::TOGGLE_FULLSCREEN); // created in registerDefaultActions()
    if (QTC_GUARD(cmd))
        m_toggleFullScreenAction = cmd->action();
    else
        m_toggleFullScreenAction = new QAction(this);
    updateFullScreenAction();
    connect(m_toggleFullScreenAction, &QAction::triggered, this, &WindowSupport::toggleFullScreen);

    m_windowList->addWindow(window);

    connect(ICore::instance(), &ICore::coreAboutToClose, this, [this] { m_shutdown = true; });
}

WindowSupport::~WindowSupport()
{
    if (!m_shutdown) { // don't update all that stuff if we are shutting down anyhow
        if (useMacShortcuts) {
            ActionManager::unregisterAction(m_minimizeAction, Constants::MINIMIZE_WINDOW);
            ActionManager::unregisterAction(m_zoomAction, Constants::ZOOM_WINDOW);
            ActionManager::unregisterAction(m_closeAction, Constants::CLOSE_WINDOW);
        }
        ActionManager::unregisterAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN);
        m_windowList->removeWindow(m_window);
    }
}

void WindowSupport::setCloseActionEnabled(bool enabled)
{
    if (useMacShortcuts)
        m_closeAction->setEnabled(enabled);
}

bool WindowSupport::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_window)
        return false;
    if (event->type() == QEvent::WindowStateChange) {
        if (Utils::HostOsInfo::isMacHost()) {
            bool minimized = m_window->isMinimized();
            m_minimizeAction->setEnabled(!minimized);
            m_zoomAction->setEnabled(!minimized);
        }
        m_previousWindowState = static_cast<QWindowStateChangeEvent *>(event)->oldState();
        updateFullScreenAction();
    } else if (event->type() == QEvent::WindowActivate) {
        m_windowList->setActiveWindow(m_window);
    } else if (event->type() == QEvent::Hide || event->type() == QEvent::Show) {
        m_windowList->updateVisibility(m_window);
    }
    return false;
}

void WindowSupport::toggleFullScreen()
{
    if (m_window->isFullScreen()) {
        m_window->setWindowState(m_previousWindowState & ~Qt::WindowFullScreen);
    } else {
        m_window->setWindowState(m_window->windowState() | Qt::WindowFullScreen);
    }
}

void WindowSupport::updateFullScreenAction()
{
    if (m_window->isFullScreen()) {
        m_toggleFullScreenAction->setText(Tr::tr("Exit Full Screen"));
    } else {
        if (Utils::HostOsInfo::isMacHost())
            m_toggleFullScreenAction->setText(Tr::tr("Enter Full Screen"));
        else
            m_toggleFullScreenAction->setText(Tr::tr("Full Screen"));
    }
}

WindowList::~WindowList()
{
    qDeleteAll(m_windowActions);
}

void WindowList::addWindow(QWidget *window)
{
#ifdef Q_OS_MACOS
    if (!m_dockMenu) {
        m_dockMenu = new QMenu;
        m_dockMenu->setAsDockMenu();
    }
#endif

    m_windows.append(window);
    Id id = Id("QtCreator.Window.").withSuffix(m_windows.size());
    m_windowActionIds.append(id);
    auto action = new QAction(window->windowTitle());
    m_windowActions.append(action);
    QObject::connect(action, &QAction::triggered,
                     action, [action, this] { activateWindow(action); });
    action->setCheckable(true);
    action->setChecked(false);
    Command *cmd = ActionManager::registerAction(action, id);
    cmd->setAttribute(Command::CA_UpdateText);
    ActionManager::actionContainer(Constants::M_WINDOW)->addAction(cmd, Constants::G_WINDOW_LIST);
    action->setVisible(window->isVisible() || window->isMinimized()); // minimized windows are hidden but should be shown
    QObject::connect(window, &QWidget::windowTitleChanged,
                     window, [window, this] { updateTitle(window); });
    if (m_dockMenu)
        m_dockMenu->addAction(action);
    if (window->isActiveWindow())
        setActiveWindow(window);
}

void WindowList::activateWindow(QAction *action)
{
    int index = m_windowActions.indexOf(action);
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_windows.size(), return);
    QWidget *window = m_windows.at(index);
    if (window->isMinimized())
        window->setWindowState(window->windowState() & ~Qt::WindowMinimized);
    ICore::raiseWindow(window);
}

void WindowList::updateTitle(QWidget *window, int i)
{
    const int index = i < 0 ? m_windows.indexOf(window) : i;
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_windowActions.size(), return);
    QString title = window->windowTitle();
    if (title.endsWith(QStringLiteral("- ") + QGuiApplication::applicationDisplayName()))
        title.chop(12);
    m_windowActions.at(index)->setText(Utils::quoteAmpersands(title.trimmed()));
}

void WindowList::updateVisibility(QWidget *window)
{
    updateVisibility(window, m_windows.indexOf(window));
}

void WindowList::updateVisibility(QWidget *window, int index)
{
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_windowActions.size(), return);
    // minimized windows are hidden, but we still want to show them
    m_windowActions.at(index)->setVisible(window->isVisible() || window->isMinimized());
}

void WindowList::removeWindow(QWidget *window)
{
    // remove window from list,
    // remove last action from menu(s)
    // and update all action titles, starting with the index where the window was
    int index = m_windows.indexOf(window);
    QTC_ASSERT(index >= 0, return);

    ActionManager::unregisterAction(m_windowActions.last(), m_windowActionIds.last());
    delete m_windowActions.takeLast();
    m_windowActionIds.removeLast();

    m_windows.removeOne(window);

    for (int i = index; i < m_windows.size(); ++i) {
        QWidget *window = m_windows.at(i);
        updateTitle(window, i);
        updateVisibility(window, i);
    }
    setActiveWindow(QApplication::activeWindow());
}

void WindowList::setActiveWindow(QWidget *window)
{
    for (int i = 0; i < m_windows.size(); ++i)
        m_windowActions.at(i)->setChecked(m_windows.at(i) == window);
}

} // Internal
} // Core
