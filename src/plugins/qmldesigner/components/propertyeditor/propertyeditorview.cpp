// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorview.h"

#include "propertyeditorqmlbackend.h"
#include "propertyeditortransaction.h"
#include "propertyeditorvalue.h"
#include "propertyeditorwidget.h"

#include <auxiliarydataproperties.h>
#include <nodemetainfo.h>
#include <qmldesignerconstants.h>
#include <qmltimeline.h>

#include <invalididexception.h>
#include <rewritingexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <projectstorage/sourcepathcache.h>

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
#include <QScopedPointer>
#include <QShortcut>
#include <QTimer>

enum {
    debug = false
};

namespace QmlDesigner {

static bool propertyIsAttachedLayoutProperty(const PropertyName &propertyName)
{
    return propertyName.contains("Layout.");
}

static bool propertyIsAttachedInsightProperty(const PropertyName &propertyName)
{
    return propertyName.contains("InsightCategory.");
}

PropertyEditorView::PropertyEditorView(AsynchronousImageCache &imageCache,
                                       ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
    , m_updateShortcut(nullptr)
    , m_timerId(0)
    , m_stackedWidget(new PropertyEditorWidget())
    , m_qmlBackEndForCurrentType(nullptr)
    , m_propertyComponentGenerator{QmlDesigner::PropertyEditorQmlBackend::propertyEditorResourcesPath(),
                                   model()}
    , m_locked(false)
    , m_setupCompleted(false)
    , m_singleShotTimer(new QTimer(this))
{
    m_qmlDir = PropertyEditorQmlBackend::propertyEditorResourcesPath();

    if (Utils::HostOsInfo::isMacHost())
        m_updateShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F3), m_stackedWidget);
    else
        m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F3), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &PropertyEditorView::reloadQml);

    m_stackedWidget->setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));
    m_stackedWidget->setMinimumWidth(340);
    m_stackedWidget->move(0, 0);
    connect(m_stackedWidget, &PropertyEditorWidget::resized, this, &PropertyEditorView::updateSize);

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));

    m_stackedWidget->setWindowTitle(tr("Properties"));
}

PropertyEditorView::~PropertyEditorView()
{
    qDeleteAll(m_qmlBackendHash);
}

void PropertyEditorView::setupPane(const TypeName &typeName)
{
    NodeMetaInfo metaInfo = model()->metaInfo(typeName);

    QUrl qmlFile = PropertyEditorQmlBackend::getQmlFileUrl("Qt/ItemPane", metaInfo);
    QUrl qmlSpecificsFile;

    qmlSpecificsFile = PropertyEditorQmlBackend::getQmlFileUrl(typeName + "Specifics", metaInfo);

    PropertyEditorQmlBackend *qmlBackend = m_qmlBackendHash.value(qmlFile.toString());

    if (!qmlBackend) {
        qmlBackend = new PropertyEditorQmlBackend(this, m_imageCache);

        qmlBackend->initialSetup(typeName, qmlSpecificsFile, this);
        qmlBackend->setSource(qmlFile);

        m_stackedWidget->addWidget(qmlBackend->widget());
        m_qmlBackendHash.insert(qmlFile.toString(), qmlBackend);
    } else {
        qmlBackend->initialSetup(typeName, qmlSpecificsFile, this);
    }
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

        if (newId == m_selectedNode.id())
            return;

        if (QmlDesigner::ModelNode::isValidId(newId)  && !hasId(newId)) {
            m_selectedNode.setIdWithRefactoring(newId);
        } else {
            m_locked = true;
            value->setValue(m_selectedNode.id());
            m_locked = false;
            QString errMsg = QmlDesigner::ModelNode::getIdValidityErrorMessage(newId);
            if (!errMsg.isEmpty())
                Core::AsynchronousMessageBox::warning(tr("Invalid ID"), errMsg);
            else
                Core::AsynchronousMessageBox::warning(tr("Invalid ID"), tr("%1 already exists.").arg(newId));
        }
        return;
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

    const NodeMetaInfo metaInfo = QmlObjectNode(m_selectedNode).modelNode().metaInfo();

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

    if (castedValue.typeId() == QVariant::Color) {
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
            && (!castedValue.isNull() || castedValue.typeId() == QVariant::Vector2D
                || castedValue.typeId() == QVariant::Vector3D
                || castedValue.typeId() == QVariant::Vector4D)) {
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

        QmlObjectNode qmlObjectNode{m_selectedNode};
        PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(QString::fromLatin1(underscoreName));

        if (!value) {
            qWarning() << "PropertyEditor::changeExpression no value for " << underscoreName;
            return;
        }

        if (auto property = qmlObjectNode.modelNode().metaInfo().property(name)) {
            const auto &propertType = property.propertyType();
            if (propertType.isColor()) {
                if (QColor(value->expression().remove('"')).isValid()) {
                    qmlObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                    return;
                }
            } else if (propertType.isBool()) {
                if (isTrueFalseLiteral(value->expression())) {
                    if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                        qmlObjectNode.setVariantProperty(name, true);
                    else
                        qmlObjectNode.setVariantProperty(name, false);
                    return;
                }
            } else if (propertType.isInteger()) {
                bool ok;
                int intValue = value->expression().toInt(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, intValue);
                    return;
                }
            } else if (propertType.isFloat()) {
                bool ok;
                qreal realValue = value->expression().toDouble(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, realValue);
                    return;
                }
            } else if (propertType.isVariant()) {
                bool ok;
                qreal realValue = value->expression().toDouble(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, realValue);
                    return;
                } else if (isTrueFalseLiteral(value->expression())) {
                    if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                        qmlObjectNode.setVariantProperty(name, true);
                    else
                        qmlObjectNode.setVariantProperty(name, false);
                    return;
                }
            }
        }

        if (value->expression().isEmpty()) {
            value->resetValue();
            return;
        }

        if (qmlObjectNode.expression(name) != value->expression()
            || !qmlObjectNode.propertyAffectedByCurrentState(name))
            qmlObjectNode.setBindingProperty(name, value->expression());
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

    executeInTransaction("PropertyEditorView::exportPropertyAsAlias", [this, name](){
        const QString id = m_selectedNode.validId();
        QString upperCasePropertyName = name;
        upperCasePropertyName.replace(0, 1, upperCasePropertyName.at(0).toUpper());
        QString aliasName = id + upperCasePropertyName;
        aliasName.replace(".", ""); //remove all dots

        PropertyName propertyName = aliasName.toUtf8();
        if (rootModelNode().hasProperty(propertyName)) {
            Core::AsynchronousMessageBox::warning(tr("Cannot Export Property as Alias"),
                                                  tr("Property %1 does already exist for root component.").arg(aliasName));
            return;
        }
        rootModelNode().bindingProperty(propertyName).setDynamicTypeNameAndExpression("alias", id + "." + name);
    });
}

void PropertyEditorView::removeAliasExport(const QString &name)
{
    if (name.isNull())
        return;

    if (locked())
        return;

    if (noValidSelection())
        return;

    executeInTransaction("PropertyEditorView::exportPropertyAsAlias", [this, name](){
        const QString id = m_selectedNode.validId();

        for (const BindingProperty &property : rootModelNode().bindingProperties())
            if (property.expression() == (id + "." + name)) {
                rootModelNode().removeProperty(property.name());
                break;
            }
    });
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

void PropertyEditorView::updateSize()
{
    if (!m_qmlBackEndForCurrentType)
        return;
    auto frame = m_qmlBackEndForCurrentType->widget()->findChild<QWidget*>("propertyEditorFrame");
    if (frame)
        frame->resize(m_stackedWidget->size());
}

void PropertyEditorView::setupPanes()
{
    if (isAttached()) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        setupPane("QtQuick.Item");
        resetView();
        m_setupCompleted = true;
        QApplication::restoreOverrideCursor();
    }
}

void PropertyEditorView::delayedResetView()
{
    if (m_timerId)
        killTimer(m_timerId);
    m_timerId = startTimer(50);
}

void PropertyEditorView::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId())
        resetView();
}

void PropertyEditorView::resetView()
{
    if (model() == nullptr)
        return;

    setSelelectedModelNode();

    m_locked = true;

    if (debug)
        qDebug() << "________________ RELOADING PROPERTY EDITOR QML _______________________";

    if (m_timerId)
        killTimer(m_timerId);

    if (m_selectedNode.isValid() && model() != m_selectedNode.model())
        m_selectedNode = ModelNode();

    setupQmlBackend();

    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->emitSelectionChanged();

    m_locked = false;

    if (m_timerId)
        m_timerId = 0;

    updateSize();
}

namespace {

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

        currentQmlBackend->setSource(qmlFileUrl);
    }

    return currentQmlBackend;
}

void setupCurrentQmlBackend(PropertyEditorQmlBackend *currentQmlBackend,
                            const ModelNode &selectedNode,
                            const QUrl &qmlSpecificsFile,
                            const QmlModelState &currentState,
                            PropertyEditorView *propertyEditorView,
                            const QString &specificQmlData)
{
    QString currentStateName = currentState.isBaseState() ? currentState.name()
                                                          : QStringLiteral("invalid state");

    QmlObjectNode qmlObjectNode{selectedNode};
    if (specificQmlData.isEmpty())
        currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
    currentQmlBackend->setup(qmlObjectNode, currentStateName, qmlSpecificsFile, propertyEditorView);
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
                panePath = path;
                if (panePath.size() && specificsPath.size())
                    return std::make_tuple(panePath, specificsPath);
            } else if (path.endsWith("Specifics.qml")) {
                specificsPath = path;
                if (panePath.size() && specificsPath.size())
                    return std::make_tuple(panePath, specificsPath);
            }
        }
    }

    return std::make_tuple(panePath, specificsPath);
}
} // namespace

void PropertyEditorView::setupQmlBackend()
{
    if constexpr (useProjectStorage()) {
        auto selfAndPrototypes = m_selectedNode.metaInfo().selfAndPrototypes();
        auto specificQmlData = m_propertyEditorComponentGenerator.create(selfAndPrototypes,
                                                                         m_selectedNode.isComponent());
        auto [panePath, specificsPath] = findPaneAndSpecificsPath(selfAndPrototypes,
                                                                  model()->pathCache());
        PropertyEditorQmlBackend *currentQmlBackend = getQmlBackend(m_qmlBackendHash,
                                                                    QUrl::fromLocalFile(
                                                                        QString{panePath}),
                                                                    m_imageCache,
                                                                    m_stackedWidget,
                                                                    this);

        setupCurrentQmlBackend(currentQmlBackend,
                               m_selectedNode,
                               QUrl::fromLocalFile(QString{specificsPath}),
                               currentState(),
                               this,
                               specificQmlData);

        setupWidget(currentQmlBackend, this, m_stackedWidget);

        m_qmlBackEndForCurrentType = currentQmlBackend;

        setupInsight(rootModelNode(), currentQmlBackend);
    } else {
        const NodeMetaInfo commonAncestor = PropertyEditorQmlBackend::findCommonAncestor(
            m_selectedNode);

        const auto [qmlFileUrl, specificsClassMetaInfo] = PropertyEditorQmlBackend::getQmlUrlForMetaInfo(
            commonAncestor);

        auto [diffClassMetaInfo, qmlSpecificsFile] = diffType(commonAncestor, specificsClassMetaInfo);

        QString specificQmlData = getSpecificQmlData(commonAncestor, m_selectedNode, diffClassMetaInfo);

        PropertyEditorQmlBackend *currentQmlBackend = getQmlBackend(m_qmlBackendHash,
                                                                    qmlFileUrl,
                                                                    m_imageCache,
                                                                    m_stackedWidget,
                                                                    this);

        setupCurrentQmlBackend(currentQmlBackend,
                               m_selectedNode,
                               qmlSpecificsFile,
                               currentState(),
                               this,
                               specificQmlData);

        setupWidget(currentQmlBackend, this, m_stackedWidget);

        m_qmlBackEndForCurrentType = currentQmlBackend;

        setupInsight(rootModelNode(), currentQmlBackend);
    }
}

void PropertyEditorView::commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value)
{
    m_locked = true;
    try {
        RewriterTransaction transaction = beginRewriterTransaction("PropertyEditorView::commitVariantValueToMode");

        for (const ModelNode &node : m_selectedNode.view()->selectedModelNodes()) {
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

void PropertyEditorView::commitAuxValueToModel(const PropertyName &propertyName, const QVariant &value)
{
    m_locked = true;

    PropertyName name = propertyName;
    name.chop(5);

    try {
        if (value.isValid()) {
            for (const ModelNode &node : m_selectedNode.view()->selectedModelNodes()) {
                node.setAuxiliaryData(AuxiliaryDataType::Document, name, value);
            }
        } else {
            for (const ModelNode &node : m_selectedNode.view()->selectedModelNodes()) {
                node.removeAuxiliaryData(AuxiliaryDataType::Document, name);
            }
        }
    }
    catch (const Exception &e) {
        e.showException();
    }
    m_locked = false;
}

void PropertyEditorView::removePropertyFromModel(const PropertyName &propertyName)
{
    m_locked = true;
    try {
        RewriterTransaction transaction = beginRewriterTransaction("PropertyEditorView::removePropertyFromModel");

        for (const ModelNode &node : m_selectedNode.view()->selectedModelNodes()) {
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
    return !QmlObjectNode::isValidQmlObjectNode(m_selectedNode);
}

void PropertyEditorView::selectedNodesChanged(const QList<ModelNode> &,
                                          const QList<ModelNode> &)
{
    select();
}

void PropertyEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (m_selectedNode.isValid() && removedNode.isValid() && m_selectedNode == removedNode)
        select();
}

void PropertyEditorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if constexpr (useProjectStorage())
        m_propertyComponentGenerator.setModel(model);

    if (debug)
        qDebug() << Q_FUNC_INFO;

    m_locked = true;

    if (!m_setupCompleted) {
        QTimer::singleShot(50, this, [this] {
            if (isAttached()) {
                PropertyEditorView::setupPanes();
                /* workaround for QTBUG-75847 */
                reloadQml();
            }
        });
    }

    m_locked = false;

    resetView();
}

void PropertyEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    m_qmlBackEndForCurrentType->propertyEditorTransaction()->end();

    resetView();
}

void PropertyEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    if (noValidSelection())
        return;

    for (const AbstractProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (node.isRootNode() && !m_selectedNode.isRootNode())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedNode).isAliasExported());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            m_locked = true;

            PropertyName propertyName = property.name();
            propertyName.replace('.', '_');

            PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(
                QString::fromUtf8(propertyName));

            if (value) {
                value->resetValue();
                m_qmlBackEndForCurrentType
                    ->setValue(m_selectedNode,
                               property.name(),
                               QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            }
            m_locked = false;

            if (propertyIsAttachedLayoutProperty(property.name())) {
                m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, property.name());

                if (property.name() == "Layout.margins") {
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.topMargin");
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.bottomMargin");
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.leftMargin");
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.rightMargin");

                }
            }

            if (propertyIsAttachedInsightProperty(property.name())) {
                m_qmlBackEndForCurrentType->setValueforInsightAttachedProperties(m_selectedNode,
                                                                                 property.name());
            }

            if ("width" == property.name() || "height" == property.name()) {
                const QmlItemNode qmlItemNode = m_selectedNode;
                if (qmlItemNode.isInLayout())
                    resetPuppet();
            }

            if (property.name().contains("anchor"))
                m_qmlBackEndForCurrentType->backendAnchorBinding().invalidate(m_selectedNode);
        }
    }
}

void PropertyEditorView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    if (noValidSelection())
        return;

    for (const VariantProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (propertyIsAttachedLayoutProperty(property.name()))
            m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode,
                                                                            property.name());

        if (propertyIsAttachedInsightProperty(property.name()))
            m_qmlBackEndForCurrentType->setValueforInsightAttachedProperties(m_selectedNode,
                                                                             property.name());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags /*propertyChange*/)
{
    if (locked() || noValidSelection())
        return;

    for (const BindingProperty &property : propertyList) {
        ModelNode node(property.parentModelNode());

        if (property.isAliasExport())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedNode).isAliasExported());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if (property.name().contains("anchor"))
                m_qmlBackEndForCurrentType->backendAnchorBinding().invalidate(m_selectedNode);

            m_locked = true;
            QString exp = QmlObjectNode(m_selectedNode).bindingProperty(property.name()).expression();
            m_qmlBackEndForCurrentType->setExpression(property.name(), exp);
            m_locked = false;
        }
    }
}

void PropertyEditorView::auxiliaryDataChanged(const ModelNode &node,
                                              [[maybe_unused]] AuxiliaryDataKeyView key,
                                              const QVariant &data)
{
    if (noValidSelection())
        return;

    if (!node.isSelected())
        return;

    m_qmlBackEndForCurrentType->setValueforAuxiliaryProperties(m_selectedNode, key);

    if (key == insightEnabledProperty)
        m_qmlBackEndForCurrentType->contextObject()->setInsightEnabled(data.toBool());

    if (key == insightCategoriesProperty)
        m_qmlBackEndForCurrentType->contextObject()->setInsightCategories(data.toStringList());
}

void PropertyEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangedHash)
{
    if (noValidSelection())
        return;

    m_locked = true;
    QList<InformationName> informationNameList = informationChangedHash.values(m_selectedNode);
    if (informationNameList.contains(Anchor)
            || informationNameList.contains(HasAnchor))
        m_qmlBackEndForCurrentType->backendAnchorBinding().setup(QmlItemNode(m_selectedNode));
    m_locked = false;
}

void PropertyEditorView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& /*oldId*/)
{
    if (noValidSelection())
        return;

    if (!QmlObjectNode(m_selectedNode).isValid())
        return;

    if (node == m_selectedNode) {

        if (m_qmlBackEndForCurrentType)
            setValue(node, "id", newId);
    }
}

void PropertyEditorView::select()
{
    if (m_qmlBackEndForCurrentType)
        m_qmlBackEndForCurrentType->emitSelectionToBeChanged();

    delayedResetView();

    auto nodes = selectedModelNodes();

    for (const auto &n : nodes) {
        n.metaInfo().isFileComponent();
    }
}

void PropertyEditorView::setSelelectedModelNode()
{
    const auto selectedNodeList = selectedModelNodes();

    m_selectedNode = ModelNode();

    if (selectedNodeList.isEmpty())
        return;

    const ModelNode node = selectedNodeList.constFirst();

    if (QmlObjectNode(node).isValid())
            m_selectedNode = node;
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
                            0,
                            tr("Properties"),
                            tr("Property Editor view"));
}

void PropertyEditorView::currentStateChanged(const ModelNode &node)
{
    QmlModelState newQmlModelState(node);
    Q_ASSERT(newQmlModelState.isValid());
    if (debug)
        qDebug() << Q_FUNC_INFO << newQmlModelState.name();
    delayedResetView();
}

void PropertyEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (!m_selectedNode.isValid())
        return;
    m_locked = true;

    using ModelNodePropertyPair = QPair<ModelNode, PropertyName>;
    for (const ModelNodePropertyPair &propertyPair : propertyList) {
        const ModelNode modelNode = propertyPair.first;
        const QmlObjectNode qmlObjectNode(modelNode);
        const PropertyName propertyName = propertyPair.second;

        if (qmlObjectNode.isValid() && m_qmlBackEndForCurrentType && modelNode == m_selectedNode && qmlObjectNode.currentState().isValid()) {
            const AbstractProperty property = modelNode.property(propertyName);
            if (modelNode == m_selectedNode || qmlObjectNode.propertyChangeForCurrentState() == qmlObjectNode) {
                if ( !modelNode.hasProperty(propertyName) || modelNode.property(property.name()).isBindingProperty() )
                    setValue(modelNode, property.name(), qmlObjectNode.instanceValue(property.name()));
                else
                    setValue(modelNode, property.name(), qmlObjectNode.modelValue(property.name()));
            }
        }

    }

    m_locked = false;

}

void PropertyEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    delayedResetView();
}

void PropertyEditorView::nodeTypeChanged(const ModelNode &node, const TypeName &, int, int)
{
     if (node == m_selectedNode)
         delayedResetView();
}

void PropertyEditorView::nodeReparented(const ModelNode &node,
                                        const NodeAbstractProperty & /*newPropertyParent*/,
                                        const NodeAbstractProperty & /*oldPropertyParent*/,
                                        AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (node == m_selectedNode)
        m_qmlBackEndForCurrentType->backendAnchorBinding().setup(QmlItemNode(m_selectedNode));
}

void PropertyEditorView::dragStarted(QMimeData *mimeData)
{
    if (!mimeData->hasFormat(Constants::MIME_TYPE_ASSETS))
        return;

    const QString assetPath = QString::fromUtf8(mimeData->data(Constants::MIME_TYPE_ASSETS))
                                  .split(',')[0];
    const QString suffix = "*." + assetPath.split('.').last().toLower();

    m_qmlBackEndForCurrentType->contextObject()->setActiveDragSuffix(suffix);
}

void PropertyEditorView::dragEnded()
{
    m_qmlBackEndForCurrentType->contextObject()->setActiveDragSuffix("");
}

void PropertyEditorView::setValue(const QmlObjectNode &qmlObjectNode,
                                  const PropertyName &name,
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
