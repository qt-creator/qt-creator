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

#include "choosetexturepropertydialog.h"
#include "nodemetainfo.h"
#include "ui_choosetexturepropertydialog.h"

namespace QmlDesigner {

// This dialog displays all texture properties of an object and allows the user to choose one
ChooseTexturePropertyDialog::ChooseTexturePropertyDialog(const ModelNode &node, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ChooseTexturePropertyDialog)
{
    m_ui->setupUi(this);
    setWindowTitle(tr("Select Texture Property"));
    m_ui->label->setText(tr("Set texture to property:"));
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

ChooseTexturePropertyDialog::~ChooseTexturePropertyDialog()
{
    delete m_ui;
}

TypeName ChooseTexturePropertyDialog::selectedProperty() const
{
    return m_selectedProperty;
}

void ChooseTexturePropertyDialog::fillList(const ModelNode &node)
{
    // Fill the list with all properties of type Texture
    const auto metaInfo = node.metaInfo();
    const auto propNames = metaInfo.propertyNames();
    const TypeName textureProp("QtQuick3D.Texture");
    QStringList nameList;
    for (const auto &propName : propNames) {
        if (metaInfo.propertyTypeName(propName) == textureProp)
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
