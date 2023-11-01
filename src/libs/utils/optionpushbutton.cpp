// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "optionpushbutton.h"

#include <QMouseEvent>
#include <QPushButton>
#include <QStyleOptionButton>

namespace Utils {

/*!
    \class Utils::OptionPushButton
    \inmodule QtCreator

    \brief The OptionPushButton class implements a QPushButton for which the menu is only opened
    if the user presses the menu indicator.

    Use OptionPushButton::setOptionalMenu() to set the menu and its actions.
    If the users clicks on the menu indicator of the push button, this menu is opened, and
    its actions are triggered when the user selects them.

    If the user clicks anywhere else on the button, the QAbstractButton::clicked() signal is
    sent, as if the button didn't have a menu.

    Note: You may not call QPushButton::setMenu(). Use OptionPushButton::setOptionalMenu() instead.
*/

/*!
    Constructs an option push button with parent \a parent.
*/
OptionPushButton::OptionPushButton(QWidget *parent)
    : QPushButton(parent)
{}

/*!
    Constructs an option push button with text \a text and parent \a parent.
*/
OptionPushButton::OptionPushButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{}

/*!
    Associates the popup menu \a menu with this push button.
    This menu is shown if the user clicks on the menu indicator that is shown.
    If the user clicks anywhere else on the button, QAbstractButton::clicked() is sent instead.

    \note Calling this method removes all connections to the QAbstractButton::pressed() signal.

    Ownership of the menu is not transferred to the push button.
*/
void OptionPushButton::setOptionalMenu(QMenu *menu)
{
    setMenu(menu);
    // Hack away that QPushButton opens the menu on "pressed".
    // Also removes all other connections to "pressed".
    disconnect(this, &QPushButton::pressed, 0, 0);
}

/*!
    \internal
*/
void OptionPushButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (!menu() || event->button() != Qt::LeftButton)
        return QPushButton::mouseReleaseEvent(event);
    setDown(false);
    const QRect r = rect();
    if (!r.contains(event->position().toPoint()))
        return;
    QStyleOptionButton option;
    initStyleOption(&option);
    // for Mac style
    const QRect contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &option, this);
    // for Common style
    const int menuButtonSize = style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &option, this);
    // see: newBtn.rect = QRect(ir.right() - mbi - 2, ir.height()/2 - mbi/2 + 3, mbi - 6, mbi - 6);
    // in QCommonStyle::drawControl, CE_PushButtonBevel, QStyleOptionButton::HasMenu
    static const int magicBorder = 4;
    const int clickX = event->position().toPoint().x();
    if ((option.direction == Qt::LeftToRight
         && clickX <= std::min(contentRect.right(), r.right() - menuButtonSize - magicBorder))
        || (option.direction == Qt::RightToLeft
            && clickX >= std::max(contentRect.left(), r.left() + menuButtonSize + magicBorder))) {
        click();
    } else {
        showMenu();
    }
}

} // namespace Utils
