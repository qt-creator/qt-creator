/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "numberseriesaction.h"
#include <QStandardItemModel>
#include <QComboBox>
#include <QDebug>


namespace QmlDesigner {

NumberSeriesAction::NumberSeriesAction(QObject *parent) :
    QWidgetAction(parent),
    m_comboBoxModelIndex(-1)
{
}

void NumberSeriesAction::addEntry(const QString &text, const QVariant &value)
{
    if (m_comboBoxModel.isNull())
        m_comboBoxModel = new QStandardItemModel(this);

    QStandardItem *newItem = new QStandardItem(text);
    newItem->setData(value);
    m_comboBoxModel->appendRow(newItem);
}

QVariant NumberSeriesAction::currentValue() const
{
    return m_comboBoxModel->item(m_comboBoxModelIndex)->data();
}

QWidget *NumberSeriesAction::createWidget(QWidget *parent)
{
    QComboBox *comboBox = new QComboBox(parent);

    comboBox->setModel(m_comboBoxModel.data());

    comboBox->setCurrentIndex(m_comboBoxModelIndex);
    connect(comboBox, SIGNAL(currentIndexChanged(int)), SLOT(emitValueChanged(int)));

    return comboBox;
}

void NumberSeriesAction::emitValueChanged(int index)
{
    if (index == -1)
        return;

    m_comboBoxModelIndex = index;

    emit valueChanged(m_comboBoxModel.data()->item(index)->data());
}

void NumberSeriesAction::setCurrentEntryIndex(int index)
{
    Q_ASSERT(index < m_comboBoxModel->rowCount());

    m_comboBoxModelIndex = index;
}

} // namespace QKinecticDesigner
