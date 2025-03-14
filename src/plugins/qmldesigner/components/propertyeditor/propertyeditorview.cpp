// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorview.h"

#include "propertyeditorqmlbackend.h"
#include "propertyeditortransaction.h"
#include "propertyeditorvalue.h"
#include "propertyeditorwidget.h"

#include <asset.h>
#include <auxiliarydataproperties.h>
#include <dynamicpropertiesmodel.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include "qmldesignerplugin.h"
#include <qmltimeline.h>

#include <rewritingexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <sourcepathstorage/sourcepathcache.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

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
    auto commonBase = first.commonBase(second);

    return commonBase.isValid() ? commonBase : first;
}

PropertyEditorView::PropertyEditorView(AsynchronousImageCache &imageCache,
                                       ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
    , m_updateShortcut(nullptr)
    , m_stackedWidget(new PropertyEditorWidget())
    , m_qmlBackEndForCurrentType(nullptr)
    , m_propertyComponentGenerator{PropertyEditorQmlBackend::propertyEditorResourcesPath(), model()}
    , m_locked(false)
    , m_dynamicPropertiesModel(new DynamicPropertiesModel(true, this))
{
    m_qmlDir = PropertyEditorQmlBackend::propertyEditorResourcesPath();

    if (Utils::HostOsInfo::isMacHost())
        m_updateShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F3), m_stackedWidget);
    else
        m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F3), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &PropertyEditorView::reloadQml);

    m_stackedWidget->setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));
    m_stackedWidget->setMinimumSize(340, 340);
    m_stackedWidget->move(0, 0);
    connect(m_stackedWidget, &PropertyEditorWidget::resized, this, &PropertyEditorView::updateSize);

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));

    m_stackedWidget->setWindowTitle(tr("Properties"));
}

PropertyEditorView::~PropertyEditorView()
{
    qDeleteAll(m_qmlBackendHash);
}

void PropertyEditorView::changeValue(const QString &name)
{
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
            objectNode.setNameAndId(newObjectName, QString::fromLatin1(activeNode().type()));
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
    if (name.isNull())
        return;

    if (locked())
        return;

    if (noValidSelection())
        return;

    executeInTransaction("PropertyEditorView::exportPropertyAsAlias",
                         [&]() { removeAliasForProperty(activeNode(), name); });
}

bool PropertyEditorView::locked() const
{
    return m_locked;
}

void PropertyEditorView::currentTimelineChanged(const ModelNode &)
{
    m_qmlBackEndForCurrentType->contextObject()->setHasActiveTimeline(QmlTimeline::hasActiveTimeline(this));
}

void PropertyEditorView::refreshMetaInfos(const TypeIds &deletedTypeIds)
{
    m_propertyComponentGenerator.refreshMetaInfos(deletedTypeIds);
}

DynamicPropertiesModel *PropertyEditorView::dynamicPropertiesModel() const
{
    return m_dynamicPropertiesModel;
}

void PropertyEditorView::setExpressionOnObjectNode(const QmlObjectNode &constObjectNode,
                                                   PropertyNameView name,
                                                   const QString &newExpression)
{
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
    QTC_ASSERT(modelNode.isValid(), return );

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
    QTC_ASSERT(modelNode.isValid(), return );

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
    static PropertyEditorView *s_instance = nullptr;

    if (s_instance)
        return s_instance;

    const QList<AbstractView *> views = QmlDesignerPlugin::instance()->viewManager().views();
    for (AbstractView *view : views) {
        PropertyEditorView *propView = qobject_cast<PropertyEditorView *>(view);
        if (propView)
            s_instance = propView;
    }

    QTC_ASSERT(s_instance, return nullptr);
    return s_instance;
}

NodeMetaInfo PropertyEditorView::findCommonAncestor(const ModelNode &node)
{
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

void PropertyEditorView::updateSize()
{
    if (!m_qmlBackEndForCurrentType)
        return;
    auto frame = m_qmlBackEndForCurrentType->widget()->findChild<QWidget *>("propertyEditorFrame");
    if (frame)
        frame->resize(m_stackedWidget->size());
}

void PropertyEditorView::resetView()
{
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
    m_isSelectionLocked = locked;
    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->contextObject()->setIsSelectionLocked(locked);

    // Show current selection on unlock
    if (!m_locked && !m_isSelectionLocked)
        select();
}

namespace {

#ifndef QDS_USE_PROJECTSTORAGE
[[maybe_unused]] std::tuple<NodeMetaInfo, QUrl> diffType(const NodeMetaInfo &commonAncestor,
                                                         const NodeMetaInfo &specificsClassMetaInfo)
{
    NodeMetaInfo diffClassMetaInfo;
    QUrl qmlSpecificsFile;

    if (commonAncestor.isValid()) {
        diffClassMetaInfo = commonAncestor;
        const NodeMetaInfos hierarchy = commonAncestor.selfAndPrototypes();
        for (const NodeMetaInfo &metaInfo : hierarchy) {
            if (PropertyEditorQmlBackend::checkIfUrlExists(qmlSpecificsFile))
                break;
            qmlSpecificsFile = PropertyEditorQmlBackend::getQmlFileUrl(metaInfo.typeName() + "Specifics",
                                                                       metaInfo);
            diffClassMetaInfo = metaInfo;
        }
    }

    if (!PropertyEditorQmlBackend::checkIfUrlExists(qmlSpecificsFile))
        diffClassMetaInfo = specificsClassMetaInfo;

    return {diffClassMetaInfo, qmlSpecificsFile};
}

[[maybe_unused]] QString getSpecificQmlData(const NodeMetaInfo &commonAncestor,
                                            const ModelNode &selectedNode,
                                            const NodeMetaInfo &diffClassMetaInfo)
{
    if (commonAncestor.isValid() && diffClassMetaInfo != selectedNode.metaInfo())
        return PropertyEditorQmlBackend::templateGeneration(commonAncestor,
                                                            diffClassMetaInfo,
                                                            selectedNode);

    return {};
}
#endif // QDS_USE_PROJECTSTORAGE

PropertyEditorQmlBackend *getQmlBackend(QHash<QString, PropertyEditorQmlBackend *> &qmlBackendHash,
                                        const QUrl &qmlFileUrl,
                                        AsynchronousImageCache &imageCache,
                                        PropertyEditorWidget *stackedWidget,
                                        PropertyEditorView *propertyEditorView)
{
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

[[maybe_unused]] auto findPaneAndSpecificsPath(const NodeMetaInfos &prototypes,
                                               const SourcePathCacheInterface &pathCache)
{
    Utils::PathString panePath;
    Utils::PathString specificsPath;

    for (const NodeMetaInfo &prototype : prototypes) {
        auto sourceId = prototype.propertyEditorPathId();
        if (sourceId) {
            auto path = pathCache.sourcePath(sourceId);
            if (path.endsWith("Pane.qml")) {
                if (!panePath.size())
                    panePath = path;
                if (specificsPath.size())
                    return std::make_tuple(panePath, specificsPath);
            } else if (path.endsWith("Specifics.qml")) {
                if (!specificsPath.size())
                    specificsPath = path;
                if (panePath.size())
                    return std::make_tuple(panePath, specificsPath);
            }
        }
    }

    return std::make_tuple(panePath, specificsPath);
}

[[maybe_unused]] QUrl createPaneUrl(Utils::SmallStringView panePath)
{
    if (panePath.empty())
        return PropertyEditorQmlBackend::emptyPaneUrl();

    return QUrl::fromLocalFile(QString{panePath});
}

} // namespace

void PropertyEditorView::handleToolBarAction(int action)
{
    switch (action) {
        case PropertyEditorContextObject::SelectionLock: {
            setIsSelectionLocked(true);
            break;
        }
        case PropertyEditorContextObject::SelectionUnlock: {
            setIsSelectionLocked(false);
            break;
        }
    }
}

void PropertyEditorView::setupQmlBackend()
{
#ifdef QDS_USE_PROJECTSTORAGE
    const NodeMetaInfo commonAncestor = findCommonAncestor(activeNode());
    auto selfAndPrototypes = commonAncestor.selfAndPrototypes();
    bool isEditableComponent = activeNode().isComponent()
                               && !QmlItemNode(activeNode()).isEffectItem();
    auto specificQmlData = m_propertyEditorComponentGenerator.create(selfAndPrototypes,
                                                                     isEditableComponent);
    auto [panePath, specificsPath] = findPaneAndSpecificsPath(selfAndPrototypes, model()->pathCache());

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
#else
    const NodeMetaInfo commonAncestor = findCommonAncestor(activeNode());

    // qmlFileUrl is panel url. and specifics is its metainfo
    const auto [qmlFileUrl, specificsClassMetaInfo] = PropertyEditorQmlBackend::getQmlUrlForMetaInfo(
        commonAncestor);

    auto [diffClassMetaInfo, qmlSpecificsFile] = diffType(commonAncestor, specificsClassMetaInfo);

    // Hack to fix Textures in property views in case obsolete specifics are loaded from module
    if (qmlFileUrl.toLocalFile().endsWith("TexturePane.qml"))
        qmlSpecificsFile = QUrl{};

    QString specificQmlData = getSpecificQmlData(commonAncestor, activeNode(), diffClassMetaInfo);

    PropertyEditorQmlBackend *currentQmlBackend = getQmlBackend(m_qmlBackendHash,
                                                                qmlFileUrl,
                                                                m_imageCache,
                                                                m_stackedWidget,
                                                                this);

    setupCurrentQmlBackend(currentQmlBackend,
                           currentNodes(),
                           qmlSpecificsFile,
                           currentStateNode(),
                           this,
                           specificQmlData);

    setupWidget(currentQmlBackend, this, m_stackedWidget);

    m_qmlBackEndForCurrentType = currentQmlBackend;

    setupInsight(rootModelNode(), currentQmlBackend);
#endif // QDS_USE_PROJECTSTORAGE

    m_dynamicPropertiesModel->setSelectedNode(activeNode());

    QObject::connect(m_qmlBackEndForCurrentType->contextObject(), SIGNAL(toolBarAction(int)), this, SLOT(handleToolBarAction(int)));
}

void PropertyEditorView::commitVariantValueToModel(PropertyNameView propertyName, const QVariant &value)
{
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
    QTC_ASSERT(m_qmlBackEndForCurrentType, return true);
    return !QmlObjectNode::isValidQmlObjectNode(activeNode());
}

ModelNode PropertyEditorView::activeNode() const
{
    return m_activeNode;
}

void PropertyEditorView::setActiveNode(const ModelNode &node)
{
    m_activeNode = node;
}

QList<ModelNode> PropertyEditorView::currentNodes() const
{
    if (m_isSelectionLocked)
        return {m_activeNode};

    return selectedModelNodes();
}

void PropertyEditorView::selectedNodesChanged(const QList<ModelNode> &,
                                          const QList<ModelNode> &)
{
    if (!m_isSelectionLocked)
        select();

    // Notify model selection changes to backend regardless of being locked
    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->handleModelSelectedNodesChanged(this);
}

bool PropertyEditorView::isNodeOrChildSelected(const ModelNode &node) const
{
    if (activeNode().isValid() && node.isValid()) {
        const ModelNodes &nodeList = node.allSubModelNodesAndThisNode();
        return nodeList.contains(activeNode());
    }
    return false;
}

void PropertyEditorView::resetSelectionLocked()
{
    if (m_isSelectionLocked)
        setIsSelectionLocked(false);
}

void PropertyEditorView::resetIfNodeIsRemoved(const ModelNode &removedNode)
{
    if (isNodeOrChildSelected(removedNode)) {
        resetSelectionLocked();
        select();
    }
}

void PropertyEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    resetIfNodeIsRemoved(removedNode);

    const ModelNodes &allRemovedNodes = removedNode.allSubModelNodesAndThisNode();

    if (Utils::contains(allRemovedNodes, model()->qtQuick3DTextureMetaInfo(), &ModelNode::metaInfo))
        m_textureAboutToBeRemoved = true;

    if (m_qmlBackEndForCurrentType) {
        if (Utils::contains(allRemovedNodes, QLatin1String{Constants::MATERIAL_LIB_ID}, &ModelNode::id))
            m_qmlBackEndForCurrentType->contextObject()->setHasMaterialLibrary(false);
    }
}

void PropertyEditorView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags)
{
    if (m_qmlBackEndForCurrentType && m_textureAboutToBeRemoved)
        m_qmlBackEndForCurrentType->refreshBackendModel();

    m_textureAboutToBeRemoved = false;
}

void PropertyEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if constexpr (useProjectStorage())
        m_propertyComponentGenerator.setModel(model);

    if (debug)
        qDebug() << Q_FUNC_INFO;

    resetView();
}

void PropertyEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    m_qmlBackEndForCurrentType->propertyEditorTransaction()->end();

    resetView();
    m_dynamicPropertiesModel->reset();
}

void PropertyEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    if (noValidSelection())
        return;

    QTC_ASSERT(m_qmlBackEndForCurrentType, return );

    bool changed = false;
    for (const AbstractProperty &property : propertyList) {
        m_qmlBackEndForCurrentType->handlePropertiesRemovedInModelNodeProxy(property);

        ModelNode node(property.parentModelNode());

        if (node.isRootNode() && !activeNode().isRootNode())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(activeNode()).isAliasExported());

        if (node == activeNode() || QmlObjectNode(activeNode()).propertyChangeForCurrentState() == node) {
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
    for (const auto &property : propertyList)
        m_dynamicPropertiesModel->removeItem(property);
}

void PropertyEditorView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
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

        if (node == activeNode() || QmlObjectNode(activeNode()).propertyChangeForCurrentState() == node) {
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

        if (node == activeNode() || QmlObjectNode(activeNode()).propertyChangeForCurrentState() == node) {
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
                                              [[maybe_unused]] AuxiliaryDataKeyView key,
                                              const QVariant &data)
{
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
}

void PropertyEditorView::signalDeclarationPropertiesChanged(
    const QVector<SignalDeclarationProperty> &propertyList, PropertyChangeFlags /* propertyChange */)
{
    for (const SignalDeclarationProperty &property : propertyList)
        m_dynamicPropertiesModel->updateItem(property);
}

void PropertyEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash)
{
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
        if (node.metaInfo().isQtQuick3DTexture())
            m_qmlBackEndForCurrentType->refreshBackendModel();
    }
}

void PropertyEditorView::select()
{
    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->emitSelectionToBeChanged();

    resetView();
}

void PropertyEditorView::setActiveNodeToSelection()
{
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
    return true;
}

WidgetInfo PropertyEditorView::widgetInfo()
{
    return createWidgetInfo(m_stackedWidget,
                            QStringLiteral("Properties"),
                            WidgetInfo::RightPane,
                            tr("Properties"),
                            tr("Property Editor view"));
}

void PropertyEditorView::currentStateChanged(const ModelNode &node)
{
    QmlModelState newQmlModelState(node);
    Q_ASSERT(newQmlModelState.isValid());
    if (debug)
        qDebug() << Q_FUNC_INFO << newQmlModelState.name();
    resetView();
}

void PropertyEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
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

void PropertyEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    resetView();
}

void PropertyEditorView::nodeTypeChanged(const ModelNode &node, const TypeName &, int, int)
{
     if (node == activeNode())
         resetView();
}

void PropertyEditorView::nodeReparented(const ModelNode &node,
                                        const NodeAbstractProperty & /*newPropertyParent*/,
                                        const NodeAbstractProperty & /*oldPropertyParent*/,
                                        AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (node == activeNode())
        m_qmlBackEndForCurrentType->backendAnchorBinding().setup(QmlItemNode(activeNode()));

    const ModelNodes &allNodes = node.allSubModelNodesAndThisNode();
    if (Utils::contains(allNodes, model()->qtQuick3DTextureMetaInfo(), &ModelNode::metaInfo))
        m_qmlBackEndForCurrentType->refreshBackendModel();

    if (m_qmlBackEndForCurrentType) {
        if (Utils::contains(allNodes, QLatin1String{Constants::MATERIAL_LIB_ID}, &ModelNode::id))
            m_qmlBackEndForCurrentType->contextObject()->setHasMaterialLibrary(true);
    }
}

void PropertyEditorView::importsChanged(const Imports &addedImports, const Imports &removedImports)
{
    if (!m_qmlBackEndForCurrentType)
        return;

    if (Utils::contains(removedImports, quick3dImport, &Import::url))
        m_qmlBackEndForCurrentType->contextObject()->setHasQuick3DImport(false);
    else if (Utils::contains(addedImports, quick3dImport, &Import::url))
        m_qmlBackEndForCurrentType->contextObject()->setHasQuick3DImport(true);
}

void PropertyEditorView::modelNodePreviewPixmapChanged(const ModelNode &node,
                                                       const QPixmap &pixmap,
                                                       const QByteArray &requestId)
{
    if (node != activeNode())
        return;

    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->handleModelNodePreviewPixmapChanged(node, pixmap, requestId);
}

void PropertyEditorView::highlightTextureProperties(bool highlight)
{
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
    m_qmlBackEndForCurrentType->contextObject()->setActiveDragSuffix("");
    highlightTextureProperties(false);
}

void PropertyEditorView::setValue(const QmlObjectNode &qmlObjectNode,
                                  PropertyNameView name,
                                  const QVariant &value)
{
    m_locked = true;
    m_qmlBackEndForCurrentType->setValue(qmlObjectNode, name, value);
    m_locked = false;
}

bool PropertyEditorView::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (m_qmlBackEndForCurrentType && m_qmlBackEndForCurrentType->widget() == obj)
            QMetaObject::invokeMethod(m_qmlBackEndForCurrentType->widget()->rootObject(), "closeContextMenu");
    }
    return AbstractView::eventFilter(obj, event);
}

void PropertyEditorView::reloadQml()
{
    m_qmlBackendHash.clear();
    while (QWidget *widget = m_stackedWidget->widget(0)) {
        m_stackedWidget->removeWidget(widget);
        delete widget;
    }
    m_qmlBackEndForCurrentType = nullptr;

    resetView();
}

} // namespace QmlDesigner
