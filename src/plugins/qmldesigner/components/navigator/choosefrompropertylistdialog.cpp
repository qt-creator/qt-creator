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
    // TODO: Metainfo based matching system (QDS-6240)

    // Fall back to a hardcoded list of supported cases:
    // Texture
    //  -> DefaultMaterial
    //  -> PrincipledMaterial
    //  -> SpriteParticle3D
    //  -> TextureInput
    //  -> SceneEnvironment
    // Effect
    //  -> SceneEnvironment
    // Shader, Command, Buffer
    //  -> Pass
    // InstanceListEntry
    //  -> InstanceList
    // Pass
    //  -> Effect
    // Particle3D
    //  -> ParticleEmitter3D
    // ParticleAbstractShape3D
    //  -> ParticleEmitter3D
    //  -> Attractor3D

    const TypeName textureType = "QtQuick3D.Texture";
    if (insertInfo.isSubclassOf(textureType)) {
        const TypeName textureTypeCpp = "<cpp>.QQuick3DTexture";
        if (parentInfo.isSubclassOf("QtQuick3D.DefaultMaterial")
            || parentInfo.isSubclassOf("QtQuick3D.PrincipledMaterial")) {
            // All texture properties are valid targets
            const PropertyNameList targetNodeNameList = parentInfo.propertyNames();
            for (const PropertyName &name : targetNodeNameList) {
                TypeName propType = parentInfo.propertyTypeName(name);
                if (propType == textureType || propType == textureTypeCpp) {
                    propertyList.append(QString::fromLatin1(name));
                    if (breakOnFirst)
                        return;
                }
            }
        } else if (parentInfo.isSubclassOf("QtQuick3D.Particles3D.SpriteParticle3D")) {
            propertyList.append("sprite");
        } else if (parentInfo.isSubclassOf("QtQuick3D.TextureInput")) {
            propertyList.append("texture");
        } else if (parentInfo.isSubclassOf("QtQuick3D.SceneEnvironment")) {
            propertyList.append("lightProbe");
        }
    } else if (insertInfo.isSubclassOf("QtQuick3D.Effect")) {
        if (parentInfo.isSubclassOf("QtQuick3D.SceneEnvironment"))
            propertyList.append("effects");
    } else if (insertInfo.isSubclassOf("QtQuick3D.Shader")) {
        if (parentInfo.isSubclassOf("QtQuick3D.Pass"))
            propertyList.append("shaders");
    } else if (insertInfo.isSubclassOf("QtQuick3D.Command")) {
        if (parentInfo.isSubclassOf("QtQuick3D.Pass"))
            propertyList.append("commands");
    } else if (insertInfo.isSubclassOf("QtQuick3D.Buffer")) {
        if (parentInfo.isSubclassOf("QtQuick3D.Pass"))
            propertyList.append("output");
    } else if (insertInfo.isSubclassOf("QtQuick3D.InstanceListEntry")) {
        if (parentInfo.isSubclassOf("QtQuick3D.InstanceList"))
            propertyList.append("instances");
    } else if (insertInfo.isSubclassOf("QtQuick3D.Pass")) {
        if (parentInfo.isSubclassOf("QtQuick3D.Effect"))
            propertyList.append("passes");
    } else if (insertInfo.isSubclassOf("QtQuick3D.Particles3D.Particle3D")) {
        if (parentInfo.isSubclassOf("QtQuick3D.Particles3D.ParticleEmitter3D"))
            propertyList.append("particle");
    } else if (insertInfo.isSubclassOf("QQuick3DParticleAbstractShape")) {
        if (parentInfo.isSubclassOf("QtQuick3D.Particles3D.ParticleEmitter3D")
                || parentInfo.isSubclassOf("QtQuick3D.Particles3D.Attractor3D"))
            propertyList.append("shape");
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
