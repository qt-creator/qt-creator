/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "zoomaction.h"

#include <QComboBox>
#include <QLineEdit>
#include <QEvent>
#include <QCoreApplication>

namespace QmlDesigner {


ZoomAction::ZoomAction(QObject *parent)
    :  QWidgetAction(parent),
    m_zoomLevel(1.0),
    m_currentComboBoxIndex(-1)
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
    if (zoomLevel < .1) {
        m_zoomLevel = 0.1;
    } else if (zoomLevel > 16.0) {
        m_zoomLevel = 16.0;
    } else {
        m_zoomLevel = zoomLevel;
    }

    emit zoomLevelChanged(m_zoomLevel);
}


QWidget *ZoomAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);

    if (m_comboBoxModel.isNull()) {
        m_comboBoxModel = comboBox->model();
        comboBox->addItem("10 %", 0.1);
        comboBox->addItem("25 %", 0.25);
        comboBox->addItem("50 %", 0.5);
        comboBox->addItem("100 %", 1.0);
        comboBox->addItem("200 %", 2.0);
        comboBox->addItem("400 %", 4.0);
        comboBox->addItem("800 %", 8.0);
        comboBox->addItem("1600 %", 16.0);

    } else {
        comboBox->setModel(m_comboBoxModel.data());
    }

    comboBox->setCurrentIndex(3);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(emitZoomLevelChanged(int)));
    connect(this, SIGNAL(indexChanged(int)), comboBox, SLOT(setCurrentIndex(int)));

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
