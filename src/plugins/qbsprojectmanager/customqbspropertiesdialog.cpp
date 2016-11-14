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

#include "customqbspropertiesdialog.h"
#include "ui_customqbspropertiesdialog.h"

#include <qbs.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QTableWidgetItem>

namespace QbsProjectManager {
namespace Internal {

CustomQbsPropertiesDialog::CustomQbsPropertiesDialog(const QVariantMap &properties, QWidget *parent)
    : QDialog(parent), m_ui(new Ui::CustomQbsPropertiesDialog)
{
    m_ui->setupUi(this);
    m_ui->propertiesTable->setRowCount(properties.count());
    m_ui->propertiesTable->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Value"));
    int currentRow = 0;
    for (QVariantMap::ConstIterator it = properties.constBegin(); it != properties.constEnd();
         ++it) {
        QTableWidgetItem * const nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, it.key());
        m_ui->propertiesTable->setItem(currentRow, 0, nameItem);
        QTableWidgetItem * const valueItem = new QTableWidgetItem;
        valueItem->setData(Qt::DisplayRole, qbs::settingsValueToRepresentation(it.value()));
        m_ui->propertiesTable->setItem(currentRow, 1, valueItem);
        ++currentRow;
    }
    connect(m_ui->addButton, &QAbstractButton::clicked,
            this, &CustomQbsPropertiesDialog::addProperty);
    connect(m_ui->removeButton, &QAbstractButton::clicked,
            this, &CustomQbsPropertiesDialog::removeSelectedProperty);
    connect(m_ui->propertiesTable, &QTableWidget::currentItemChanged,
            this, &CustomQbsPropertiesDialog::handleCurrentItemChanged);
    handleCurrentItemChanged();
}

QVariantMap CustomQbsPropertiesDialog::properties() const
{
    QVariantMap properties;
    for (int row = 0; row < m_ui->propertiesTable->rowCount(); ++row) {
        const QTableWidgetItem * const nameItem = m_ui->propertiesTable->item(row, 0);
        const QString name = nameItem->text();
        if (name.isEmpty())
            continue;
        const QString rawString = m_ui->propertiesTable->item(row, 1)->text();
        properties.insert(name, qbs::representationToSettingsValue(rawString));
    }
    return properties;
}

CustomQbsPropertiesDialog::~CustomQbsPropertiesDialog()
{
    delete m_ui;
}

void CustomQbsPropertiesDialog::addProperty()
{
    const int row = m_ui->propertiesTable->rowCount();
    m_ui->propertiesTable->insertRow(row);
    m_ui->propertiesTable->setItem(row, 0, new QTableWidgetItem);
    m_ui->propertiesTable->setItem(row, 1, new QTableWidgetItem);
}

void CustomQbsPropertiesDialog::removeSelectedProperty()
{
    const QTableWidgetItem * const currentItem = m_ui->propertiesTable->currentItem();
    QTC_ASSERT(currentItem, return);
    m_ui->propertiesTable->removeRow(currentItem->row());
}

void CustomQbsPropertiesDialog::handleCurrentItemChanged()
{
    m_ui->removeButton->setEnabled(m_ui->propertiesTable->currentItem());
}

} // namespace Internal
} // namespace QbsProjectManager
