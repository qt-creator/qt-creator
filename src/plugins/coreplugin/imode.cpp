// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imode.h"

#include "modemanager.h"

#include <utils/fancymainwindow.h>

#include <aggregation/aggregate.h>

namespace Core {

namespace Internal {

class IModePrivate
{
public:
    QString m_displayName;
    QIcon m_icon;
    QMenu *m_menu = nullptr;
    Utils::FancyMainWindow *m_mainWindow = nullptr;
    int m_priority = -1;
    Utils::Id m_id;
    bool m_isEnabled = true;
};

} // namespace Internal

/*!
    \class Core::IMode
    \inheaderfile coreplugin/imode.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The IMode class represents a mode in \QC.

    This class defines a mode and its representation as a tool button in the
    mode selector on the left side of \QC's main window.

    Modes are used to show a different UI for different development tasks.
    Therefore modes control the layout of most of Qt Creator's main window,
    except for the tool bar on the left side and the status bar. For example
    Edit mode, the most commonly used mode for coding, shows the code editor
    and various navigation and output panes. Debug mode enhances that view with
    a configurable layout of debugging related information. Design mode
    reserves all the main window's space for the graphical editor.

    A mode is an IContext. Set the context's \l{IContext::widget()}{widget}
    to define the mode's layout.

    Adding a mode should be done sparingly, only as a last reserve. Consider if
    your feature can instead be implemented as a INavigationWidgetFactory,
    IOutputPane, \c{Debugger::Utils::Perspective}, separate dialog, or
    specialized IEditor first.

    If you add a mode, consider adding a NavigationWidgetPlaceHolder
    on the left side and a OutputPanePlaceHolder on the bottom of your
    mode's layout.

    Modes automatically register themselves with \QC when they are created and
    unregister themselves when they are destructed.
*/

/*!
    \property Core::IMode::enabled

    This property holds whether the mode is enabled.

    By default, this property is \c true.
*/

/*!
    \property Core::IMode::displayName

    This property holds the display name of the mode.

    The display name is shown under the mode icon in the mode selector.
*/

/*!
    \property Core::IMode::icon

    This property holds the icon of the mode.

    The icon is shown for the mode in the mode selector. Mode icons should
    support the sizes 34x34 pixels and 68x68 pixels for HiDPI.
*/

/*!
    \property Core::IMode::priority

    This property holds the priority of the mode.

    The priority defines the order in which the modes are shown in the mode
    selector. Higher priority moves to mode towards the top. Welcome mode,
    which should stay at the top, has the priority 100. The default priority is
    -1.
*/

/*!
    \property Core::IMode::id

    This property holds the ID of the mode.
*/

/*!
    \property Core::IMode::menu

    This property holds the mode's menu.

    By default, a mode does not have a menu. When you set a menu, it is not
    owned by the mode unless you set the parent explicitly.
*/

/*!
    Creates an IMode with an optional \a parent.

    Registers the mode in \QC.
*/
IMode::IMode(QObject *parent)
    : IContext(parent)
    , m_d(new Internal::IModePrivate)
{
    ModeManager::addMode(this);
}

IMode::~IMode() = default;

QString IMode::displayName() const
{
    return m_d->m_displayName;
}

QIcon IMode::icon() const
{
    return m_d->m_icon;
}

int IMode::priority() const
{
    return m_d->m_priority;
}

Utils::Id IMode::id() const
{
    return m_d->m_id;
}

void IMode::setEnabled(bool enabled)
{
    if (m_d->m_isEnabled == enabled)
        return;
    m_d->m_isEnabled = enabled;
    emit enabledStateChanged(m_d->m_isEnabled);
}

void IMode::setDisplayName(const QString &displayName)
{
    m_d->m_displayName = displayName;
}

void IMode::setIcon(const QIcon &icon)
{
    m_d->m_icon = icon;
}

void IMode::setPriority(int priority)
{
    m_d->m_priority = priority;
}

void IMode::setId(Utils::Id id)
{
    m_d->m_id = id;
}

void IMode::setMenu(QMenu *menu)
{
    m_d->m_menu = menu;
}

Utils::FancyMainWindow *IMode::mainWindow()
{
    if (m_d->m_mainWindow)
        return m_d->m_mainWindow;
    return qobject_cast<Utils::FancyMainWindow *>(widget());
}

void IMode::setMainWindow(Utils::FancyMainWindow *mw)
{
    m_d->m_mainWindow = mw;
    emit ModeManager::instance()->currentMainWindowChanged();
}

bool IMode::isEnabled() const
{
    return m_d->m_isEnabled;
}

QMenu *IMode::menu() const
{
    return m_d->m_menu;
}

} // namespace Core
