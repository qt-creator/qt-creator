// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorview.h"

#include "propertyeditorqmlbackend.h"
#include "propertyeditortracing.h"
#include "propertyeditortransaction.h"
#include "propertyeditorvalue.h"
#include "propertyeditorwidget.h"

#include "qmldesignerplugin.h"
#include <asset.h>
#include <auxiliarydataproperties.h>
#include <dynamicpropertiesmodel.h>
#include <functional.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmltimeline.h>

#include <rewritingexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <sourcepathstorage/sourcepathcache.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils3d.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QQuickItem>
#include <QScopeGuard>
#include <QShortcut>
#include <QTimer>

enum {
    debug = false
};

namespace QmlDesigner {

using PropertyEditorTracing::category;

constexpr QStringView quick3dImport{u"QtQuick3D"};

static bool propertyIsAttachedLayoutProperty(PropertyNameView propertyName)
{
    return propertyName.contains("Layout.");
}

static bool propertyIsAttachedInsightProperty(PropertyNameView propertyName)
{
    return propertyName.contains("InsightCategory.");
}

static NodeMetaInfo findCommonSuperClass(const NodeMetaInfo &first, const NodeMetaInfo &second)
{
    auto commonPrototype = first.commonPrototype(second);

    return commonPrototype.isValid() ? commonPrototype : first;
}

PropertyEditorView::PropertyEditorView(AsynchronousImageCache &imageCache,
                                       ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
    , m_updateShortcut(nullptr)
    , m_dynamicPropertiesModel(std::make_unique<DynamicPropertiesModel>(true, this))
    , m_stackedWidget(new PropertyEditorWidget())
    , m_qmlBackEndForCurrentType(nullptr)
    , m_propertyComponentGenerator{PropertyEditorQmlBackend::propertyEditorResourcesPath(), model()}
    , m_locked(false)
    , m_manageNotifications(ManageCustomNotifications::Yes)
{
    NanotraceHR::Tracer tracer{"property editor view constructor", category()};

    m_qmlDir = PropertyEditorQmlBackend::propertyEditorResourcesPath();

    if (Utils::HostOsInfo::isMacHost())
        m_updateShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F3), m_stackedWidget);
    else
        m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F3), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &PropertyEditorView::reloadQml);

    m_stackedWidget->setStyleSheet(Theme::replaceCssColors(
        Utils::FileUtils::fetchQrc(":/qmldesigner/stylesheet.css")));
    m_stackedWidget->setMinimumSize(340, 340);
    m_stackedWidget->move(0, 0);
    connect(m_stackedWidget, &PropertyEditorWidget::resized, this, &PropertyEditorView::updateSize);

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));

    m_stackedWidget->setWindowTitle(tr("Properties"));

    m_extraPropertyViewsCallbacks.setTargetNode = std::bind_front(&PropertyEditorView::setTargetNode,
                                                                  this);
}

PropertyEditorView::~PropertyEditorView()
{
    NanotraceHR::Tracer tracer{"property editor view destructor", category()};

    qDeleteAll(m_qmlBackendHash);
}

void PropertyEditorView::changeValue(const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor view change value", category()};

    PropertyName propertyName = name.toUtf8();

    if (propertyName.isNull())
        return;

    if (locked())
        return;

    if (propertyName == Constants::PROPERTY_EDITOR_CLASSNAME_PROPERTY)
        return;

    if (noValidSelection())
        return;

    if (propertyName == "id") {
        PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(QString::fromUtf8(propertyName));
        const QString newId = value->value().toString();

        if (newId == activeNode().id())
            return;

        if (QmlDesigner::ModelNode::isValidId(newId)  && !hasId(newId)) {
            executeInTransaction("PropertyEditorView::changeId",
                                 [&] { activeNode().setIdWithRefactoring(newId); });
        } else {
            m_locked = true;
            value->setValue(activeNode().id());
            m_locked = false;
            QString errMsg = QmlDesigner::ModelNode::getIdValidityErrorMessage(newId);
            if (!errMsg.isEmpty())
                Core::AsynchronousMessageBox::warning(tr("Invalid ID"), errMsg);
            else
                Core::AsynchronousMessageBox::warning(tr("Invalid ID"), tr("%1 already exists.").arg(newId));
        }
        return;
    }

    if (propertyName == "objectName" && currentNodes().size() == 1) {
        if (activeNode().metaInfo().isQtQuick3DMaterial()
            || activeNode().metaInfo().isQtQuick3DTexture()) {
            PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(
                "objectName");
            const QString &newObjectName = value->value().toString();
            QmlObjectNode objectNode(activeNode());
            objectNode.setNameAndId(newObjectName, QString::fromLatin1(activeNode().documentTypeRepresentation()));
            return;
        }
    }

    PropertyName underscoreName(propertyName);
    underscoreName.replace('.', '_');
    PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(QString::fromLatin1(underscoreName));

    if (value == nullptr)
        return;

    if (propertyName.endsWith("__AUX")) {
        commitAuxValueToModel(propertyName, value->value());
        return;
    }

    const NodeMetaInfo metaInfo = QmlObjectNode(activeNode()).modelNode().metaInfo();

    QVariant castedValue;

    if (auto property = metaInfo.property(propertyName)) {
        castedValue = property.castedValue(value->value());
    } else if (propertyIsAttachedLayoutProperty(propertyName)
               || propertyIsAttachedInsightProperty(propertyName)) {
        castedValue = value->value();
    } else {
        qWarning() << "PropertyEditor:" << propertyName << "cannot be casted (metainfo)";
        return ;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << "PropertyEditor:" << propertyName << "not properly casted (metainfo)";
        return ;
    }

    bool propertyTypeUrl = false;

    if (metaInfo.property(propertyName).propertyType().isUrl()) {
        // turn absolute local file paths into relative paths
        propertyTypeUrl = true;
        QString filePath = castedValue.toUrl().toString();
        QFileInfo fi(filePath);
        if (fi.exists() && fi.isAbsolute()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            castedValue = QUrl(fileDir.relativeFilePath(filePath));
        }
    }

    if (name == "state" && castedValue.toString() == "base state")
        castedValue = "";

    if (castedValue.typeId() == QMetaType::QColor) {
        QColor color = castedValue.value<QColor>();
        QColor newColor = QColor(color.name());
        newColor.setAlpha(color.alpha());
        castedValue = QVariant(newColor);
    }

    if (!value->value().isValid()
            || (propertyTypeUrl && value->value().toString().isEmpty())) { // reset
        removePropertyFromModel(propertyName);
    } else {
        // QVector*D(0, 0, 0) detects as null variant though it is valid value
        if (castedValue.isValid()
            && (!castedValue.isNull() || castedValue.typeId() == QMetaType::QVector2D
                || castedValue.typeId() == QMetaType::QVector3D
                || castedValue.typeId() == QMetaType::QVector4D)) {
            commitVariantValueToModel(propertyName, castedValue);
        }
    }
}

static bool isTrueFalseLiteral(const QString &expression)
{
    return (expression.compare("false", Qt::CaseInsensitive) == 0)
           || (expression.compare("true", Qt::CaseInsensitive) == 0);
}

void PropertyEditorView::changeExpression(const QString &propertyName)
{
    NanotraceHR::Tracer tracer{"property editor view change expression", category()};

    PropertyName name = propertyName.toUtf8();

    if (name.isNull())
        return;

    if (locked())
        return;

    if (noValidSelection())
        return;

    const QScopeGuard cleanup([&] { m_locked = false; });
    m_locked = true;

    executeInTransaction("PropertyEditorView::changeExpression", [this, name] {
        PropertyName underscoreName(name);
        underscoreName.replace('.', '_');

        QmlObjectNode qmlObjectNode{activeNode()};
        PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(
            QString::fromUtf8(underscoreName));

        if (!value) {
            qWarning() << "PropertyEditor::changeExpression no value for " << underscoreName;
            return;
        }

        if (value->expression().isEmpty()) {
            value->resetValue();
            return;
        }
        setExpressionOnObjectNode(qmlObjectNode, name, value->expression());
    }); /* end of transaction */
}

void PropertyEditorView::exportPropertyAsAlias(const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor view export property as alias", category()};

    if (name.isNull())
        return;

    if (locked())
        return;

    if (noValidSelection())
        return;

    executeInTransaction("PropertyEditorView::exportPropertyAsAlias",
                         [&]() { generateAliasForProperty(activeNode(), name); });
}

void PropertyEditorView::removeAliasExport(const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor view remove alias export", category()};

    if (name.isNull())
        return;

    if (locked())
        return;

    if (noValidSelection())
        return;

    executeInTransaction("PropertyEditorView::exportPropertyAsAlias",
                         [&]() { removeAliasForProperty(activeNode(), name); });
}

void PropertyEditorView::demoteCustomManagerRole()
{
    NanotraceHR::Tracer tracer{"property editor view demote custom manager role", category()};

    m_manageNotifications = ManageCustomNotifications::No;
}

void PropertyEditorView::setExtraPropertyViewsCallbacks(const ExtraPropertyViewsCallbacks &callbacks)
{
    m_extraPropertyViewsCallbacks = callbacks;
}

bool PropertyEditorView::locked() const
{
    NanotraceHR::Tracer tracer{"property editor view locked", category()};

    return m_locked;
}

void PropertyEditorView::currentTimelineChanged(const ModelNode &)
{
    NanotraceHR::Tracer tracer{"property editor view current timeline changed", category()};

    m_qmlBackEndForCurrentType->contextObject()->setHasActiveTimeline(
        QmlTimeline::hasActiveTimeline(this));
}

void PropertyEditorView::refreshMetaInfos(const TypeIds &deletedTypeIds)
{
    NanotraceHR::Tracer tracer{"property editor view refresh meta infos", category()};

    m_propertyComponentGenerator.refreshMetaInfos(deletedTypeIds);
}

DynamicPropertiesModel *PropertyEditorView::dynamicPropertiesModel() const
{
    NanotraceHR::Tracer tracer{"property editor view dynamic properties model", category()};

    return m_dynamicPropertiesModel.get();
}

void PropertyEditorView::setUnifiedAction(QAction *unifiedAction)
{
    NanotraceHR::Tracer tracer{"property editor set unified action", category()};

    m_unifiedAction = unifiedAction;
    action()->setVisible(m_unifiedAction.isNull());
}

QAction *PropertyEditorView::unifiedAction() const
{
    return m_unifiedAction.get();
}

void PropertyEditorView::registerWidgetInfo()
{
    NanotraceHR::Tracer tracer{"property editor register widget info", category()};

    AbstractView::registerWidgetInfo();
    m_extraPropertyViewsCallbacks.registerEditor(this);
}

void PropertyEditorView::deregisterWidgetInfo()
{
    NanotraceHR::Tracer tracer{"property editor deregister widget info", category()};

    AbstractView::deregisterWidgetInfo();
    m_extraPropertyViewsCallbacks.unregisterEditor(this);
}

void PropertyEditorView::showExtraWidget()
{
    if (auto wr = widgetRegistration())
        wr->showExtraWidget(widgetInfo());
}

void PropertyEditorView::closeExtraWidget()
{
    if (auto wr = widgetRegistration())
        wr->hideExtraWidget(widgetInfo());
}

void PropertyEditorView::setExpressionOnObjectNode(const QmlObjectNode &constObjectNode,
                                                   PropertyNameView name,
                                                   const QString &newExpression)
{
    NanotraceHR::Tracer tracer{"property editor view set expression on object node", category()};

    auto qmlObjectNode = constObjectNode;
    auto expression = newExpression;
    if (auto property = qmlObjectNode.modelNode().metaInfo().property(name)) {
        const auto &propertType = property.propertyType();
        if (propertType.isColor()) {
            if (QColor(expression.remove('"')).isValid()) {
                qmlObjectNode.setVariantProperty(name, QColor(expression.remove('"')));
                return;
            }
        } else if (propertType.isBool()) {
            if (isTrueFalseLiteral(expression)) {
                if (expression.compare("true", Qt::CaseInsensitive) == 0)
                    qmlObjectNode.setVariantProperty(name, true);
                else
                    qmlObjectNode.setVariantProperty(name, false);
                return;
            }
        } else if (propertType.isInteger()) {
            bool ok;
            int intValue = expression.toInt(&ok);
            if (ok) {
                qmlObjectNode.setVariantProperty(name, intValue);
                return;
            }
        } else if (propertType.isFloat()) {
            bool ok;
            qreal realValue = expression.toDouble(&ok);
            if (ok) {
                qmlObjectNode.setVariantProperty(name, realValue);
                return;
            }
        } else if (propertType.isVariant()) {
            bool ok;
            qreal realValue = expression.toDouble(&ok);
            if (ok) {
                qmlObjectNode.setVariantProperty(name, realValue);
                return;
            } else if (isTrueFalseLiteral(expression)) {
                if (expression.compare("true", Qt::CaseInsensitive) == 0)
                    qmlObjectNode.setVariantProperty(name, true);
                else
                    qmlObjectNode.setVariantProperty(name, false);
                return;
            }
        }
    }

    if (qmlObjectNode.expression(name) != newExpression
        || !qmlObjectNode.propertyAffectedByCurrentState(name))
        qmlObjectNode.setBindingProperty(name, newExpression);
}

void PropertyEditorView::generateAliasForProperty(const ModelNode &modelNode, const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor view generate alias for property", category()};

    QTC_ASSERT(modelNode.isValid(), return);

    auto view = modelNode.view();

    auto rootNode = view->rootModelNode();

    auto nonConstModelNode = modelNode;
    const QString id = nonConstModelNode.validId();

    QString upperCasePropertyName = name;
    upperCasePropertyName.replace(0, 1, upperCasePropertyName.at(0).toUpper());
    QString aliasName = id + upperCasePropertyName;
    aliasName.replace(".", ""); //remove all dots

    PropertyName propertyName = aliasName.toUtf8();
    if (rootNode.hasProperty(propertyName)) {
        Core::AsynchronousMessageBox::warning(
            tr("Cannot Export Property as Alias"),
            tr("Property %1 does already exist for root component.").arg(aliasName));
        return;
    }
    rootNode.bindingProperty(propertyName).setDynamicTypeNameAndExpression("alias", id + "." + name);
}

void PropertyEditorView::removeAliasForProperty(const ModelNode &modelNode, const QString &propertyName)
{
    NanotraceHR::Tracer tracer{"property editor view remove alias for property", category()};

    QTC_ASSERT(modelNode.isValid(), return);

    auto view = modelNode.view();

    auto rootNode = view->rootModelNode();

    auto nonConstModelNode = modelNode;

    const QString id = nonConstModelNode.validId();

    for (const BindingProperty &property : rootNode.bindingProperties()) {
        if (property.expression() == (id + "." + propertyName)) {
            rootNode.removeProperty(property.name());
            break;
        }
    }
}

PropertyEditorView *PropertyEditorView::instance()
{
    NanotraceHR::Tracer tracer{"property editor view instance", category()};
    return QmlDesignerPlugin::instance()->viewManager().propertyEditorView();
}

NodeMetaInfo PropertyEditorView::findCommonAncestor(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"property editor view find common ancestor", category()};

    if (!node.isValid())
        return node.metaInfo();

    const QList<ModelNode> allNodes = currentNodes();
    if (allNodes.size() > 1) {
        NodeMetaInfo commonClass = node.metaInfo();

        for (const ModelNode &selectedNode : allNodes) {
            const NodeMetaInfo &nodeMetaInfo = selectedNode.metaInfo();
            if (nodeMetaInfo.isValid() && !nodeMetaInfo.isBasedOn(commonClass))
                commonClass = findCommonSuperClass(nodeMetaInfo, commonClass);
        }
        return commonClass;
    }

    return node.metaInfo();
}

AuxiliaryDataKey PropertyEditorView::activeNodeAuxKey() const
{
    return AuxiliaryDataKey{AuxiliaryDataType::Temporary,
                            QLatin1StringView("PropertyEditor_ActiveNode_%1").arg(m_uniqueWidgetId)};
}

void PropertyEditorView::showAsExtraWidget()
{
    NanotraceHR::Tracer tracer{"property editor show as extra widget", category()};

    if (auto wr = widgetRegistration())
        wr->showExtraWidget(widgetInfo());
}

void PropertyEditorView::updateSize()
{
    NanotraceHR::Tracer tracer{"property editor view update size", category()};

    if (!m_qmlBackEndForCurrentType)
        return;
    auto frame = m_qmlBackEndForCurrentType->widget()->findChild<QWidget *>("propertyEditorFrame");
    if (frame)
        frame->resize(m_stackedWidget->size());
}

void PropertyEditorView::resetView()
{
    NanotraceHR::Tracer tracer{"property editor view reset view", category()};

    if (model() == nullptr)
        return;

    setActiveNodeToSelection();

    m_locked = true;

    if (debug)
        qDebug() << "________________ RELOADING PROPERTY EDITOR QML _______________________";

    setupQmlBackend();

    if (m_qmlBackEndForCurrentType) {
        m_qmlBackEndForCurrentType->emitSelectionChanged();

        const auto qmlBackEndObject = m_qmlBackEndForCurrentType->widget()->rootObject();
        if (qmlBackEndObject) {
            const auto metaObject = qmlBackEndObject->metaObject();
            const int methodIndex = metaObject->indexOfMethod("clearSearch()");
            if (methodIndex != -1)
                metaObject->method(methodIndex).invoke(qmlBackEndObject);
        }
    }

    m_locked = false;

    updateSize();
}

void PropertyEditorView::setIsSelectionLocked(bool locked)
{
    NanotraceHR::Tracer tracer{"property editor view set is selection locked", category()};

    if (m_isSelectionLocked != locked) {
        m_isSelectionLocked = locked;
        for (PropertyEditorQmlBackend *qmlBackend : std::as_const(m_qmlBackendHash))
            qmlBackend->contextObject()->setIsSelectionLocked(locked);
    }

    // Show current selection on unlock
    if (!m_locked && !m_isSelectionLocked)
        select();
}

namespace {

PropertyEditorQmlBackend *getQmlBackend(QHash<QString, PropertyEditorQmlBackend *> &qmlBackendHash,
                                        const QUrl &qmlFileUrl,
                                        AsynchronousImageCache &imageCache,
                                        PropertyEditorWidget *stackedWidget,
                                        PropertyEditorView *propertyEditorView)
{
    NanotraceHR::Tracer tracer{"property editor view get Qml Backend", category()};

    auto qmlFileName = qmlFileUrl.toString();
    PropertyEditorQmlBackend *currentQmlBackend = qmlBackendHash.value(qmlFileName);

    if (!currentQmlBackend) {
        currentQmlBackend = new PropertyEditorQmlBackend(propertyEditorView, imageCache);

        stackedWidget->addWidget(currentQmlBackend->widget());
        qmlBackendHash.insert(qmlFileName, currentQmlBackend);

        currentQmlBackend->setupContextProperties();
        currentQmlBackend->setSource(qmlFileUrl);
    }

    return currentQmlBackend;
}

void setupCurrentQmlBackend(PropertyEditorQmlBackend *currentQmlBackend,
                            const ModelNodes &editorNodes,
                            const QUrl &qmlSpecificsFile,
                            const QmlModelState &currentState,
                            PropertyEditorView *propertyEditorView,
                            const QString &specificQmlData)
{
    QString currentStateName = currentState.isBaseState() ? QStringLiteral("invalid state")
                                                          : currentState.name();

    if (specificQmlData.isEmpty())
        currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
    currentQmlBackend->setup(editorNodes, currentStateName, qmlSpecificsFile, propertyEditorView);
    currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
}

void setupInsight(const ModelNode &rootModelNode, PropertyEditorQmlBackend *currentQmlBackend)
{
    if (rootModelNode.hasAuxiliaryData(insightEnabledProperty)) {
        currentQmlBackend->contextObject()->setInsightEnabled(
            rootModelNode.auxiliaryData(insightEnabledProperty)->toBool());
    }

    if (rootModelNode.hasAuxiliaryData(insightCategoriesProperty)) {
        currentQmlBackend->contextObject()->setInsightCategories(
            rootModelNode.auxiliaryData(insightCategoriesProperty)->toStringList());
    }
}

void setupWidget(PropertyEditorQmlBackend *currentQmlBackend,
                 PropertyEditorView *propertyEditorView,
                 QStackedWidget *stackedWidget)
{
    currentQmlBackend->widget()->installEventFilter(propertyEditorView);
    stackedWidget->setCurrentWidget(currentQmlBackend->widget());
    currentQmlBackend->contextObject()->triggerSelectionChanged();
}

auto findPaneAndSpecificsPath(const NodeMetaInfos &prototypes,
                              const SourcePathCacheInterface &pathCache)
{
    Utils::PathString panePath;
    Utils::PathString specificsPath;
    Utils::PathString specificsDynamicPath;

    for (const NodeMetaInfo &prototype : prototypes) {
        auto sourceId = prototype.propertyEditorPathId();
        if (sourceId) {
            auto path = pathCache.sourcePath(sourceId);
            if (path.endsWith("Pane.qml")) {
                panePath = path;
                // Pane should always be encountered last, so we can return
                return std::make_tuple(panePath, specificsPath, specificsDynamicPath);
            } else if (path.endsWith("Specifics.qml")) {
                if (!specificsPath.size())
                    specificsPath = path;
            } else if (path.endsWith("SpecificsDynamic.qml")) {
                if (!specificsDynamicPath.size())
                    specificsDynamicPath = path;
            }
        }
    }

    return std::make_tuple(panePath, specificsPath, specificsDynamicPath);
}

QUrl createPaneUrl(Utils::SmallStringView panePath)
{
    if (panePath.empty())
        return PropertyEditorQmlBackend::emptyPaneUrl();

    return QUrl::fromLocalFile(QString{panePath});
}

} // namespace

void PropertyEditorView::handleToolBarAction(int action)
{
    NanotraceHR::Tracer tracer{"property editor view handle toolbar action", category()};

    switch (action) {
    case PropertyEditorContextObject::SelectionLock: {
        setIsSelectionLocked(true);
        break;
    }
    case PropertyEditorContextObject::SelectionUnlock: {
        setIsSelectionLocked(false);
        break;
    }
    case PropertyEditorContextObject::AddExtraWidget: {
        m_extraPropertyViewsCallbacks.addEditor(widgetInfo().uniqueId);
        break;
    }
    }
}

void PropertyEditorView::setupQmlBackend()
{
    NanotraceHR::Tracer tracer{"property editor view setup Qml Backend", category()};

    const NodeMetaInfo commonAncestor = findCommonAncestor(activeNode());
    auto selfAndPrototypes = commonAncestor.selfAndPrototypes();
    bool isEditableComponent = activeNode().isComponent()
                               && !QmlItemNode(activeNode()).isEffectItem();
    auto [panePath, specificsPath, specificsDynamicPath]
        = findPaneAndSpecificsPath(selfAndPrototypes, model()->pathCache());

    QString specificQmlData;

    if (specificsDynamicPath.size()) {
        Utils::FilePath fp = Utils::FilePath::fromString(QString{specificsDynamicPath});
        specificQmlData = QString::fromUtf8(fp.fileContents().value_or(QByteArray()));
    } else {
        specificQmlData = m_propertyEditorComponentGenerator.create(selfAndPrototypes,
                                                                    isEditableComponent);
    }

    PropertyEditorQmlBackend *currentQmlBackend = getQmlBackend(m_qmlBackendHash,
                                                                createPaneUrl(panePath),
                                                                m_imageCache,
                                                                m_stackedWidget,
                                                                this);
    setupCurrentQmlBackend(currentQmlBackend,
                           currentNodes(),
                           QUrl::fromLocalFile(QString{specificsPath}),
                           currentStateNode(),
                           this,
                           specificQmlData);

    setupWidget(currentQmlBackend, this, m_stackedWidget);

    m_qmlBackEndForCurrentType = currentQmlBackend;

    setupInsight(rootModelNode(), currentQmlBackend);

    m_dynamicPropertiesModel->setSelectedNode(activeNode());
    connect(m_qmlBackEndForCurrentType->contextObject(),
            &PropertyEditorContextObject::toolBarAction,
            this,
            &PropertyEditorView::handleToolBarAction,
            Qt::UniqueConnection);
}

void PropertyEditorView::commitVariantValueToModel(PropertyNameView propertyName, const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor view commit variant value to model", category()};

    m_locked = true;
    try {
        RewriterTransaction transaction = beginRewriterTransaction("PropertyEditorView::commitVariantValueToMode");

        const QList<ModelNode> nodes = currentNodes();
        for (const ModelNode &node : nodes) {
            if (auto qmlObjectNode = QmlObjectNode(node))
                qmlObjectNode.setVariantProperty(propertyName, value);
        }
        transaction.commit();
    }
    catch (const RewritingException &e) {
        e.showException();
    }
    m_locked = false;
}

void PropertyEditorView::commitAuxValueToModel(PropertyNameView propertyName, const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor view commit aux value to model", category()};

    m_locked = true;

    PropertyNameView name = propertyName;
    name.chop(5);

    try {
        const QList<ModelNode> nodes = currentNodes();
        if (value.isValid()) {
            for (const ModelNode &node : nodes)
                node.setAuxiliaryData(AuxiliaryDataType::Document, name, value);
        } else {
            for (const ModelNode &node : nodes)
                node.removeAuxiliaryData(AuxiliaryDataType::Document, name);
        }
    }
    catch (const Exception &e) {
        e.showException();
    }
    m_locked = false;
}

void PropertyEditorView::removePropertyFromModel(PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{"property editor view remove property from model", category()};

    m_locked = true;
    try {
        RewriterTransaction transaction = beginRewriterTransaction("PropertyEditorView::removePropertyFromModel");

        const QList<ModelNode> nodes = currentNodes();
        for (const ModelNode &node : nodes) {
            if (QmlObjectNode::isValidQmlObjectNode(node))
                QmlObjectNode(node).removeProperty(propertyName);
        }

        transaction.commit();
    }
    catch (const RewritingException &e) {
        e.showException();
    }
    m_locked = false;
}

bool PropertyEditorView::noValidSelection() const
{
    NanotraceHR::Tracer tracer{"property editor view no valid selection", category()};

    QTC_ASSERT(m_qmlBackEndForCurrentType, return true);
    return !QmlObjectNode::isValidQmlObjectNode(activeNode());
}

ModelNode PropertyEditorView::activeNode() const
{
    NanotraceHR::Tracer tracer{"property editor view active node", category()};

    return m_activeNode;
}

void PropertyEditorView::setActiveNode(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"property editor view set active node", category()};

    m_activeNode = node;
}

/*!
 * \brief PropertyEditorView::setTargetNode forces the node on the editor and sets the focus
 * on the editor.
 */
void PropertyEditorView::setTargetNode(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"property set target node", category()};

    if (node != activeNode()) {
        setSelectionUnlocked();
        setSelectedModelNode(node);
    }

    m_stackedWidget->setFocus();
}

void PropertyEditorView::setInstancesCount(int n)
{
    NanotraceHR::Tracer tracer{"property editor view set instances count", category()};

    if (m_instancesCount == n)
        return;

    m_instancesCount = n;

    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->contextObject()->setEditorInstancesCount(instancesCount());
}

int PropertyEditorView::instancesCount() const
{
    NanotraceHR::Tracer tracer{"property editor view instances count", category()};

    return m_instancesCount;
}

QList<ModelNode> PropertyEditorView::currentNodes() const
{
    NanotraceHR::Tracer tracer{"property editor view current nodes", category()};

    if (m_isSelectionLocked)
        return {activeNode()};

    return selectedModelNodes();
}

void PropertyEditorView::selectedNodesChanged(const QList<ModelNode> &,
                                          const QList<ModelNode> &)
{
    NanotraceHR::Tracer tracer{"property editor view selected nodes changed", category()};

    if (!m_isSelectionLocked)
        select();

    // Notify model selection changes to backend regardless of being locked
    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->handleModelSelectedNodesChanged(this);
}

bool PropertyEditorView::isNodeOrChildSelected(const ModelNode &node) const
{
    NanotraceHR::Tracer tracer{"property editor view is node or child selected", category()};

    if (activeNode().isValid() && node.isValid()) {
        const ModelNodes &nodeList = node.allSubModelNodesAndThisNode();
        return nodeList.contains(activeNode());
    }
    return false;
}

void PropertyEditorView::setSelectionUnlocked()
{
    NanotraceHR::Tracer tracer{"property editor view set selection unlocked", category()};

    if (m_isSelectionLocked)
        setIsSelectionLocked(false);
}

void PropertyEditorView::setSelectionUnlockedIfNodeRemoved(const ModelNode &removedNode)
{
    NanotraceHR::Tracer tracer{"property editor set selection unlocked if node is removed",
                               category()};

    if (isNodeOrChildSelected(removedNode)) {
        setSelectionUnlocked();
        select();
    }
}

void PropertyEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    NanotraceHR::Tracer tracer{"property editor view node about to be removed", category()};

    setSelectionUnlockedIfNodeRemoved(removedNode);

    const ModelNodes &allRemovedNodes = removedNode.allSubModelNodesAndThisNode();

    using SL = ModelTracing::SourceLocation;

    NodeMetaInfo qtQuick3DMaterialMetaInfo = model()->qtQuick3DMaterialMetaInfo();
    NodeMetaInfo qtQuick3DTextureMetaInfo = model()->qtQuick3DTextureMetaInfo();

    for (const ModelNode &selectedNode : allRemovedNodes) {
        const NodeMetaInfo &nodeMetaInfo = selectedNode.metaInfo();
        if (nodeMetaInfo.isBasedOn(qtQuick3DMaterialMetaInfo,
                                   qtQuick3DTextureMetaInfo)) {
            m_texOrMatAboutToBeRemoved = true;
            break;
        }
    }

    if (m_qmlBackEndForCurrentType) {
        if (Utils::contains(allRemovedNodes,
                            QLatin1String{Constants::MATERIAL_LIB_ID},
                            bind_back(&ModelNode::id, SL{})))
            m_qmlBackEndForCurrentType->contextObject()->setHasMaterialLibrary(false);
    }
}

void PropertyEditorView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags)
{
    NanotraceHR::Tracer tracer{"property editor view node removed", category()};

    if (m_qmlBackEndForCurrentType && m_texOrMatAboutToBeRemoved)
        m_qmlBackEndForCurrentType->refreshBackendModel();

    m_texOrMatAboutToBeRemoved = false;
}

void PropertyEditorView::modelAttached(Model *model)
{
    NanotraceHR::Tracer tracer{"property editor view model attached", category()};

    AbstractView::modelAttached(model);

    m_propertyComponentGenerator.setModel(model);

    if (debug)
        qDebug() << Q_FUNC_INFO;

    loadLockedNode();
    resetView();

    showAsExtraWidget();
}

static PropertyEditorValue *variantToPropertyEditorValue(const QVariant &value)
{
    if (auto object = get_if<QObject *>(&value))
        return qobject_cast<PropertyEditorValue *>(*object);

    return nullptr;
}

void PropertyEditorView::modelAboutToBeDetached(Model *model)
{
    NanotraceHR::Tracer tracer{"property editor view model about to be detached", category()};

    saveLockedNode();
    AbstractView::modelAboutToBeDetached(model);
    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->propertyEditorTransaction()->end();

    resetView();
    m_dynamicPropertiesModel->reset();

    for (PropertyEditorQmlBackend *qmlBackend : std::as_const(m_qmlBackendHash)) {
        const QStringList propNames = qmlBackend->backendValuesPropertyMap().keys();
        for (const QString &propName : propNames) {
            if (PropertyEditorValue *valueObject = variantToPropertyEditorValue(
                    qmlBackend->backendValuesPropertyMap().value(propName))) {
                valueObject->resetMetaInfo();
            }
        }
    }
    setActiveNode({});
}

void PropertyEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    NanotraceHR::Tracer tracer{"property editor view properties removed", category()};

    if (noValidSelection())
        return;

    QTC_ASSERT(m_qmlBackEndForCurrentType, return );

    bool changed = false;
    for (const AbstractProperty &property : propertyList) {
        m_qmlBackEndForCurrentType->handlePropertiesRemovedInModelNodeProxy(property);

        ModelNode node(property.parentModelNode());

        if (node.isRootNode() && !activeNode().isRootNode())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(activeNode()).isAliasExported());

        if (node == activeNode()
            || QmlObjectNode(activeNode()).propertyChangeForCurrentState() == node) {
            m_locked = true;
            changed = true;

            const PropertyName propertyName = property.name().toByteArray();
            PropertyName convertedpropertyName = propertyName;

            convertedpropertyName.replace('.', '_');

            PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(
                QString::fromUtf8(convertedpropertyName));

            if (value) {
                value->resetValue();
                m_qmlBackEndForCurrentType
                    ->setValue(activeNode(),
                               propertyName,
                               QmlObjectNode(activeNode()).instanceValue(propertyName));
            }
            m_locked = false;

            if (propertyIsAttachedLayoutProperty(propertyName)) {
                m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(activeNode(),
                                                                                propertyName);

                if (propertyName == "Layout.margins") {
                    m_qmlBackEndForCurrentType
                        ->setValueforLayoutAttachedProperties(activeNode(), "Layout.topMargin");
                    m_qmlBackEndForCurrentType
                        ->setValueforLayoutAttachedProperties(activeNode(), "Layout.bottomMargin");
                    m_qmlBackEndForCurrentType
                        ->setValueforLayoutAttachedProperties(activeNode(), "Layout.leftMargin");
                    m_qmlBackEndForCurrentType
                        ->setValueforLayoutAttachedProperties(activeNode(), "Layout.rightMargin");
                }
            }

            if (propertyIsAttachedInsightProperty(propertyName)) {
                m_qmlBackEndForCurrentType->setValueforInsightAttachedProperties(activeNode(),
                                                                                 propertyName);
            }

            if ("width" == propertyName || "height" == propertyName) {
                const QmlItemNode qmlItemNode = activeNode();
                if (qmlItemNode.isInLayout())
                    resetPuppet();
            }

            if (propertyName.contains("anchor"))
                m_qmlBackEndForCurrentType->backendAnchorBinding().invalidate(activeNode());

            dynamicPropertiesModel()->dispatchPropertyChanges(property);
        }
    }
    if (changed)
        m_qmlBackEndForCurrentType->updateInstanceImage();
}

void PropertyEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    NanotraceHR::Tracer tracer{"property editor view properties about to be removed", category()};

    for (const auto &property : propertyList)
        m_dynamicPropertiesModel->removeItem(property);
}

void PropertyEditorView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"property editor view variant properties changed", category()};

    if (noValidSelection())
        return;

    QTC_ASSERT(m_qmlBackEndForCurrentType, return );

    bool changed = false;

    bool selectedNodeIsMaterial = activeNode().metaInfo().isQtQuick3DMaterial();
    bool selectedNodeHasBindingProperties = !activeNode().bindingProperties().isEmpty();

    for (const VariantProperty &property : propertyList) {
        m_qmlBackEndForCurrentType->handleVariantPropertyChangedInModelNodeProxy(property);

        ModelNode node(property.parentModelNode());

        if (propertyIsAttachedLayoutProperty(property.name()))
            m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(activeNode(),
                                                                            property.name());

        if (propertyIsAttachedInsightProperty(property.name()))
            m_qmlBackEndForCurrentType->setValueforInsightAttachedProperties(activeNode(),
                                                                             property.name());

        if (node == activeNode()
            || QmlObjectNode(activeNode()).propertyChangeForCurrentState() == node) {
            if (property.isDynamic())
                m_dynamicPropertiesModel->updateItem(property);
            if ( QmlObjectNode(activeNode()).modelNode().property(property.name()).isBindingProperty())
                setValue(activeNode(), property.name(), QmlObjectNode(activeNode()).instanceValue(property.name()));
            else
                setValue(activeNode(), property.name(), QmlObjectNode(activeNode()).modelValue(property.name()));
            changed = true;
        }

        if (!changed) {
            // Check if property changes affects the selected node preview

            if (selectedNodeIsMaterial && selectedNodeHasBindingProperties
                && node.metaInfo().isQtQuick3DTexture()) {
                changed = true;
            }
        }
        m_dynamicPropertiesModel->dispatchPropertyChanges(property);
    }

    if (changed)
        m_qmlBackEndForCurrentType->updateInstanceImage();
}

void PropertyEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                                  PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"property editor view binding properties changed", category()};

    if (noValidSelection())
        return;

    QTC_ASSERT(m_qmlBackEndForCurrentType, return);

    if (locked()) {
        for (const BindingProperty &property : propertyList)
            m_qmlBackEndForCurrentType->handleBindingPropertyInModelNodeProxyAboutToChange(property);
        return;
    }

    bool changed = false;
    for (const BindingProperty &property : propertyList) {
        m_qmlBackEndForCurrentType->handleBindingPropertyChangedInModelNodeProxy(property);

        ModelNode node(property.parentModelNode());

        if (property.isAliasExport())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(activeNode()).isAliasExported());

        if (node == activeNode()
            || QmlObjectNode(activeNode()).propertyChangeForCurrentState() == node) {
            if (property.isDynamic())
                m_dynamicPropertiesModel->updateItem(property);
            if (property.name().contains("anchor"))
                m_qmlBackEndForCurrentType->backendAnchorBinding().invalidate(activeNode());

            m_locked = true;
            QString exp = QmlObjectNode(activeNode()).bindingProperty(property.name()).expression();
            m_qmlBackEndForCurrentType->setExpression(property.name(), exp);
            m_locked = false;
            changed = true;
        }
        m_dynamicPropertiesModel->dispatchPropertyChanges(property);
    }

    if (changed)
        m_qmlBackEndForCurrentType->updateInstanceImage();
}

void PropertyEditorView::auxiliaryDataChanged(const ModelNode &node,
                                              AuxiliaryDataKeyView key,
                                              const QVariant &data)
{
    NanotraceHR::Tracer tracer{"property editor view auxiliary data changed", category()};

    if (noValidSelection())
        return;

    bool saved = false;

    QScopeGuard rootGuard([this, node, key, &saved] {
        if (node.isRootNode()) {
            if (!saved)
                m_qmlBackEndForCurrentType->setValueforAuxiliaryProperties(activeNode(), key);
            m_qmlBackEndForCurrentType->handleAuxiliaryDataChanges(node, key);
        }
    });

    if (!node.isSelected())
        return;

    m_qmlBackEndForCurrentType->setValueforAuxiliaryProperties(activeNode(), key);
    saved = true;

    if (key == insightEnabledProperty)
        m_qmlBackEndForCurrentType->contextObject()->setInsightEnabled(data.toBool());

    if (key == insightCategoriesProperty)
        m_qmlBackEndForCurrentType->contextObject()->setInsightCategories(data.toStringList());

    if (key == active3dSceneProperty) {
        bool hasScene3D = data.toInt() != -1;
        m_qmlBackEndForCurrentType->contextObject()->setHas3DScene(hasScene3D);
    }
}

void PropertyEditorView::signalDeclarationPropertiesChanged(
    const QVector<SignalDeclarationProperty> &propertyList, PropertyChangeFlags /* propertyChange */)
{
    NanotraceHR::Tracer tracer{"property editor view signal declaration properties changed",
                               category()};

    for (const SignalDeclarationProperty &property : propertyList)
        m_dynamicPropertiesModel->updateItem(property);
}

void PropertyEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash)
{
    NanotraceHR::Tracer tracer{"property editor view instance informations changed", category()};

    if (noValidSelection())
        return;

    m_locked = true;
    QList<InformationName> informationNameList = informationChangedHash.values(activeNode());
    if (informationNameList.contains(Anchor)
            || informationNameList.contains(HasAnchor))
        m_qmlBackEndForCurrentType->backendAnchorBinding().setup(QmlItemNode(activeNode()));
    m_locked = false;
}

void PropertyEditorView::nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId)
{
    NanotraceHR::Tracer tracer{"property editor view node id changed", category()};

    if (noValidSelection())
        return;

    if (!QmlObjectNode(activeNode()).isValid())
        return;

    m_dynamicPropertiesModel->reset();

    if (m_qmlBackEndForCurrentType) {
        if (newId == Constants::MATERIAL_LIB_ID)
            m_qmlBackEndForCurrentType->contextObject()->setHasMaterialLibrary(true);
        else if (oldId == Constants::MATERIAL_LIB_ID)
            m_qmlBackEndForCurrentType->contextObject()->setHasMaterialLibrary(false);

        if (node == activeNode())
            setValue(node, "id", newId);
        if (node.metaInfo().isBasedOn(model()->qtQuick3DTextureMetaInfo(),
                                      model()->qtQuick3DMaterialMetaInfo())) {
            m_qmlBackEndForCurrentType->refreshBackendModel();
        }
    }
}

void PropertyEditorView::select()
{
    NanotraceHR::Tracer tracer{"property editor view select", category()};

    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->emitSelectionToBeChanged();

    resetView();
}

void PropertyEditorView::loadLockedNode()
{
    NanotraceHR::Tracer tracer{"property editor load locked node", category()};

    ModelNode rootNode = rootModelNode();
    ModelNode loadedNode;
    AuxiliaryDataKey key = activeNodeAuxKey();

    if (auto data = rootNode.auxiliaryData(key); data.has_value()) {
        loadedNode = data->value<ModelNode>();
        rootNode.removeAuxiliaryData(key);
    }

    setActiveNode(loadedNode);
    setIsSelectionLocked(loadedNode.isValid());
}

void PropertyEditorView::saveLockedNode()
{
    NanotraceHR::Tracer tracer{"property editor save locked node", category()};

    ModelNode rootNode = rootModelNode();
    if (!rootNode)
        return;

    ModelNode lockedNode = m_isSelectionLocked ? activeNode() : ModelNode{};
    if (lockedNode)
        rootNode.setAuxiliaryData(activeNodeAuxKey(), QVariant::fromValue(lockedNode));
}

void PropertyEditorView::setActiveNodeToSelection()
{
    NanotraceHR::Tracer tracer{"property editor view set active node to selection", category()};

    const auto selectedNodeList = currentNodes();

    setActiveNode(ModelNode());

    if (selectedNodeList.isEmpty())
        return;

    const ModelNode node = selectedNodeList.constFirst();

    if (QmlObjectNode(node).isValid())
        setActiveNode(node);
}

bool PropertyEditorView::hasWidget() const
{
    NanotraceHR::Tracer tracer{"property editor view has widget", category()};

    return true;
}

void PropertyEditorView::setWidgetInfo(WidgetInfo info)
{
    auto mainPropertyEditor = QmlDesignerPlugin::instance()->viewManager().propertyEditorView();
    if (this == mainPropertyEditor)
        return;

    m_parentWidgetId = info.parentId;
    m_widgetTabName = info.tabName;

    if (m_uniqueWidgetId == mainPropertyEditor->m_uniqueWidgetId) {
        static int counter = 0;
        m_uniqueWidgetId = QString("Properties_%1").arg(++counter);
    }
}

WidgetInfo PropertyEditorView::widgetInfo()
{
    NanotraceHR::Tracer tracer{"property editor view widget info", category()};

    return createWidgetInfo(m_stackedWidget,
                            m_uniqueWidgetId,
                            WidgetInfo::RightPane,
                            m_widgetTabName,
                            tr("Property Editor view"),
                            DesignerWidgetFlags::DisableOnError,
                            m_parentWidgetId);
}

void PropertyEditorView::currentStateChanged(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"property editor view current state changed", category()};

    QmlModelState newQmlModelState(node);
    Q_ASSERT(newQmlModelState.isValid());
    if (debug)
        qDebug() << Q_FUNC_INFO << newQmlModelState.name();
    resetView();
}

void PropertyEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    NanotraceHR::Tracer tracer{"property editor view instance property changed", category()};

    if (!activeNode().isValid())
        return;

    QTC_ASSERT(m_qmlBackEndForCurrentType, return );

    m_locked = true;
    bool changed = false;
    using ModelNodePropertyPair = QPair<ModelNode, PropertyName>;
    for (const ModelNodePropertyPair &propertyPair : propertyList) {
        const ModelNode modelNode = propertyPair.first;
        const QmlObjectNode qmlObjectNode(modelNode);
        const PropertyName propertyName = propertyPair.second;

        m_qmlBackEndForCurrentType->handleInstancePropertyChangedInModelNodeProxy(modelNode,
                                                                                  propertyName);

        if (qmlObjectNode.isValid() && modelNode == activeNode()
            && qmlObjectNode.currentState().isValid()) {
            const AbstractProperty property = modelNode.property(propertyName);
            if (!modelNode.hasProperty(propertyName) || property.isBindingProperty())
                setValue(modelNode, property.name(), qmlObjectNode.instanceValue(property.name()));
            else
                setValue(modelNode, property.name(), qmlObjectNode.modelValue(property.name()));
            changed = true;
        }

        m_dynamicPropertiesModel->handleInstancePropertyChanged(modelNode, propertyName);
    }

    if (changed)
        m_qmlBackEndForCurrentType->updateInstanceImage();

    m_locked = false;
}

void PropertyEditorView::rootNodeTypeChanged(const QString & /*type*/)
{
    NanotraceHR::Tracer tracer{"property editor view root node type changed", category()};

    resetView();
}

void PropertyEditorView::nodeTypeChanged(const ModelNode &node, const TypeName &)
{
    NanotraceHR::Tracer tracer{"property editor view node type changed", category()};

    if (node == activeNode())
        resetView();
}

void PropertyEditorView::nodeReparented(const ModelNode &node,
                                        const NodeAbstractProperty & /*newPropertyParent*/,
                                        const NodeAbstractProperty & /*oldPropertyParent*/,
                                        AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    NanotraceHR::Tracer tracer{"property editor view node reparented", category()};

    if (!m_qmlBackEndForCurrentType)
        return;

    if (node == activeNode())
        m_qmlBackEndForCurrentType->backendAnchorBinding().setup(QmlItemNode(activeNode()));

    using SL = const ModelTracing::SourceLocation;
    const ModelNodes &allNodes = node.allSubModelNodesAndThisNode();

    NodeMetaInfo qtQuick3DMaterialMetaInfo = model()->qtQuick3DMaterialMetaInfo();
    NodeMetaInfo qtQuick3DTextureMetaInfo = model()->qtQuick3DTextureMetaInfo();

    for (const ModelNode &selectedNode : allNodes) {
        const NodeMetaInfo &nodeMetaInfo = selectedNode.metaInfo();
        if (nodeMetaInfo.isBasedOn(qtQuick3DMaterialMetaInfo,
                                   qtQuick3DTextureMetaInfo)) {
            m_qmlBackEndForCurrentType->refreshBackendModel();
            break;
        }
    }

    if (Utils::contains(allNodes,
                        QLatin1String{Constants::MATERIAL_LIB_ID},
                        bind_back(&ModelNode::id, SL{})))
        m_qmlBackEndForCurrentType->contextObject()->setHasMaterialLibrary(true);
}

void PropertyEditorView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    NanotraceHR::Tracer tracer{"property editor view imports changed", category()};

    if (!m_qmlBackEndForCurrentType)
        return;

    if (Utils::contains(removedImports, quick3dImport, &Import::url))
        m_qmlBackEndForCurrentType->contextObject()->setHasQuick3DImport(false);
    else if (Utils::contains(addedImports, quick3dImport, &Import::url))
        m_qmlBackEndForCurrentType->contextObject()->setHasQuick3DImport(true);
}

void PropertyEditorView::customNotification(const AbstractView *,
                                            const QString &identifier,
                                            const QList<ModelNode> &nodeList,
                                            const QList<QVariant> &)
{
    NanotraceHR::Tracer tracer{"property editor view custom notification", category()};

    if (m_manageNotifications == ManageCustomNotifications::No)
        return;

    if (identifier == "set_property_editor_target_node") {
        if (nodeList.isEmpty())
            return;

        m_extraPropertyViewsCallbacks.setTargetNode(nodeList.first());
    }
}

void PropertyEditorView::modelNodePreviewPixmapChanged(const ModelNode &node,
                                                       const QPixmap &pixmap,
                                                       const QByteArray &requestId)
{
    NanotraceHR::Tracer tracer{"property editor view model node preview pixmap changed", category()};

    if (node != activeNode())
        return;

    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->handleModelNodePreviewPixmapChanged(node, pixmap, requestId);
}

void PropertyEditorView::highlightTextureProperties(bool highlight)
{
    NanotraceHR::Tracer tracer{"property editor view highlight texture properties", category()};

    NodeMetaInfo metaInfo = activeNode().metaInfo();
    QTC_ASSERT(metaInfo.isValid(), return);

    DesignerPropertyMap &propMap = m_qmlBackEndForCurrentType->backendValuesPropertyMap();
    const QStringList propNames = propMap.keys();
    for (const QString &propName : propNames) {
        if (metaInfo.property(propName.toUtf8()).propertyType().isQtQuick3DTexture()) {
            QObject *propEditorValObj = propMap.value(propName).value<QObject *>();
            PropertyEditorValue *propEditorVal = qobject_cast<PropertyEditorValue *>(propEditorValObj);
            propEditorVal->setHasActiveDrag(highlight);
        }
    }
}

void PropertyEditorView::dragStarted(QMimeData *mimeData)
{
    NanotraceHR::Tracer tracer{"property editor view drag started", category()};

    if (mimeData->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        const QString assetPath = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS))
                                      .split(',')[0];
        const QString suffix = "*." + assetPath.split('.').last().toLower();

        m_qmlBackEndForCurrentType->contextObject()->setActiveDragSuffix(suffix);

        Asset asset(assetPath);
        if (!asset.isValidTextureSource())
            return;

        highlightTextureProperties();
    } else if (mimeData->hasFormat(Constants::MIME_TYPE_TEXTURE)
               || mimeData->hasFormat(Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        highlightTextureProperties();
    }
}

void PropertyEditorView::dragEnded()
{
    NanotraceHR::Tracer tracer{"property editor view drag ended", category()};

    m_qmlBackEndForCurrentType->contextObject()->setActiveDragSuffix("");
    highlightTextureProperties(false);
}

void PropertyEditorView::setValue(const QmlObjectNode &qmlObjectNode,
                                  PropertyNameView name,
                                  const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor view set value", category()};

    m_locked = true;
    m_qmlBackEndForCurrentType->setValue(qmlObjectNode, name, value);
    m_locked = false;
}

bool PropertyEditorView::eventFilter(QObject *obj, QEvent *event)
{
    NanotraceHR::Tracer tracer{"property editor view event filter", category()};

    if (event->type() == QEvent::FocusOut) {
        if (m_qmlBackEndForCurrentType && m_qmlBackEndForCurrentType->widget() == obj)
            QMetaObject::invokeMethod(m_qmlBackEndForCurrentType->widget()->rootObject(), "closeContextMenu");
    }
    return AbstractView::eventFilter(obj, event);
}

void PropertyEditorView::reloadQml()
{
    NanotraceHR::Tracer tracer{"property editor view reload Qml", category()};

    m_qmlBackendHash.clear();
    while (QWidget *widget = m_stackedWidget->widget(0)) {
        m_stackedWidget->removeWidget(widget);
        delete widget;
    }
    m_qmlBackEndForCurrentType = nullptr;

    resetView();
}

} // namespace QmlDesigner
