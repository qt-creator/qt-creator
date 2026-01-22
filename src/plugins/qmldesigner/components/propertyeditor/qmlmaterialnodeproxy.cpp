// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmaterialnodeproxy.h"

#include "propertyeditortracing.h"

#include <auxiliarydataproperties.h>
#include <designmodewidget.h>
#include <nodeinstanceview.h>
#include <nodelistproperty.h>
#include <qmldesignerplugin.h>
#include <utils3d.h>

#include <qmldesignerutils/stringutils.h>

#include <QQmlEngine>

namespace QmlDesigner {

using QmlDesigner::PropertyEditorTracing::category;

using namespace Qt::StringLiterals;

static bool allMaterialTypesAre(const ModelNodes &materials, const QString &materialType)
{
    if (materials.isEmpty())
        return false;

    for (const ModelNode &material : materials) {
        if (material.exportedTypeName().name.toQString() != materialType)
            return false;
    }

    return true;
}

QmlMaterialNodeProxy::QmlMaterialNodeProxy()
    : QObject()
    , m_previewUpdateTimer(this)
{
    NanotraceHR::Tracer tracer{"qml material node proxy constructor", category()};

    m_previewUpdateTimer.setInterval(200);
    m_previewUpdateTimer.setSingleShot(true);
    m_previewUpdateTimer.callOnTimeout(
        std::bind_front(&QmlMaterialNodeProxy::updatePreviewModel, this));
}

QmlMaterialNodeProxy::~QmlMaterialNodeProxy() = default;

void QmlMaterialNodeProxy::setup(const ModelNodes &editorNodes)
{
    NanotraceHR::Tracer tracer{"qml material node proxy setup", category()};

    QmlObjectNode objectNode = editorNodes.isEmpty() ? ModelNode{} : editorNodes.first();
    const QmlObjectNode material = objectNode.metaInfo().isQtQuick3DMaterial() ? objectNode
                                                                               : QmlObjectNode{};
    setMaterialNode(material);
    setEditorNodes(editorNodes);
    updatePossibleTypes();
    updatePreviewModel();
}

ModelNode QmlMaterialNodeProxy::materialNode() const
{
    NanotraceHR::Tracer tracer{"qml material node proxy material node", category()};

    return m_materialNode;
}

ModelNodes QmlMaterialNodeProxy::editorNodes() const
{
    NanotraceHR::Tracer tracer{"qml material node proxy editor nodes", category()};

    return m_editorNodes;
}

void QmlMaterialNodeProxy::setPossibleTypes(const QStringList &types)
{
    NanotraceHR::Tracer tracer{"qml material node proxy set possible types", category()};

    if (types == m_possibleTypes)
        return;

    m_possibleTypes = types;
    emit possibleTypesChanged();

    updatePossibleTypeIndex();
}

void QmlMaterialNodeProxy::updatePossibleTypes()
{
    NanotraceHR::Tracer tracer{"qml material node proxy update possible types", category()};

    static const QStringList basicTypes{
        "CustomMaterial",
        "DefaultMaterial",
        "PrincipledMaterial",
        "SpecularGlossyMaterial",
    };

    if (!materialNode()) {
        setPossibleTypes({});
        return;
    }

    const QString &matType = materialNode().exportedTypeName().name.toQString();
    const ModelNodes selectedNodes = editorNodes();
    bool allAreBasic = Utils::allOf(selectedNodes, [&](const ModelNode &node) {
        return basicTypes.contains(node.exportedTypeName().name.toQString());
    });

    if (allAreBasic) {
        setPossibleTypes(basicTypes);
        setCurrentType(matType);
    } else if (allMaterialTypesAre(selectedNodes, matType)) {
        setPossibleTypes(QStringList{matType});
        setCurrentType(matType);
    } else {
        setPossibleTypes(QStringList{"multiselection"});
        setCurrentType("multiselection");
    }
}

void QmlMaterialNodeProxy::setCurrentType(const QString &type)
{
    NanotraceHR::Tracer tracer{"qml material node proxy set current type", category()};

    m_currentType = StringUtils::split_last(type).second.toString();
    updatePossibleTypeIndex();
}

void QmlMaterialNodeProxy::toolBarAction(int action)
{
    NanotraceHR::Tracer tracer{"qml material node proxy tool bar action", category()};

    QTC_ASSERT(hasQuick3DImport(), return);

    switch (ToolBarAction(action)) {
    case ToolBarAction::ApplyToSelected: {
        Utils3D::applyMaterialToModels(materialView(),
                                       materialNode(),
                                       Utils3D::getSelectedModels(materialView()));
        break;
    }

    case ToolBarAction::ApplyToSelectedAdd: {
        Utils3D::applyMaterialToModels(materialView(),
                                       materialNode(),
                                       Utils3D::getSelectedModels(materialView()),
                                       true);
        break;
    }

    case ToolBarAction::AddNewMaterial: {
        Utils3D::createMaterial(materialView());
        break;
    }

    case ToolBarAction::DeleteCurrentMaterial: {
        ModelNode nodeToDestroy = materialNode();
        AbstractView *view = materialView();

        QTimer::singleShot(0, view, [view, nodeToDestroy]() mutable {
            view->executeInTransaction(__FUNCTION__, [&nodeToDestroy]() {
                nodeToDestroy.destroy();
            });
        });
        break;
    }

    case ToolBarAction::OpenMaterialBrowser: {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialBrowser", true);
        break;
    }
    }
}

void QmlMaterialNodeProxy::setPreviewEnv(const QString &envAndValue)
{
    NanotraceHR::Tracer tracer{"qml material node proxy set preview env", category()};

    if (envAndValue.isEmpty())
        return;

    if (!hasQuick3DImport())
        return;

    AbstractView *view = m_materialNode.modelNode().view();
    ModelNode rootModelNode = view->rootModelNode();

    QStringList parts = envAndValue.split('=');
    QString env = parts[0];
    QString value;
    if (parts.size() > 1)
        value = parts[1];

    if (env == "Color" && value.isEmpty())
        value = rootModelNode.auxiliaryDataWithDefault(materialPreviewColorDocProperty).toString();

    auto renderPreviews = [rootModelNode](const QString &auxEnv, const QString &auxValue) {
        if (!rootModelNode)
            return;
        rootModelNode.setAuxiliaryData(materialPreviewEnvDocProperty, auxEnv);
        rootModelNode.setAuxiliaryData(materialPreviewEnvProperty, auxEnv);
        rootModelNode.setAuxiliaryData(materialPreviewEnvValueDocProperty, auxValue);
        rootModelNode.setAuxiliaryData(materialPreviewEnvValueProperty, auxValue);

        if (auxEnv == "Color" && !auxValue.isEmpty())
            rootModelNode.setAuxiliaryData(materialPreviewColorDocProperty, auxEnv);

        rootModelNode.view()->emitCustomNotification("refresh_material_browser", {});
    };

    QMetaObject::invokeMethod(view, renderPreviews, env, value);
}

void QmlMaterialNodeProxy::setPreviewModel(const QString &modelStr)
{
    NanotraceHR::Tracer tracer{"qml material node proxy set preview model", category()};

    if (modelStr.isEmpty())
        return;

    if (!hasQuick3DImport())
        return;

    AbstractView *view = m_materialNode.modelNode().view();
    ModelNode rootModelNode = view->rootModelNode();

    auto renderPreviews = [rootModelNode](const QString &modelStr) {
        if (!rootModelNode)
            return;

        rootModelNode.setAuxiliaryData(materialPreviewModelDocProperty, modelStr);
        rootModelNode.setAuxiliaryData(materialPreviewModelProperty, modelStr);

        rootModelNode.view()->emitCustomNotification("refresh_material_browser", {});
    };

    QMetaObject::invokeMethod(view, renderPreviews, modelStr);
}

void QmlMaterialNodeProxy::handleAuxiliaryPropertyChanges()
{
    NanotraceHR::Tracer tracer{"qml material node proxy handle auxiliary property changes",
                               category()};

    if (!hasQuick3DImport())
        return;

    m_previewUpdateTimer.start();
}

void QmlMaterialNodeProxy::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"qml material node proxy register declarative type", category()};

    qmlRegisterType<QmlMaterialNodeProxy>("HelperWidgets", 2, 0, "QmlMaterialNodeProxy");
}

void QmlMaterialNodeProxy::updatePossibleTypeIndex()
{
    NanotraceHR::Tracer tracer{"qml material node proxy update possible type index", category()};

    int newIndex = -1;
    if (!m_currentType.isEmpty())
        newIndex = m_possibleTypes.indexOf(m_currentType);

    // Emit valid possible type index change even if the index doesn't change, as currentIndex on
    // QML side will change to default internally if model is updated
    if (m_possibleTypeIndex != -1 || m_possibleTypeIndex != newIndex) {
        m_possibleTypeIndex = newIndex;
        emit possibleTypeIndexChanged();
    }
}

void QmlMaterialNodeProxy::updatePreviewModel()
{
    NanotraceHR::Tracer tracer{"qml material node proxy update preview model", category()};

    if (!hasQuick3DImport())
        return;

    AbstractView *view = m_materialNode.modelNode().view();
    ModelNode rootModelNode = view->rootModelNode();

    // Read auxiliary preview Data
    QString env = rootModelNode.auxiliaryDataWithDefault(materialPreviewEnvDocProperty).toString();
    QString envValue = rootModelNode.auxiliaryDataWithDefault(materialPreviewEnvValueDocProperty)
                           .toString();
    QString modelStr = rootModelNode.auxiliaryDataWithDefault(materialPreviewModelDocProperty).toString();

    if (!envValue.isEmpty() && env != "Basic") {
        env += '=';
        env += envValue;
    }
    if (env.isEmpty())
        env = "SkyBox=preview_studio";

    if (modelStr.isEmpty())
        modelStr = "#Sphere";

    // Set node proxy properties
    if (m_previewModel != modelStr) {
        m_previewModel = modelStr;
        emit previewModelChanged();
    }

    if (m_previewEnv != env) {
        m_previewEnv = env;
        emit previewEnvChanged();
    }
}

void QmlMaterialNodeProxy::setMaterialNode(const QmlObjectNode &material)
{
    NanotraceHR::Tracer tracer{"qml material node proxy set material node", category()};

    if (material == m_materialNode)
        return;

    m_materialNode = material;
    emit materialNodeChanged();
}

void QmlMaterialNodeProxy::setEditorNodes(const ModelNodes &editorNodes)
{
    NanotraceHR::Tracer tracer{"qml material node proxy set editor nodes", category()};

    m_editorNodes = editorNodes;
}

bool QmlMaterialNodeProxy::hasQuick3DImport() const
{
    NanotraceHR::Tracer tracer{"qml material node proxy has quick3d import", category()};

    return materialNode().isValid() && materialNode().model()->hasImport("QtQuick3D"_L1);
}

AbstractView *QmlMaterialNodeProxy::materialView() const
{
    NanotraceHR::Tracer tracer{"qml material node proxy material view", category()};

    return materialNode().view();
}

} // namespace QmlDesigner
