/****************************************************************************
**
** Copyright (C) 2016 Falko Arps
** Copyright (C) 2016 Sven Klein
** Copyright (C) 2016 Giuliano Schneider
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

#include "ioutputpane.h"

/*!
    \class Core::IOutputPane
    \brief The IOutputPane class is an interface for providing \gui Output panes.

    \mainclass
    \inmodule Qt Creator
*/

/*!
    \enum IOutputPane::Flag

    This enum type controls the behavior of the output pane when it is requested to show itself.

    \value NoModeSwitch
                Does not switch between the modes.
    \value ModeSwitch
                Does switch between the modes.
    \value WithFocus
                Sets focus if canFocus returns true.
    \value EnsureSizeHint
                Ensures the use of the minimum size.
*/

/*!
    \fn IOutputPane::IOutputPane(QObject *parent)

    Constructs an output pane as the child of \a parent.
*/

/*!
    \fn QWidget *IOutputPane::outputWidget(QWidget *parent)

    Returns the output widget (as the child of \a parent) for the output pane.
*/

/*!
    \fn QList<QWidget *> IOutputPane::toolBarWidgets() const

    Returns the toolbar widgets for the output pane.
*/

/*!
    \fn QString IOutputPane::displayName() const

    Returns the translated display name of the output pane.
*/

/*!
    \fn int IOutputPane::priorityInStatusBar() const

    Determines the position of the output pane on the status bar.
    \list
        \li 100 to 0 from front to end
        \li -1 do not show in status bar
    \endlist
*/

/*!
    \fn void IOutputPane::clearContents()

    Is called on selecting the clear button.
*/

/*!
    \fn void IOutputPane::visibilityChanged(bool visible)

    Gets called when the visibility is changed. \a visible is \c true when the output pane is now
    visible or \c false otherwise.
*/

/*!
    \fn void IOutputPane::setFocus()

    Gives focus to the output pane window.
*/

/*!
    \fn bool IOutputPane::hasFocus() const

    Returns \c true when the output pane has focus.

    \sa IOutputPane::canFocus()
*/

/*!
    \fn bool IOutputPane::canFocus() const

    Returns \c true when the output pane can be focused right now (for example, the search
    result window does not want to be focused if there are no results).
*/

/*!
    \fn bool IOutputPane::canNavigate() const

    Determines whether the output pane's navigation buttons can be enabled.
    When this returns \c false, the buttons are disabled and cannot be enabled.

    \sa IOutputPane::canNext()
    \sa IOutputPane::canPrevious()
*/

/*!
    \fn bool IOutputPane::canNext() const

    Determines whether the \gui Next button in the output pane is enabled.
    Is overwritten when \c canNavigate() returns \c false.

    \sa IOutputPane::canNavigate()
    \sa IOutputPane::canPrevious()
    \sa IOutputPane::goToNext()
*/

/*!
    \fn bool IOutputPane::canPrevious() const

    Determines whether the \gui Previous button in the output pane is enabled.
    Is overwritten when \c canNavigate() returns \c false.

    \sa IOutputPane::canNavigate()
    \sa IOutputPane::canNext()
    \sa IOutputPane::goToPrev()
*/

/*!
    \fn void IOutputPane::goToNext()

    Is called on selecting the \gui Next button.

    \sa IOutputPane::canNext()
*/

/*!
    \fn void IOutputPane::goToPrev()

    Is called on selecting the \gui Previous button.

    \sa IOutputPane::canPrevious()
*/

/*!
    \fn void IOutputPane::popup(int flags)

    Emits the signal \c{showPage(int flags)} with the given parameter \a flags.

    \sa IOutputPane::showPage()
*/

/*!
    \fn void IOutputPane::hide()

    Emits the signal \c hidePage().

    \sa IOutputPane::hidePage()
*/

/*!
    \fn void IOutputPane::toggle(int flags)

    Emits the signal \c{togglePage(int flags)} with the given parameter \a flags.

    \sa IOutputPane::togglePage()
*/

/*!
    \fn void IOutputPane::navigateStateChanged()

    Emits the signal \c navigateStateUpdate().

    \sa IOutputPane::navigateStateUpdate()
*/

/*!
    \fn void IOutputPane::flash()

    Emits the signal \c flashButton().

    \sa IOutputPane::flashButton()
*/

/*!
    \fn void IOutputPane::setIconBadgeNumber(int number)

    Emits the signal \c{setBadgeNumber(int number)} with the given parameter \a number.

    \sa IOutputPane::setBadgeNumber()
*/

/*!
    \fn void IOutputPane::showPage(int flags)

    Shows the output pane. The parameter \a flags controls the behavior.

    \sa IOutputPane::Flags
*/

/*!
    \fn void IOutputPane::hidePage()

    Hides the output pane.
*/

/*!
    \fn void IOutputPane::togglePage(int flags)

    Toggles the hide and show states of the output pane. The parameter \a flags controls the
    behavior.

    \sa IOutputPane::hidePage()
    \sa IOutputPane::showPage()
    \sa IOutputPane::Flags
*/

/*!
    \fn void IOutputPane::navigateStateUpdate()

    Notifies the output pane manager that the state of canNext, canPrevious, or canNavigate has
    changed and the buttons need to be updated.
*/

/*!
    \fn void IOutputPane::flashButton()

    Makes the status bar button belonging to the output pane flash.
*/

/*!
    \fn void IOutputPane::setBadgeNumber(int number)

    Displays \a number in the status bar button belonging to the output pane
    (for example, number of issues on building).
*/
