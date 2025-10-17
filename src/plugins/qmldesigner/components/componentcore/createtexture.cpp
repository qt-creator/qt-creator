// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "createtexture.h"
#include "componentcoretracing.h"

#include <abstractview.h>
#include <asset.h>
#include <designmodewidget.h>
#include <documentmanager.h>
#include <modelnode.h>
#include <modelnodeoperations.h>
#include <modelutils.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <qmlobjectnode.h>
#include <uniquename.h>
#include <utils3d.h>
#include <variantproperty.h>

#include <coreplugin/messagebox.h>

#include <QRegularExpression>
#include <QStack>
#include <QTimer>
#include <QUrl>

namespace QmlDesigner {

using ComponentCoreTracing::category;
using NanotraceHR::keyValue;

using namespace Qt::StringLiterals;

static QString nameFromId(const QString &id, const QString &defaultName)
{
    if (id.isEmpty())
        return defaultName;

    QString newName = id;
    static const QRegularExpression sideUnderscores{R"((?:^_+)|(?:_+$))"};
    static const QRegularExpression underscores{R"((?:_+))"};
    static const QRegularExpression camelCases{R"((?:[A-Z](?=[a-z]))|(?:(?<=[a-z])[A-Z]))"};

    newName.remove(sideUnderscores);

    // Insert underscore to camel case edges
    QRegularExpressionMatchIterator caseMatch = camelCases.globalMatch(newName);
    QStack<int> camelCaseIndexes;
    while (caseMatch.hasNext())
        camelCaseIndexes.push(caseMatch.next().capturedStart());
    while (!camelCaseIndexes.isEmpty())
        newName.insert(camelCaseIndexes.pop(), '_');

    // Replace underscored joints with space
    newName.replace(underscores, " ");
    newName = newName.trimmed();

    if (newName.isEmpty())
        return defaultName;

    newName[0] = newName[0].toUpper();
    return newName;
}

CreateTexture::CreateTexture(AbstractView *view)
    : m_view{view}
{
    NanotraceHR::Tracer tracer{"create texture constructor", category()};
}

ModelNode CreateTexture::execute()
{
    NanotraceHR::Tracer tracer{"create texture execute", category()};

    ModelNode matLib = Utils3D::materialLibraryNode(m_view);
    if (!matLib.isValid())
        return {};

    ModelNode newTextureNode;
    m_view->executeInTransaction(__FUNCTION__, [&]() {
        newTextureNode = m_view->createModelNode("Texture");

        newTextureNode.ensureIdExists();
        VariantProperty textureName = newTextureNode.variantProperty("objectName");
        textureName.setValue(nameFromId(newTextureNode.id(), "Texture"_L1));
        matLib.defaultNodeListProperty().reparentHere(newTextureNode);
    });

    QTimer::singleShot(0, m_view, [newTextureNode]() {
        Utils3D::openNodeInPropertyEditor(newTextureNode);
    });

    return newTextureNode;
}

ModelNode CreateTexture::execute(const QString &filePath, AddTextureMode mode, int sceneId)
{
    NanotraceHR::Tracer tracer{"create texture execute from file",
                               category(),
                               keyValue("file path", filePath),
                               keyValue("scene id", sceneId)};

    Asset asset(filePath);
    if (!asset.isValidTextureSource())
        return {};

    Utils::FilePath assetPath = Utils::FilePath::fromString(filePath);
    if (!assetPath.isChildOf(DocumentManager::currentResourcePath())) {
        if (!addFileToProject(filePath))
            return {};

        // after importing change assetPath to path in project
        QString assetName = assetPath.fileName();
        assetPath = ModelNodeOperations::getImagesDefaultDirectory().pathAppended(assetName);
    }

    ModelNode texture = createTextureFromImage(assetPath, mode);
    if (!texture.isValid())
        return {};

    if (mode == AddTextureMode::LightProbe && sceneId != -1)
        Utils3D::assignTextureAsLightProbe(m_view, texture, sceneId);

    QTimer::singleShot(0, m_view, [view = m_view, texture]() {
        if (view && view->model() && texture.isValid()) {
            QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser");
            Utils3D::openNodeInPropertyEditor(texture);
        }
    });

    return texture;
}

/*!
 * \brief Duplicates a texture node.
 * The duplicate keeps a copy of all of the source node properties.
 * It also generates a unique id, and also a name based on its id.
 * \param texture The source texture
 * \return Duplicate texture
 */
ModelNode CreateTexture::execute(const ModelNode &texture)
{
    NanotraceHR::Tracer tracer{"create texture execute duplicate",
                               category(),
                               keyValue("node", texture)};

    QTC_ASSERT(texture.isValid(), return {});

    if (!m_view || !m_view->model())
        return {};

    TypeName matType = texture.documentTypeRepresentation();
    QmlObjectNode sourceTexture(texture);
    ModelNode duplicateTextureNode;
    QList<AbstractProperty> dynamicProps;

    m_view->executeInTransaction(__FUNCTION__, [&] {
        ModelNode matLib = Utils3D::materialLibraryNode(m_view);
        if (!matLib.isValid())
            return;

        // create the duplicate texture
        QmlObjectNode duplicateTex = m_view->createModelNode(matType);

        duplicateTextureNode = duplicateTex.modelNode();

        // sync properties. Only the base state is duplicated.
        const QList<AbstractProperty> props = texture.properties();
        for (const AbstractProperty &prop : props) {
            if (prop.name() == "objectName" || prop.name() == "data")
                continue;

            if (prop.isVariantProperty()) {
                if (prop.isDynamic())
                    dynamicProps.append(prop);
                else
                    duplicateTex.setVariantProperty(prop.name(), prop.toVariantProperty().value());
            } else if (prop.isBindingProperty()) {
                if (prop.isDynamic()) {
                    dynamicProps.append(prop);
                } else {
                    duplicateTex.setBindingProperty(prop.name(),
                                                    prop.toBindingProperty().expression());
                }
            }
        }

        const QString preferredId = m_view->model()->generateNewId(sourceTexture.id(), "Texture");
        duplicateTextureNode.setIdWithoutRefactoring(preferredId);
        duplicateTextureNode.ensureIdExists();
        duplicateTex.setVariantProperty("objectName", nameFromId(duplicateTex.id(), "Texture"_L1));

        matLib.defaultNodeListProperty().reparentHere(duplicateTex);
    });

    // For some reason, creating dynamic properties in the same transaction doesn't work, so
    // let's do it in separate transaction.
    // TODO: Fix the issue and merge transactions (QDS-8094)
    if (!dynamicProps.isEmpty()) {
        m_view->executeInTransaction(__FUNCTION__, [&] {
            for (const AbstractProperty &prop : std::as_const(dynamicProps)) {
                if (prop.isVariantProperty()) {
                    VariantProperty variantProp = duplicateTextureNode.variantProperty(prop.name());
                    variantProp.setDynamicTypeNameAndValue(prop.dynamicTypeName(),
                                                           prop.toVariantProperty().value());
                } else if (prop.isBindingProperty()) {
                    BindingProperty bindingProp = duplicateTextureNode.bindingProperty(prop.name());
                    bindingProp.setDynamicTypeNameAndExpression(prop.dynamicTypeName(),
                                                                prop.toBindingProperty().expression());
                }
            }
        });
    }

    return duplicateTextureNode;
}

void CreateTexture::execute(const QStringList &filePaths, AddTextureMode mode, int sceneId)
{
    NanotraceHR::Tracer tracer{"create texture execute from multiple files",
                               category(),
                               keyValue("file paths", filePaths),
                               keyValue("scene id", sceneId)};

    for (const QString &path : filePaths)
        execute(path, mode, sceneId);
}

bool CreateTexture::addFileToProject(const QString &filePath)
{
    NanotraceHR::Tracer tracer{"create texture add file to project",
                               category(),
                               keyValue("file path", filePath)};

    AddFilesResult result = ModelNodeOperations::addImageToProject(
        {filePath}, ModelNodeOperations::getImagesDefaultDirectory().toUrlishString(), false);

    if (result.status() == AddFilesResult::Failed) {
        Core::AsynchronousMessageBox::warning(Tr::tr("Failed to Add Texture"),
                                              Tr::tr("Could not add %1 to project.").arg(filePath));
        return false;
    }

    return true;
}

ModelNode CreateTexture::createTextureFromImage(const  Utils::FilePath &assetPath, AddTextureMode mode)
{
    NanotraceHR::Tracer tracer{"create texture from image", category()};

    if (mode != AddTextureMode::Texture && mode != AddTextureMode::LightProbe)
        return {};

    ModelNode matLib = Utils3D::materialLibraryNode(m_view);
    if (!matLib.isValid())
        return {};

    QString textureSource = assetPath.relativePathFromDir(DocumentManager::currentFilePath().parentDir()).toUrlishString();

    ModelNode newTexNode = Utils3D::getTextureDefaultInstance(textureSource, m_view);
    if (!newTexNode.isValid()) {
        newTexNode = m_view->createModelNode("Texture");

        newTexNode.setIdWithoutRefactoring(m_view->model()->generateNewId(assetPath.baseName()));

        VariantProperty textureName = newTexNode.variantProperty("objectName");
        textureName.setValue(nameFromId(newTexNode.id(), "Texture"_L1));

        VariantProperty sourceProp = newTexNode.variantProperty("source");
        sourceProp.setValue(QUrl(textureSource));

        matLib.defaultNodeListProperty().reparentHere(newTexNode);
    }

    return newTexNode;
}

} // namespace QmlDesigner
