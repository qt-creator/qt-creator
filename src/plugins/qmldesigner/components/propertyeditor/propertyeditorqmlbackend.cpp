// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorqmlbackend.h"

#include "instanceimageprovider.h"
#include "propertyeditortracing.h"
#include "propertyeditortransaction.h"
#include "propertyeditorvalue.h"
#include "propertymetainfo.h"

#include "componentcore/utils3d.h"

#include <auxiliarydataproperties.h>
#include <bindingproperty.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmldesignertr.h>
#include <qmlobjectnode.h>
#include <qmltimeline.h>
#include <sourcepathcache.h>
#include <variantproperty.h>

#include <theme.h>

#include <qmldesignerbase/settings/designersettings.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <utils/algorithm.h>
#include <utils/array.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/smallstring.h>

#include <qmlprojectmanager/qmlproject.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QQuickItem>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include <QLoggingCategory>

#include <tuple>

using QmlDesigner::PropertyEditorTracing::category;

static Q_LOGGING_CATEGORY(propertyEditorBenchmark, "qtc.propertyeditor.load", QtWarningMsg)

namespace QmlDesigner {

using namespace Qt::StringLiterals;

static bool isMaterialAuxiliaryKey(AuxiliaryDataKeyView key)
{
    static constexpr auto previewKeys = Utils::to_array<AuxiliaryDataKeyView>(
        materialPreviewEnvDocProperty,
        materialPreviewEnvValueDocProperty,
        materialPreviewModelDocProperty,
        materialPreviewEnvProperty,
        materialPreviewEnvValueProperty,
        materialPreviewModelProperty);

    return std::ranges::find(previewKeys, key) != std::ranges::end(previewKeys);
}

PropertyEditorQmlBackend::PropertyEditorQmlBackend(PropertyEditorView *propertyEditor,
                                                   AsynchronousImageCache &imageCache)
    : m_contextObject(
          std::make_unique<PropertyEditorContextObject>(propertyEditor->dynamicPropertiesModel()))
    , m_view(Utils::makeUniqueObjectPtr<Quick2PropertyEditorView>(imageCache))
    , m_propertyEditorTransaction(std::make_unique<PropertyEditorTransaction>(propertyEditor))
    , m_dummyPropertyEditorValue(std::make_unique<PropertyEditorValue>())
{
    NanotraceHR::Tracer tracer{"property editor qml backend constructor", category()};

    m_contextObject->setQuickWidget(m_view.get());
    m_view->engine()->setOutputWarningsToStandardError(QmlDesignerPlugin::instance()
        ->settings().value(DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS).toBool());

    m_view->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_view->engine()->addImportPath(scriptsEditorResourcesPath() + "/imports");

    m_dummyPropertyEditorValue->setValue(QLatin1String("#000000"));
    context()->setContextProperty(QLatin1String("dummyBackendValue"),
                                  m_dummyPropertyEditorValue.get());
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    m_contextObject->setModel(propertyEditor->model());
    m_contextObject->insertInQmlContext(context());

    QObject::connect(&m_backendValuesPropertyMap,
                     &QQmlPropertyMap::valueChanged,
                     propertyEditor,
                     &PropertyEditorView::changeValue);
}

PropertyEditorQmlBackend::~PropertyEditorQmlBackend()
{
    NanotraceHR::Tracer tracer{"property editor qml backend destructor", category()};
}

namespace {
PropertyName auxNamePostFix(Utils::SmallStringView propertyName)
{
    return PropertyNameView(propertyName) + "__AUX";
}

QVariant properDefaultAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                          AuxiliaryDataKeyDefaultValue key)
{
    const ModelNode node = qmlObjectNode.modelNode();

    if (auto data = node.auxiliaryData(key))
        return *data;

    return getDefaultValueAsQVariant(key);
}

QVariant properDefaultLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                               PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{
        "property editor qml backend proper default layout attached properties", category()};
    const QVariant value = qmlObjectNode.modelValue("Layout."_sv + propertyName);
    QVariant marginsValue = qmlObjectNode.modelValue("Layout.margins");

    if (!marginsValue.isValid())
        marginsValue.setValue(0.0);

    if (value.isValid())
        return value;

    if ("fillHeight" == propertyName || "fillWidth" == propertyName)
        return false;

      if ("minimumWidth" == propertyName || "minimumHeight" == propertyName)
          return 0;

      if ("preferredWidth" == propertyName || "preferredHeight" == propertyName)
          return -1;

      if ("maximumWidth" == propertyName || "maximumHeight" == propertyName)
          return 0xffff;

     if ("columnSpan" == propertyName || "rowSpan" == propertyName)
         return 1;

     if ("topMargin" == propertyName || "bottomMargin" == propertyName ||
         "leftMargin" == propertyName || "rightMargin" == propertyName ||
         "margins" == propertyName)
         return marginsValue;

    return QVariant();
}

QVariant properDefaultInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                                PropertyNameView propertyName)
{
    const QVariant value = qmlObjectNode.modelValue("InsightCategory."_sv + propertyName);

    if (value.isValid())
        return value;

    return QString();
}
} // namespace

void PropertyEditorQmlBackend::setupLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor)
{
    NanotraceHR::Tracer tracer{"property editor qml backend setup layout attached properties",
                               category()};

    if (QmlItemNode(qmlObjectNode).isInLayout()) {
        static constexpr PropertyNameView propertyNames[] = {"alignment",
                                                             "column",
                                                             "columnSpan",
                                                             "fillHeight",
                                                             "fillWidth",
                                                             "maximumHeight",
                                                             "maximumWidth",
                                                             "minimumHeight",
                                                             "minimumWidth",
                                                             "preferredHeight",
                                                             "preferredWidth",
                                                             "row",
                                                             "rowSpan",
                                                             "topMargin",
                                                             "bottomMargin",
                                                             "leftMargin",
                                                             "rightMargin",
                                                             "margins"};

        for (PropertyNameView propertyName : propertyNames) {
            createPropertyEditorValue(qmlObjectNode,
                                      "Layout."_sv + propertyName,
                                      properDefaultLayoutAttachedProperties(qmlObjectNode,
                                                                            propertyName),
                                      propertyEditor);
        }
    }
}

void PropertyEditorQmlBackend::setupInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                                              PropertyEditorView *propertyEditor)
{
    NanotraceHR::Tracer tracer{"property editor qml backend setup insight attached properties",
                               category()};

    const PropertyName propertyName = "category";
    createPropertyEditorValue(qmlObjectNode,
                              "InsightCategory."_sv + propertyName,
                              properDefaultInsightAttachedProperties(qmlObjectNode, propertyName),
                              propertyEditor);
}

void PropertyEditorQmlBackend::setupAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                                        PropertyEditorView *propertyEditor)
{
    NanotraceHR::Tracer tracer{"property editor qml backend setup auxiliary properties", category()};

    const QmlItemNode itemNode(qmlObjectNode);

    auto createProperty = [&](auto &&...properties) {
        (createPropertyEditorValue(qmlObjectNode,
                                   auxNamePostFix(properties.name),
                                   properDefaultAuxiliaryProperties(qmlObjectNode, properties),
                                   propertyEditor),
         ...);
    };

    constexpr auto commonProperties = std::make_tuple(customIdProperty);

    std::apply(createProperty, commonProperties);
}

void PropertyEditorQmlBackend::handleInstancePropertyChangedInModelNodeProxy(
    const ModelNode &modelNode, PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{"property editor qml backend handle instance property changed",
                               category()};

    m_backendModelNode.handleInstancePropertyChanged(modelNode, propertyName);
}

void PropertyEditorQmlBackend::handleAuxiliaryDataChanges(const QmlObjectNode &qmlObjectNode,
                                                          AuxiliaryDataKeyView key)
{
    NanotraceHR::Tracer tracer{"property editor qml backend handle auxiliary data changes",
                               category()};

    if (qmlObjectNode.isRootModelNode()) {
        if (isMaterialAuxiliaryKey(key)) {
            m_backendMaterialNode.handleAuxiliaryPropertyChanges();
            m_view->instanceImageProvider()->invalidate();
        } else if (key == active3dSceneProperty) {
            contextObject()->setHas3DScene(Utils3D::active3DSceneId(qmlObjectNode.model()) != -1);
        }
    }
}

void PropertyEditorQmlBackend::handleVariantPropertyChangedInModelNodeProxy(const VariantProperty &property)
{
    NanotraceHR::Tracer tracer{"property editor qml backend handle variant property changed",
                               category()};

    m_backendModelNode.handleVariantPropertyChanged(property);
    updateInstanceImage();
}

void PropertyEditorQmlBackend::handleBindingPropertyChangedInModelNodeProxy(const BindingProperty &property)
{
    NanotraceHR::Tracer tracer{"property editor qml backend handle binding property changed",
                               category()};

    m_backendModelNode.handleBindingPropertyChanged(property);
    m_backendTextureNode.handleBindingPropertyChanged(property);
    updateInstanceImage();
}

void PropertyEditorQmlBackend::handleBindingPropertyInModelNodeProxyAboutToChange(
    const BindingProperty &property)
{
    NanotraceHR::Tracer tracer{
        "property editor qml backend handle binding property about to change", category()};

    if (m_backendMaterialNode.materialNode()) {
        ModelNode expressionNode = property.resolveToModelNode();
        if (expressionNode.metaInfo().isQtQuick3DTexture())
            updateInstanceImage();
    }
    m_backendTextureNode.handleBindingPropertyChanged(property);
}

void PropertyEditorQmlBackend::handlePropertiesRemovedInModelNodeProxy(const AbstractProperty &property)
{
    NanotraceHR::Tracer tracer{"property editor qml backend handle properties removed", category()};

    m_backendModelNode.handlePropertiesRemoved(property);
    m_backendTextureNode.handlePropertiesRemoved(property);
    updateInstanceImage();
}

void PropertyEditorQmlBackend::handleModelNodePreviewPixmapChanged(const ModelNode &node,
                                                                   const QPixmap &pixmap,
                                                                   const QByteArray &requestId)
{
    InstanceImageProvider *imageProvider = m_view->instanceImageProvider();

    if (!imageProvider)
        return;

    bool imageFed = imageProvider->feedImage(node, pixmap, requestId);
    if (imageFed && !imageProvider->hasPendingRequest())
        refreshPreview();
}

void PropertyEditorQmlBackend::handleModelSelectedNodesChanged(PropertyEditorView *propertyEditor)
{
    NanotraceHR::Tracer tracer{"property editor qml backend handle model selected nodes changed",
                               category()};

    contextObject()->setHas3DModelSelected(!Utils3D::getSelectedModels(propertyEditor).isEmpty());
    m_backendTextureNode.updateSelectionDetails();
}

void PropertyEditorQmlBackend::createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                                         PropertyNameView name,
                                                         const QVariant &value,
                                                         PropertyEditorView *propertyEditor,
                                                         const PropertyMetaInfo &propertyMetaInfo)
{
    NanotraceHR::Tracer tracer{"property editor qml backend create property editor value", category()};

    QString propertyName = QString::fromUtf8(name);
    propertyName.replace('.', '_');
    auto valueObject = propertyValueForName(propertyName);
    if (!valueObject) {
        valueObject = new PropertyEditorValue(&backendValuesPropertyMap());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &QQmlPropertyMap::valueChanged);
        QObject::connect(valueObject, &PropertyEditorValue::expressionChanged, propertyEditor, &PropertyEditorView::changeExpression);
        QObject::connect(valueObject, &PropertyEditorValue::exportPropertyAsAliasRequested, propertyEditor, &PropertyEditorView::exportPropertyAsAlias);
        QObject::connect(valueObject, &PropertyEditorValue::removeAliasExportRequested, propertyEditor, &PropertyEditorView::removeAliasExport);
        backendValuesPropertyMap().insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setModelNodeAndProperty(qmlObjectNode, name, propertyMetaInfo);

    if (qmlObjectNode.propertyAffectedByCurrentState(name)
        && !(qmlObjectNode.hasBindingProperty(name)))
        valueObject->setValue(qmlObjectNode.modelValue(name));

    else
        valueObject->setValue(value);

    if (name != "id" && qmlObjectNode.currentState().isBaseState()
        && qmlObjectNode.modelNode().property(name).isBindingProperty()) {
        valueObject->setExpression(qmlObjectNode.modelNode().bindingProperty(name).expression());
    } else {
        if (qmlObjectNode.hasBindingProperty(name))
            valueObject->setExpression(qmlObjectNode.expression(name));
        else
            valueObject->setExpression(qmlObjectNode.instanceValue(name).toString());
    }
}

void PropertyEditorQmlBackend::setValue(const QmlObjectNode &,
                                        PropertyNameView name,
                                        const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor qml backend set value", category()};

    // Vector*D values need to be split into their subcomponents
    if (value.typeId() == QMetaType::QVector2D) {
        const char *suffix[2] = {"_x", "_y"};
        auto vecValue = value.value<QVector2D>();
        for (int i = 0; i < 2; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = propertyValueForName(QString::fromUtf8(subPropName));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else if (value.typeId() == QMetaType::QVector3D) {
        const char *suffix[3] = {"_x", "_y", "_z"};
        auto vecValue = value.value<QVector3D>();
        for (int i = 0; i < 3; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = propertyValueForName(QString::fromUtf8(subPropName));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else if (value.typeId() == QMetaType::QVector4D) {
        const char *suffix[4] = {"_x", "_y", "_z", "_w"};
        auto vecValue = value.value<QVector4D>();
        for (int i = 0; i < 4; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = propertyValueForName(QString::fromUtf8(subPropName));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else {
        PropertyName propertyName = name.toByteArray();
        propertyName.replace('.', '_');
        auto propertyValue = propertyValueForName(QString::fromUtf8(propertyName));

        if (propertyValue)
            propertyValue->setValue(value);
    }
}

void PropertyEditorQmlBackend::setExpression(PropertyNameView propName, const QString &exp)
{
    NanotraceHR::Tracer tracer{"property editor qml backend set expression", category()};

    PropertyEditorValue *propertyValue = propertyValueForName(QString::fromUtf8(propName));
    if (propertyValue)
        propertyValue->setExpression(exp);
}

QQmlContext *PropertyEditorQmlBackend::context()
{
    NanotraceHR::Tracer tracer{"property editor qml backend context", category()};

    return m_view->rootContext();
}

PropertyEditorContextObject *PropertyEditorQmlBackend::contextObject()
{
    NanotraceHR::Tracer tracer{"property editor qml backend context object", category()};

    return m_contextObject.get();
}

QQuickWidget *PropertyEditorQmlBackend::widget()
{
    NanotraceHR::Tracer tracer{"property editor qml backend widget", category()};

    return m_view.get();
}

void PropertyEditorQmlBackend::setSource(const QUrl &url)
{
    NanotraceHR::Tracer tracer{"property editor qml backend set source", category()};

    m_view->setSource(url);

    const bool showError = qEnvironmentVariableIsSet(Constants::ENVIRONMENT_SHOW_QML_ERRORS);

    if (showError && !m_view->errors().isEmpty()) {
        const QString errMsg = m_view->errors().constFirst().toString();
        Core::AsynchronousMessageBox::warning(Tr::tr("Invalid QML Source"), errMsg);
    }
}

QmlAnchorBindingProxy &PropertyEditorQmlBackend::backendAnchorBinding()
{
    NanotraceHR::Tracer tracer{"property editor qml backend backend anchor binding", category()};

    return m_backendAnchorBinding;
}

QQmlPropertyMap &PropertyEditorQmlBackend::backendValuesPropertyMap()
{
    NanotraceHR::Tracer tracer{"property editor qml backend backend values property map", category()};

    return m_backendValuesPropertyMap;
}

PropertyEditorTransaction *PropertyEditorQmlBackend::propertyEditorTransaction()
{
    NanotraceHR::Tracer tracer{"property editor qml backend property editor transaction", category()};

    return m_propertyEditorTransaction.get();
}

PropertyEditorValue *PropertyEditorQmlBackend::propertyValueForName(const QString &propertyName)
{
    NanotraceHR::Tracer tracer{"property editor qml backend property value for name", category()};

    return variantToPropertyEditorValue(backendValuesPropertyMap().value(propertyName));
}

void QmlDesigner::PropertyEditorQmlBackend::createPropertyEditorValues(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor)
{
    NanotraceHR::Tracer tracer{"property editor qml backend create property editor values",
                               category()};

    for (const auto &property : MetaInfoUtils::addInflatedValueAndReferenceProperties(
             qmlObjectNode.metaInfo().properties())) {
        auto propertyName = property.name();
        createPropertyEditorValue(qmlObjectNode,
                                  propertyName,
                                  qmlObjectNode.instanceValue(propertyName),
                                  propertyEditor,
                                  property.property);
    }
}

PropertyEditorValue *PropertyEditorQmlBackend::insertValue(const QString &name,
                                                           const QVariant &value,
                                                           const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"property editor qml backend insert value", category()};

    auto valueObject = propertyValueForName(name);

    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);

    if (modelNode)
        valueObject->setModelNodeAndProperty(modelNode, name.toUtf8());

    if (value.isValid())
        valueObject->setValue(value);

    QObject::connect(valueObject,
                     &PropertyEditorValue::valueChanged,
                     &backendValuesPropertyMap(),
                     &QQmlPropertyMap::valueChanged);
    m_backendValuesPropertyMap.insert(name, QVariant::fromValue(valueObject));

    return valueObject;
}

void PropertyEditorQmlBackend::updateInstanceImage()
{
    NanotraceHR::Tracer tracer{"property editor qml backend update instance image", category()};

    m_view->instanceImageProvider()->invalidate();
    refreshPreview();
}

void PropertyEditorQmlBackend::setup(const ModelNodes &editorNodes,
                                     const QString &stateName,
                                     const QUrl &qmlSpecificsFile,
                                     PropertyEditorView *propertyEditor)
{
    NanotraceHR::Tracer tracer{"property editor qml backend setup", category()};

    QmlObjectNode qmlObjectNode(editorNodes.isEmpty() ? ModelNode{} : editorNodes.first());
    if (!qmlObjectNode.isValid()) {
        qWarning() << "PropertyEditor: invalid node for setup";
        return;
    }

    m_contextObject->setModel(propertyEditor->model());

    qCInfo(propertyEditorBenchmark) << Q_FUNC_INFO;

    QElapsedTimer time;
    if (propertyEditorBenchmark().isInfoEnabled())
        time.start();

    createPropertyEditorValues(qmlObjectNode, propertyEditor);
    setupLayoutAttachedProperties(qmlObjectNode, propertyEditor);
    setupInsightAttachedProperties(qmlObjectNode, propertyEditor);
    setupAuxiliaryProperties(qmlObjectNode, propertyEditor);

    // model node
    m_backendModelNode.setup(editorNodes);
    context()->setContextProperty("modelNodeBackend", &m_backendModelNode);

    m_backendMaterialNode.setup(editorNodes);
    m_backendTextureNode.setup(qmlObjectNode);

    insertValue(Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY,
                m_backendModelNode.simplifiedTypeName(),
                qmlObjectNode.modelNode());

    insertValue("id"_L1, m_backendModelNode.nodeId(), qmlObjectNode.modelNode());
    insertValue("objectName"_L1, m_backendModelNode.nodeObjectName(), qmlObjectNode.modelNode());

    QmlItemNode itemNode(qmlObjectNode.modelNode());

    // anchors
    m_backendAnchorBinding.setup(qmlObjectNode.modelNode());
    setupContextProperties();

    contextObject()->setHasMultiSelection(m_backendModelNode.multiSelection());

    qCInfo(propertyEditorBenchmark) << "anchors:" << time.elapsed();

    qCInfo(propertyEditorBenchmark) << "context:" << time.elapsed();

    contextObject()->setSpecificsUrl(qmlSpecificsFile);

    qCInfo(propertyEditorBenchmark) << "specifics:" << time.elapsed();

    contextObject()->setStateName(stateName);

    context()->setContextProperty(QLatin1String("propertyCount"),
                                  QVariant(qmlObjectNode.modelNode().properties().size()));

    QStringList stateNames = qmlObjectNode.allStateNames();
    stateNames.prepend("base state");
    contextObject()->setAllStateNames(stateNames);

    contextObject()->setIsBaseState(qmlObjectNode.isInBaseState());

    contextObject()->setHasAliasExport(qmlObjectNode.isAliasExported());

    contextObject()->setHasActiveTimeline(QmlTimeline::hasActiveTimeline(qmlObjectNode.view()));

    contextObject()->setSelectionChanged(false);

    contextObject()->setMajorVersion(-1);
    contextObject()->setMinorVersion(-1);

    const auto version = QmlProjectManager::QmlProject::qtQuickVersion();
    contextObject()->setMajorQtQuickVersion(version.major);
    contextObject()->setMinorQtQuickVersion(version.minor);

    contextObject()->setEditorInstancesCount(propertyEditor->instancesCount());

    contextObject()->setHasMaterialLibrary(Utils3D::materialLibraryNode(propertyEditor).isValid());
    contextObject()->setIsQt6Project(propertyEditor->externalDependencies().isQt6Project());
    contextObject()->setEditorNodes(editorNodes);
    contextObject()->setHasQuick3DImport(propertyEditor->model()->hasImport("QtQuick3D"));

    m_view->instanceImageProvider()->setModelNode(m_backendModelNode.singleSelectedNode());
    updateInstanceImage();

    qCInfo(propertyEditorBenchmark) << "final:" << time.elapsed();
}

QString PropertyEditorQmlBackend::propertyEditorResourcesPath()
{
    NanotraceHR::Tracer tracer{"property editor qml backend property editor resources path",
                               category()};

    return resourcesPath("propertyEditorQmlSources");
}

QString PropertyEditorQmlBackend::scriptsEditorResourcesPath()
{
    NanotraceHR::Tracer tracer{"property editor qml backend scripts editor resources path",
                               category()};

    return resourcesPath("scriptseditor");
}

inline bool dotPropertyHeuristic(const QmlObjectNode &node,
                                 const NodeMetaInfo &type,
                                 PropertyNameView name)
{
    if (!name.contains("."))
        return true;

    if (name.count('.') > 1)
        return false;

    QList<QByteArray> list = name.toByteArray().split('.');
    const PropertyName parentProperty = list.first();
    const PropertyName itemProperty = list.last();

    NodeMetaInfo propertyType = type.property(parentProperty).propertyType();

    NodeMetaInfo itemInfo = node.view()->model()->qtQuickItemMetaInfo();
    NodeMetaInfo textInfo = node.view()->model()->qtQuickTextMetaInfo();
    NodeMetaInfo rectangleInfo = node.view()->model()->qtQuickRectangleMetaInfo();
    NodeMetaInfo imageInfo = node.view()->model()->qtQuickImageMetaInfo();
    NodeMetaInfo fontInfo = node.view()->model()->fontMetaInfo();
    NodeMetaInfo vector4dInfo = node.view()->model()->vector4dMetaInfo();
    NodeMetaInfo textureInfo = node.view()->model()->qtQuick3DTextureMetaInfo();

    if (itemInfo.hasProperty(itemProperty)
        || propertyType.isBasedOn(textInfo, fontInfo, rectangleInfo, imageInfo, vector4dInfo, textureInfo))
        return false;

    return true;
}

TypeName PropertyEditorQmlBackend::fixTypeNameForPanes(const TypeName &typeName)
{
    NanotraceHR::Tracer tracer{"property editor qml backend fix type name for panes", category()};

    TypeName fixedTypeName = typeName;
    fixedTypeName.replace('.', '/');
    return fixedTypeName;
}

QString PropertyEditorQmlBackend::resourcesPath(const QString &dir)
{
    NanotraceHR::Tracer tracer{"property editor qml backend resources path", category()};

#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/" + dir;
#endif
    return Core::ICore::resourcePath("qmldesigner/" + dir).toUrlishString();
}

void PropertyEditorQmlBackend::refreshBackendModel()
{
    NanotraceHR::Tracer tracer{"property editor qml backend refresh backend model", category()};

    m_backendModelNode.refresh();
}

void PropertyEditorQmlBackend::refreshPreview()
{
    NanotraceHR::Tracer tracer{"property editor qml backend refresh preview", category()};

    auto qmlPreview = widget()->rootObject();

    if (qmlPreview && qmlPreview->metaObject()->indexOfMethod("refreshPreview()") > -1)
        QMetaObject::invokeMethod(qmlPreview, "refreshPreview");
}

void PropertyEditorQmlBackend::setupContextProperties()
{
    NanotraceHR::Tracer tracer{"property editor qml backend setup context properties", category()};

    context()->setContextProperties({
        {"modelNodeBackend", QVariant::fromValue(&m_backendModelNode)},
        {"materialNodeBackend", QVariant::fromValue(&m_backendMaterialNode)},
        {"textureNodeBackend", QVariant::fromValue(&m_backendTextureNode)},
        {"anchorBackend", QVariant::fromValue(&m_backendAnchorBinding)},
        {"transaction", QVariant::fromValue(m_propertyEditorTransaction.get())},
        {"dummyBackendValue", QVariant::fromValue(m_dummyPropertyEditorValue.get())},
    });
}

QUrl PropertyEditorQmlBackend::fileToUrl(const QString &filePath)
{
    NanotraceHR::Tracer tracer{"property editor qml backend file to url", category()};

    QUrl fileUrl;

    if (filePath.isEmpty())
        return fileUrl;

    if (filePath.startsWith(':')) {
        fileUrl.setScheme(QLatin1String("qrc"));
        QString path = filePath;
        path.remove(0, 1); // remove trailing ':'
        fileUrl.setPath(path);
    } else {
        fileUrl = QUrl::fromLocalFile(filePath);
    }

    return fileUrl;
}

QUrl PropertyEditorQmlBackend::emptyPaneUrl()
{
    NanotraceHR::Tracer tracer{"property editor qml backend empty pane url", category()};

    return fileToUrl(QDir(propertyEditorResourcesPath()).filePath("QtQuick/emptyPane.qml"_L1));
}

QString PropertyEditorQmlBackend::fileFromUrl(const QUrl &url)
{
    NanotraceHR::Tracer tracer{"property editor qml backend file from url", category()};

    if (url.scheme() == QStringLiteral("qrc")) {
        const QString &path = url.path();
        return QStringLiteral(":") + path;
    }

    return url.toLocalFile();
}

bool PropertyEditorQmlBackend::checkIfUrlExists(const QUrl &url)
{
    NanotraceHR::Tracer tracer{"property editor qml backend check if url exists", category()};

    const QString &file = fileFromUrl(url);
    return !file.isEmpty() && QFileInfo::exists(file);
}

void PropertyEditorQmlBackend::emitSelectionToBeChanged()
{
    NanotraceHR::Tracer tracer{"property editor qml backend emit selection to be changed", category()};

    m_backendModelNode.emitSelectionToBeChanged();
}

void PropertyEditorQmlBackend::emitSelectionChanged()
{
    NanotraceHR::Tracer tracer{"property editor qml backend emit selection changed", category()};

    m_backendModelNode.emitSelectionChanged();
}

void PropertyEditorQmlBackend::setValueforLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                                                   PropertyNameView name)
{
    NanotraceHR::Tracer tracer{
        "property editor qml backend set value for layout attached properties", category()};

    PropertyName propertyName = name.toByteArray();
    propertyName.replace("Layout.", "");
    setValue(qmlObjectNode, name, properDefaultLayoutAttachedProperties(qmlObjectNode, propertyName));

    if (propertyName == "margins") {
        const QVariant marginsValue = properDefaultLayoutAttachedProperties(qmlObjectNode, "margins");
        setValue(qmlObjectNode, "Layout.topMargin", marginsValue);
        setValue(qmlObjectNode, "Layout.bottomMargin", marginsValue);
        setValue(qmlObjectNode, "Layout.leftMargin", marginsValue);
        setValue(qmlObjectNode, "Layout.rightMargin", marginsValue);
    }
}

void PropertyEditorQmlBackend::setValueforInsightAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                                                    PropertyNameView name)
{
    NanotraceHR::Tracer tracer{
        "property editor qml backend set value for insight attached properties", category()};

    PropertyName propertyName = name.toByteArray();
    propertyName.replace("InsightCategory.", "");
    setValue(qmlObjectNode, name, properDefaultInsightAttachedProperties(qmlObjectNode, propertyName));
}

void PropertyEditorQmlBackend::setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                                              AuxiliaryDataKeyView key)
{
    NanotraceHR::Tracer tracer{"property editor qml backend set value for auxiliary properties",
                               category()};

    const PropertyName propertyName = auxNamePostFix(key.name);
    setValue(qmlObjectNode, propertyName, qmlObjectNode.modelNode().auxiliaryDataWithDefault(key));
}

} //QmlDesigner
