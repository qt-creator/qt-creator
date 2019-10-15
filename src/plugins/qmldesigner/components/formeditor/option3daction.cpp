/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "option3daction.h"

#include <QComboBox>
#include <QPainter>

namespace QmlDesigner {

Option3DAction::Option3DAction(QObject *parent) :
    QWidgetAction(parent)
{
}

void Option3DAction::set3DEnabled(bool enabled)
{
    m_comboBox->setCurrentIndex(enabled ? 1 : 0);
}

QWidget *Option3DAction::createWidget(QWidget *parent)
{
    m_comboBox = new QComboBox(parent);
    m_comboBox->setFixedWidth(82);

    m_comboBox->addItem(tr("2D"));
    m_comboBox->addItem(tr("2D/3D"));

    m_comboBox->setCurrentIndex(0);
    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](){
        emit enabledChanged(m_comboBox->currentIndex() != 0);
    });
    connect(m_comboBox, QOverload<int>::of(&QComboBox::activated),
            this, [this](){
        emit activated();
    });

    m_comboBox->setProperty("hideborder", true);
    m_comboBox->setToolTip(tr("Enable/Disable 3D edit mode."));
    return m_comboBox;
}

} // namespace QmlDesigner
