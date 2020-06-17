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

#include "imode.h"

#include "modemanager.h"

namespace Core {

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
    \property IMode::enabled

    This property holds whether the mode is enabled.

    By default, this property is \c true.
*/

/*!
    \property IMode::displayName

    This property holds the display name of the mode.

    The display name is shown under the mode icon in the mode selector.
*/

/*!
    \property IMode::icon

    This property holds the icon of the mode.

    The icon is shown for the mode in the mode selector. Mode icons should
    support the sizes 34x34 pixels and 68x68 pixels for HiDPI.
*/

/*!
    \property IMode::priority

    This property holds the priority of the mode.

    The priority defines the order in which the modes are shown in the mode
    selector. Higher priority moves to mode towards the top. Welcome mode,
    which should stay at the top, has the priority 100. The default priority is
    -1.
*/

/*!
    \property IMode::id

    This property holds the ID of the mode.
*/

/*!
    \property IMode::menu

    This property holds the mode's menu.

    By default, a mode does not have a menu. When you set a menu, it is not
    owned by the mode unless you set the parent explicitly.
*/

/*!
    Creates an IMode with an optional \a parent.

    Registers the mode in \QC.
*/
IMode::IMode(QObject *parent) : IContext(parent)
{
    ModeManager::instance()->addMode(this);
}

/*!
    Unregisters the mode from \QC and destroys it.
*/
IMode::~IMode()
{
    ModeManager::instance()->removeMode(this);
}

void IMode::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
        return;
    m_isEnabled = enabled;
    emit enabledStateChanged(m_isEnabled);
}

bool IMode::isEnabled() const
{
    return m_isEnabled;
}

} // namespace Core
