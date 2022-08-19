// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "customqbspropertiesdialog.h"
#include "ui_customqbspropertiesdialog.h"

#include "qbsprofilemanager.h"

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
        auto * const nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, it.key());
        m_ui->propertiesTable->setItem(currentRow, 0, nameItem);
        auto * const valueItem = new QTableWidgetItem;
        valueItem->setData(Qt::DisplayRole, toJSLiteral(it.value()));
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
        properties.insert(name, fromJSLiteral(m_ui->propertiesTable->item(row, 1)->text()));
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
