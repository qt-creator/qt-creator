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
#include <QAbstractItemView>

namespace QmlDesigner {

const int defaultZoomIndex = 13;

ZoomAction::ZoomAction(QObject *parent)
    :  QWidgetAction(parent),
    m_zoomLevel(1.0),
    m_currentComboBoxIndex(defaultZoomIndex)
{

}

float ZoomAction::zoomLevel() const
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

void ZoomAction::resetZoomLevel()
{
    m_zoomLevel = 1.0;
    m_currentComboBoxIndex = defaultZoomIndex;
    emit reseted();
}

void ZoomAction::setZoomLevel(float zoomLevel)
{
    if (qFuzzyCompare(m_zoomLevel, zoomLevel))
        return;

    forceZoomLevel(zoomLevel);
}

void ZoomAction::forceZoomLevel(float zoomLevel)
{
    m_zoomLevel = qBound(0.01f, zoomLevel, 16.0f);
    emit zoomLevelChanged(m_zoomLevel);
}

//initial m_zoomLevel and m_currentComboBoxIndex
const QVector<float> s_zoomFactors = {0.01f, 0.02f, 0.05f, 0.0625f, 0.1f, 0.125f, 0.2f, 0.25f,
                                      0.33f, 0.5f, 0.66f, 0.75f, 0.9f, 1.0f, 1.1f, 1.25f, 1.33f,
                                      1.5f, 1.66f, 1.75f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 10.0f, 16.0f };

int getZoomIndex(float zoom)
{
    for (int i = 0; i < s_zoomFactors.length(); i++) {
        if (qFuzzyCompare(s_zoomFactors.at(i), zoom))
            return i;
    }
    return -1;
}

float ZoomAction::getClosestZoomLevel(float zoomLevel)
{
    int i = 0;
    while (i < s_zoomFactors.size() && s_zoomFactors[i] < zoomLevel)
        ++i;

    return s_zoomFactors[qBound(0, i - 1, s_zoomFactors.size() - 1)];
}

QWidget *ZoomAction::createWidget(QWidget *parent)
{
    auto comboBox = new QComboBox(parent);

    /*
     * When add zoom levels do not forget to update defaultZoomIndex
     */
    if (m_comboBoxModel.isNull()) {
        m_comboBoxModel = comboBox->model();
        for (float z : s_zoomFactors) {
            const QString name = QString::number(z * 100, 'g', 4) + " %";
            comboBox->addItem(name, z);
        }
    } else {
        comboBox->setModel(m_comboBoxModel.data());
    }

    comboBox->setCurrentIndex(m_currentComboBoxIndex);
    comboBox->setToolTip(comboBox->currentText());
    connect(this, &ZoomAction::reseted, comboBox, [this, comboBox]() {
        blockSignals(true);
        comboBox->setCurrentIndex(m_currentComboBoxIndex);
        blockSignals(false);
    });
    connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, comboBox](int index) {
        m_currentComboBoxIndex = index;

        if (index == -1)
            return;

        const QModelIndex modelIndex(m_comboBoxModel.data()->index(index, 0));
        setZoomLevel(m_comboBoxModel.data()->data(modelIndex, Qt::UserRole).toFloat());
        comboBox->setToolTip(modelIndex.data().toString());
    });

    connect(this, &ZoomAction::indexChanged, comboBox, &QComboBox::setCurrentIndex);

    connect(this, &ZoomAction::zoomLevelChanged, comboBox, [comboBox](double zoom){
        const int index = getZoomIndex(zoom);
        if (comboBox->currentIndex() != index)
            comboBox->setCurrentIndex(index);
    });

    comboBox->setProperty("hideborder", true);
    comboBox->setMaximumWidth(qMax(comboBox->view()->sizeHintForColumn(0) / 2, 16));
    return comboBox;
}

} // namespace QmlDesigner
