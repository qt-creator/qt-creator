/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "numberseriesaction.h"
#include <QStandardItemModel>
#include <QComboBox>
#include <QtDebug>


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
