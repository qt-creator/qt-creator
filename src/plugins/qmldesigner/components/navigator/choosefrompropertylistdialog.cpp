/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "choosefrompropertylistdialog.h"
#include "nodemetainfo.h"
#include "ui_choosefrompropertylistdialog.h"

namespace QmlDesigner {

// This dialog displays all given type properties of an object and allows the user to choose one
ChooseFromPropertyListDialog::ChooseFromPropertyListDialog(const ModelNode &node, TypeName type, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ChooseFromPropertyListDialog)
{
    m_propertyTypeName = type;
    m_ui->setupUi(this);
    setWindowTitle(tr("Select property"));
    m_ui->label->setText(tr("Bind to property:"));
    m_ui->label->setToolTip(tr("Binds this component to the parent's selected property."));
    setFixedSize(size());

    connect(m_ui->listProps, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        m_selectedProperty = item->isSelected() ? item->data(Qt::DisplayRole).toByteArray() : QByteArray();
    });

    connect(m_ui->listProps, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        Q_UNUSED(item)
        QDialog::accept();
    });

    fillList(node);
}

ChooseFromPropertyListDialog::~ChooseFromPropertyListDialog()
{
    delete m_ui;
}

TypeName ChooseFromPropertyListDialog::selectedProperty() const
{
    return m_selectedProperty;
}

void ChooseFromPropertyListDialog::fillList(const ModelNode &node)
{
    // Fill the list with all properties of given type
    const auto metaInfo = node.metaInfo();
    const auto propNames = metaInfo.propertyNames();
    const TypeName property(m_propertyTypeName);
    QStringList nameList;
    for (const auto &propName : propNames) {
        if (metaInfo.propertyTypeName(propName) == property)
            nameList.append(QString::fromLatin1(propName));
    }

    if (!nameList.isEmpty()) {
        QString defaultProp = nameList.first();

        nameList.sort();
        for (const auto &propName : qAsConst(nameList)) {
            QListWidgetItem *newItem = new QListWidgetItem(propName);
            m_ui->listProps->addItem(newItem);
        }

        // Select the default prop
        m_ui->listProps->setCurrentRow(nameList.indexOf(defaultProp));
        m_selectedProperty = defaultProp.toLatin1();
    }
}

}
