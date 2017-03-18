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

#include "zoomaction.h"

#include <QComboBox>

namespace QmlDesigner {


ZoomAction::ZoomAction(QObject *parent)
    :  QWidgetAction(parent),
    m_zoomLevel(1.0),
    m_currentComboBoxIndex(8)
{

}

double ZoomAction::zoomLevel() const
{
    return m_zoomLevel;
}

void ZoomAction::zoomIn()
{
    if (m_currentComboBoxIndex > 0)
        emit indexChanged(m_currentComboBoxIndex - 1);
}

void ZoomAction::zoomOut()
{
    if (m_currentComboBoxIndex < (m_comboBoxModel->rowCount() - 1))
        emit indexChanged(m_currentComboBoxIndex + 1);
}

void ZoomAction::setZoomLevel(double zoomLevel)
{
    m_zoomLevel = qBound(0.1, zoomLevel, 16.0);
    emit zoomLevelChanged(m_zoomLevel);
}

QWidget *ZoomAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);

    if (m_comboBoxModel.isNull()) {
        m_comboBoxModel = comboBox->model();
        comboBox->addItem(QLatin1String("6.25 %"), 0.0625);
        comboBox->addItem(QLatin1String("12.5 %"), 0.125);
        comboBox->addItem(QLatin1String("25 %"), 0.25);
        comboBox->addItem(QLatin1String("33 %"), 0.33);
        comboBox->addItem(QLatin1String("50 %"), 0.5);
        comboBox->addItem(QLatin1String("66 %"), 0.66);
        comboBox->addItem(QLatin1String("75 %"), 0.75);
        comboBox->addItem(QLatin1String("90 %"), 0.90);
        comboBox->addItem(QLatin1String("100 %"), 1.0); // initial m_zoomLevel and m_currentComboBoxIndex
        comboBox->addItem(QLatin1String("125 %"), 1.25);
        comboBox->addItem(QLatin1String("150 %"), 1.5);
        comboBox->addItem(QLatin1String("175 %"), 1.75);
        comboBox->addItem(QLatin1String("200 %"), 2.0);
        comboBox->addItem(QLatin1String("300 %"), 3.0);
        comboBox->addItem(QLatin1String("400 %"), 4.0);
        comboBox->addItem(QLatin1String("600 %"), 6.0);
        comboBox->addItem(QLatin1String("800 %"), 8.0);
        comboBox->addItem(QLatin1String("1000 %"), 10.0);
        comboBox->addItem(QLatin1String("1600 %"), 16.0);
    } else {
        comboBox->setModel(m_comboBoxModel.data());
    }

    comboBox->setCurrentIndex(8);
    connect(comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &ZoomAction::emitZoomLevelChanged);
    connect(this, &ZoomAction::indexChanged, comboBox, &QComboBox::setCurrentIndex);

    comboBox->setProperty("hideborder", true);
    return comboBox;
}

void ZoomAction::emitZoomLevelChanged(int index)
{
    m_currentComboBoxIndex = index;

    if (index == -1)
        return;

    QModelIndex modelIndex(m_comboBoxModel.data()->index(index, 0));
    setZoomLevel(m_comboBoxModel.data()->data(modelIndex, Qt::UserRole).toDouble());
}

} // namespace QmlDesigner
