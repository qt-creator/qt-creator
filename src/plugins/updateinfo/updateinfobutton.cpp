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

#include "updateinfobutton.h"

#include <coreplugin/coreconstants.h>
#include <utils/stylehelper.h>

#include <QIcon>
#include <QPainter>


namespace UpdateInfo {
namespace Internal {

UpdateInfoButton::UpdateInfoButton(QWidget *parent) :
    QAbstractButton(parent)
{
    setIcon(QIcon(QLatin1String(":/updateinfo/images/update_available_logo.png")));
}

//copied from fancytoolbutton
QSize UpdateInfoButton::minimumSizeHint() const
{
    return QSize(8, 8);
}

//copied from fancytoolbutton
QSize UpdateInfoButton::sizeHint() const
{
    return iconSize().expandedTo(QSize(64, 38));
}

//copied from fancytoolbutton and removed unused things
void UpdateInfoButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    QRect iconRect(0, 0, Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);


    iconRect.moveCenter(rect().center());
    Utils::StyleHelper::drawIconWithShadow(icon(), iconRect, &painter, isEnabled() ? QIcon::Normal : QIcon::Disabled);

}


} //Internal
} //namespace UpdateInfo
