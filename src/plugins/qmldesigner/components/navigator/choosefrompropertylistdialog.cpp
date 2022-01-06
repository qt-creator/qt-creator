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

// This will filter and return possible properties that the given type can be bound to
ChooseFromPropertyListFilter::ChooseFromPropertyListFilter(const NodeMetaInfo &insertInfo,
                                                           const NodeMetaInfo &parentInfo,
                                                           bool breakOnFirst)
{
    TypeName typeName = insertInfo.typeName();
    TypeName superClass = insertInfo.directSuperClass().typeName();
    TypeName nodePackage = insertInfo.typeName().replace(insertInfo.simplifiedTypeName(), "");
    TypeName targetPackage = parentInfo.typeName().replace(parentInfo.simplifiedTypeName(), "");
    if (!nodePackage.contains(targetPackage))
        return;

    // Common base types cause too many rarely valid matches, so they are ignored
    const QSet<TypeName> ignoredTypes {"<cpp>.QObject",
                                       "<cpp>.QQuickItem",
                                       "<cpp>.QQuick3DObject",
                                       "QtQuick.Item",
                                       "QtQuick3D.Object3D",
                                       "QtQuick3D.Node",
                                       "QtQuick3D.Particles3D.ParticleSystem3D"};
    const PropertyNameList targetNodeNameList = parentInfo.propertyNames();
    for (const PropertyName &name : targetNodeNameList) {
        if (!name.contains('.')) {
            TypeName propType = parentInfo.propertyTypeName(name).replace("<cpp>.", targetPackage);
            // Skip properties that are not sub classes of anything
            if (propType.contains('.')
                && !ignoredTypes.contains(propType)
                && (typeName == propType || propType == superClass)
                && parentInfo.propertyIsWritable(propType)) {
                propertyList.append(QString::fromLatin1(name));
                if (breakOnFirst)
                    break;
            }
        }
    }
}

// This dialog displays specified properties and allows the user to choose one
ChooseFromPropertyListDialog::ChooseFromPropertyListDialog(const QStringList &propNames,
                                                           QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ChooseFromPropertyListDialog)
{
    if (propNames.count() == 1) {
       m_selectedProperty = propNames.first().toLatin1();
       m_isSoloProperty = true;
       return;
    }
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

    fillList(propNames);
}

ChooseFromPropertyListDialog::~ChooseFromPropertyListDialog()
{
    delete m_ui;
}

TypeName ChooseFromPropertyListDialog::selectedProperty() const
{
    return m_selectedProperty;
}

// Create dialog for selecting any property matching newNode type
// Subclass type matches are also valid
ChooseFromPropertyListDialog *ChooseFromPropertyListDialog::createIfNeeded(
        const ModelNode &targetNode, const ModelNode &newNode, QWidget *parent)
{
    TypeName typeName = newNode.type();

    // Component matches cases where you don't want to insert a plain component,
    // such as layer.effect. Also, default property is often a Component (typically 'delegate'),
    // and inserting into such property will silently overwrite implicit component, if any.
    if (typeName == "QtQml.Component")
        return nullptr;

    const NodeMetaInfo info = newNode.metaInfo();
    const NodeMetaInfo targetInfo = targetNode.metaInfo();
    ChooseFromPropertyListFilter *filter = new ChooseFromPropertyListFilter(info, targetInfo);

    if (!filter->propertyList.isEmpty())
        return new ChooseFromPropertyListDialog(filter->propertyList, parent);

    return nullptr;
}

// Create dialog for selecting writable properties of exact property type
ChooseFromPropertyListDialog *ChooseFromPropertyListDialog::createIfNeeded(
        const ModelNode &targetNode, TypeName type, QWidget *parent)
{
    const NodeMetaInfo metaInfo = targetNode.metaInfo();
    const PropertyNameList propNames = metaInfo.propertyNames();
    const TypeName property(type);
    QStringList matchingNames;
    for (const auto &propName : propNames) {
        if (metaInfo.propertyTypeName(propName) == property && metaInfo.propertyIsWritable(propName))
            matchingNames.append(QString::fromLatin1(propName));
    }

    if (!matchingNames.isEmpty())
        return new ChooseFromPropertyListDialog(matchingNames, parent);

    return nullptr;
}

void ChooseFromPropertyListDialog::fillList(const QStringList &propNames)
{
    if (propNames.isEmpty())
        return;

    QString defaultProp = propNames.first();
    QStringList sortedNames = propNames;
    sortedNames.sort();
    for (const auto &propName : qAsConst(sortedNames)) {
        QListWidgetItem *newItem = new QListWidgetItem(propName);
        m_ui->listProps->addItem(newItem);
    }

    // Select the default prop
    m_ui->listProps->setCurrentRow(sortedNames.indexOf(defaultProp));
    m_selectedProperty = defaultProp.toLatin1();
}

}
