// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "customqbspropertiesdialog.h"

#include "qbsprofilemanager.h"
#include "qbsprojectmanagertr.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QPushButton>
#include <QTableWidgetItem>

namespace QbsProjectManager {
namespace Internal {

CustomQbsPropertiesDialog::CustomQbsPropertiesDialog(const QVariantMap &properties, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Custom Properties"));

    m_propertiesTable = new QTableWidget;
    m_propertiesTable->setColumnCount(2);
    m_propertiesTable->setRowCount(properties.count());
    m_propertiesTable->setHorizontalHeaderLabels(QStringList() << Tr::tr("Key") << Tr::tr("Value"));
    m_propertiesTable->horizontalHeader()->setStretchLastSection(true);
    m_propertiesTable->verticalHeader()->setVisible(false);
    int currentRow = 0;
    for (QVariantMap::ConstIterator it = properties.constBegin(); it != properties.constEnd();
         ++it) {
        auto * const nameItem = new QTableWidgetItem;
        nameItem->setData(Qt::DisplayRole, it.key());
        m_propertiesTable->setItem(currentRow, 0, nameItem);
        auto * const valueItem = new QTableWidgetItem;
        valueItem->setData(Qt::DisplayRole, toJSLiteral(it.value()));
        m_propertiesTable->setItem(currentRow, 1, valueItem);
        ++currentRow;
    }
    m_removeButton = new QPushButton(Tr::tr("&Remove"));
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        Row {
            m_propertiesTable,
            Column {
                PushButton {
                    text(Tr::tr("&Add")),
                    onClicked([this] { addProperty(); } ),
                },
                m_removeButton,
                st
            },
        },
        buttonBox,
    }.attachTo(this);

    connect(m_removeButton, &QAbstractButton::clicked,
            this, &CustomQbsPropertiesDialog::removeSelectedProperty);
    connect(m_propertiesTable, &QTableWidget::currentItemChanged,
            this, &CustomQbsPropertiesDialog::handleCurrentItemChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CustomQbsPropertiesDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &CustomQbsPropertiesDialog::reject);

    handleCurrentItemChanged();
}

QVariantMap CustomQbsPropertiesDialog::properties() const
{
    QVariantMap properties;
    for (int row = 0; row < m_propertiesTable->rowCount(); ++row) {
        const QTableWidgetItem * const nameItem = m_propertiesTable->item(row, 0);
        const QString name = nameItem->text();
        if (name.isEmpty())
            continue;
        properties.insert(name, fromJSLiteral(m_propertiesTable->item(row, 1)->text()));
    }
    return properties;
}

void CustomQbsPropertiesDialog::addProperty()
{
    const int row = m_propertiesTable->rowCount();
    m_propertiesTable->insertRow(row);
    m_propertiesTable->setItem(row, 0, new QTableWidgetItem);
    m_propertiesTable->setItem(row, 1, new QTableWidgetItem);
}

void CustomQbsPropertiesDialog::removeSelectedProperty()
{
    const QTableWidgetItem * const currentItem = m_propertiesTable->currentItem();
    QTC_ASSERT(currentItem, return);
    m_propertiesTable->removeRow(currentItem->row());
}

void CustomQbsPropertiesDialog::handleCurrentItemChanged()
{
    m_removeButton->setEnabled(m_propertiesTable->currentItem());
}

} // namespace Internal
} // namespace QbsProjectManager
