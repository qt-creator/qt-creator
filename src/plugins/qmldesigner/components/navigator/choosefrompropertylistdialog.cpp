// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    //  -> SpecularGlossyMaterial
    //  -> SpriteParticle3D
    //  -> TextureInput
    //  -> SceneEnvironment
    //  -> Model
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
    // Material
    //  -> Model
    // BundleMaterial
    //  -> Model
    // Effect
    //  -> Item
    // BakedLightmap
    //  -> Model

    if (insertInfo.isQtQuick3DTexture()) {
        if (parentInfo.isQtQuick3DDefaultMaterial() || parentInfo.isQtQuick3DPrincipledMaterial()
            || parentInfo.isQtQuick3DSpecularGlossyMaterial()) {
            // All texture properties are valid targets
            for (const auto &property : parentInfo.properties()) {
                const auto &propType = property.propertyType();
                if (propType.isQtQuick3DTexture()) {
                    propertyList.append(QString::fromUtf8(property.name()));
                    if (breakOnFirst)
                        return;
                }
            }
        } else if (parentInfo.isQtQuick3DParticles3DSpriteParticle3D()) {
            propertyList.append("sprite");
        } else if (parentInfo.isQtQuick3DTextureInput()) {
            propertyList.append("texture");
        } else if (parentInfo.isQtQuick3DSceneEnvironment()) {
            if (insertInfo.isQtQuick3DCubeMapTexture())
                propertyList.append("skyBoxCubeMap");
            else
                propertyList.append("lightProbe");
        } else if (parentInfo.isQtQuick3DModel()) {
            propertyList.append("materials");
        }
    } else if (insertInfo.isQtQuick3DEffect()) {
        if (parentInfo.isQtQuick3DSceneEnvironment())
            propertyList.append("effects");
    } else if (insertInfo.isQtQuick3DShader()) {
        if (parentInfo.isQtQuick3DPass())
            propertyList.append("shaders");
    } else if (insertInfo.isQtQuick3DCommand()) {
        if (parentInfo.isQtQuick3DPass())
            propertyList.append("commands");
    } else if (insertInfo.isQtQuick3DBuffer()) {
        if (parentInfo.isQtQuick3DPass())
            propertyList.append("output");
    } else if (insertInfo.isQtQuick3DInstanceListEntry()) {
        if (parentInfo.isQtQuick3DInstanceList())
            propertyList.append("instances");
    } else if (insertInfo.isQtQuick3DPass()) {
        if (parentInfo.isQtQuick3DEffect())
            propertyList.append("passes");
    } else if (insertInfo.isQtQuick3DParticles3DParticle3D()) {
        if (parentInfo.isQtQuick3DParticles3DParticleEmitter3D())
            propertyList.append("particle");
    } else if (insertInfo.isQtQuick3DParticlesAbstractShape()) {
        if (parentInfo.isQtQuick3DParticles3DParticleEmitter3D()
            || parentInfo.isQtQuick3DParticles3DAttractor3D())
            propertyList.append("shape");
    } else if (insertInfo.isQtQuick3DMaterial()) {
        if (parentInfo.isQtQuick3DModel())
            propertyList.append("materials");
    } else if (insertInfo.typeName().startsWith("ComponentBundles.MaterialBundle")) {
        if (parentInfo.isQtQuick3DModel())
            propertyList.append("materials");
    } else if (insertInfo.isQtQuick3DBakedLightmap()) {
        if (parentInfo.isQtQuick3DModel())
            propertyList.append("bakedLightmap");
    }
}

// This dialog displays specified properties and allows the user to choose one
ChooseFromPropertyListDialog::ChooseFromPropertyListDialog(const QStringList &propNames,
                                                           QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ChooseFromPropertyListDialog)
{
    if (propNames.size() == 1) {
        m_selectedProperty = propNames.first().toLatin1();
        m_isSoloProperty = true;
        return;
    }
    m_ui->setupUi(this);
    setWindowTitle(tr("Select Property"));
    m_ui->label->setText(tr("Bind to property:"));
    m_ui->label->setToolTip(tr("Binds this component to the parent's selected property."));
    setFixedSize(size());

    connect(m_ui->listProps, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        m_selectedProperty = item->isSelected() ? item->data(Qt::DisplayRole).toByteArray() : QByteArray();
    });

    connect(m_ui->listProps,
            &QListWidget::itemDoubleClicked,
            this,
            [this]([[maybe_unused]] QListWidgetItem *item) { QDialog::accept(); });

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
    const ModelNode &targetNode, const NodeMetaInfo &propertyType, QWidget *parent)
{
    const NodeMetaInfo metaInfo = targetNode.metaInfo();
    QStringList matchingNames;
    for (const auto &property : metaInfo.properties()) {
        if (property.propertyType() == propertyType && property.isWritable())
            matchingNames.append(QString::fromUtf8(property.name()));
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
    for (const auto &propName : std::as_const(sortedNames)) {
        QListWidgetItem *newItem = new QListWidgetItem(propName);
        m_ui->listProps->addItem(newItem);
    }

    // Select the default prop
    m_ui->listProps->setCurrentRow(sortedNames.indexOf(defaultProp));
    m_selectedProperty = defaultProp.toLatin1();
}

}
