/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "propertyeditorview.h"

#include "propertyeditorqmlbackend.h"
#include "propertyeditorvalue.h"
#include "propertyeditortransaction.h"

#include <qmldesignerconstants.h>
#include <qmltimeline.h>
#include <nodemetainfo.h>

#include <invalididexception.h>
#include <rewritingexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDebug>
#include <QTimer>
#include <QShortcut>
#include <QApplication>

enum {
    debug = false
};

namespace QmlDesigner {

static bool propertyIsAttachedLayoutProperty(const PropertyName &propertyName)
{
    return propertyName.contains("Layout.");
}

PropertyEditorView::PropertyEditorView(QWidget *parent) :
        AbstractView(parent),
        m_parent(parent),
        m_updateShortcut(nullptr),
        m_timerId(0),
        m_stackedWidget(new PropertyEditorWidget(parent)),
        m_qmlBackEndForCurrentType(nullptr),
        m_locked(false),
        m_setupCompleted(false),
        m_singleShotTimer(new QTimer(this))
{
    m_qmlDir = PropertyEditorQmlBackend::propertyEditorResourcesPath();

    if (Utils::HostOsInfo().isMacHost())
        m_updateShortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_F3), m_stackedWidget);
    else
        m_updateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F3), m_stackedWidget);
    connect(m_updateShortcut, &QShortcut::activated, this, &PropertyEditorView::reloadQml);

    m_stackedWidget->setStyleSheet(Theme::replaceCssColors(
            QString::fromUtf8(Utils::FileReader::fetchQrc(QStringLiteral(":/qmldesigner/stylesheet.css")))));
    m_stackedWidget->setMinimumWidth(340);
    m_stackedWidget->move(0, 0);
    connect(m_stackedWidget, &PropertyEditorWidget::resized, this, &PropertyEditorView::updateSize);

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));

    Quick2PropertyEditorView::registerQmlTypes();
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
        qmlBackend = new PropertyEditorQmlBackend(this);

        qmlBackend->context()->setContextProperty("finishedNotify", QVariant(false) );
        qmlBackend->initialSetup(typeName, qmlSpecificsFile, this);
        qmlBackend->setSource(qmlFile);
        qmlBackend->context()->setContextProperty("finishedNotify", QVariant(true) );

        m_stackedWidget->addWidget(qmlBackend->widget());
        m_qmlBackendHash.insert(qmlFile.toString(), qmlBackend);
    } else {
        qmlBackend->context()->setContextProperty("finishedNotify", QVariant(false) );

        qmlBackend->initialSetup(typeName, qmlSpecificsFile, this);
        qmlBackend->context()->setContextProperty("finishedNotify", QVariant(true) );
    }
}

void PropertyEditorView::changeValue(const QString &name)
{
    PropertyName propertyName = name.toUtf8();

    if (propertyName.isNull())
        return;

    if (locked())
        return;

    if (propertyName == "className")
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
            if (!QmlDesigner::ModelNode::isValidId(newId))
                Core::AsynchronousMessageBox::warning(tr("Invalid Id"),  tr("%1 is an invalid id.").arg(newId));
            else
                Core::AsynchronousMessageBox::warning(tr("Invalid Id"),  tr("%1 already exists.").arg(newId));
        }
        return;
    }

    PropertyName underscoreName(propertyName);
    underscoreName.replace('.', '_');
    PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(QString::fromLatin1(underscoreName));

    if (value ==nullptr)
        return;

    if (propertyName.endsWith( "__AUX")) {
        commitAuxValueToModel(propertyName, value->value());
        return;
    }

    QmlObjectNode qmlObjectNode(m_selectedNode);

    QVariant castedValue;

    if (qmlObjectNode.modelNode().metaInfo().isValid() && qmlObjectNode.modelNode().metaInfo().hasProperty(propertyName)) {
        castedValue = qmlObjectNode.modelNode().metaInfo().propertyCastedValue(propertyName, value->value());
    } else if (propertyIsAttachedLayoutProperty(propertyName)) {
        castedValue = value->value();
    } else {
        qWarning() << "PropertyEditor:" <<propertyName << "cannot be casted (metainfo)";
        return ;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << "PropertyEditor:" << propertyName << "not properly casted (metainfo)";
        return ;
    }

    if (qmlObjectNode.modelNode().metaInfo().isValid() && qmlObjectNode.modelNode().metaInfo().hasProperty(propertyName)) {
        if (qmlObjectNode.modelNode().metaInfo().propertyTypeName(propertyName) == "QUrl"
                || qmlObjectNode.modelNode().metaInfo().propertyTypeName(propertyName) == "url") { //turn absolute local file paths into relative paths
                QString filePath = castedValue.toUrl().toString();
            QFileInfo fi(filePath);
            if (fi.exists() && fi.isAbsolute()) {
                QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
                castedValue = QUrl(fileDir.relativeFilePath(filePath));
            }
        }
    }

    if (name == "state" && castedValue.toString() == "base state")
        castedValue = "";

    if (castedValue.type() == QVariant::Color) {
        QColor color = castedValue.value<QColor>();
        QColor newColor = QColor(color.name());
        newColor.setAlpha(color.alpha());
        castedValue = QVariant(newColor);
    }

    if (!value->value().isValid()) { //reset
        removePropertyFromModel(propertyName);
    } else {
        // QVector*D(0, 0, 0) detects as null variant though it is valid value
        if (castedValue.isValid() && (!castedValue.isNull()
                                      || castedValue.type() == QVariant::Vector2D
                                      || castedValue.type() == QVariant::Vector3D)) {
            commitVariantValueToModel(propertyName, castedValue);
        }
    }
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

    executeInTransaction("PropertyEditorView::changeExpression", [this, name](){
        PropertyName underscoreName(name);
        underscoreName.replace('.', '_');

        QmlObjectNode qmlObjectNode(m_selectedNode);
        PropertyEditorValue *value = m_qmlBackEndForCurrentType->propertyValueForName(QString::fromLatin1(underscoreName));

        if (!value) {
            qWarning() << "PropertyEditor::changeExpression no value for " << underscoreName;
            return;
        }

        if (qmlObjectNode.modelNode().metaInfo().isValid() && qmlObjectNode.modelNode().metaInfo().hasProperty(name)) {
            if (qmlObjectNode.modelNode().metaInfo().propertyTypeName(name) == "QColor") {
                if (QColor(value->expression().remove('"')).isValid()) {
                    qmlObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                    return;
                }
            } else if (qmlObjectNode.modelNode().metaInfo().propertyTypeName(name) == "bool") {
                if (value->expression().compare(QLatin1String("false"), Qt::CaseInsensitive) == 0
                        || value->expression().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0) {
                    if (value->expression().compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
                        qmlObjectNode.setVariantProperty(name, true);
                    else
                        qmlObjectNode.setVariantProperty(name, false);
                    return;
                }
            } else if (qmlObjectNode.modelNode().metaInfo().propertyTypeName(name) == "int") {
                bool ok;
                int intValue = value->expression().toInt(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, intValue);
                    return;
                }
            } else if (qmlObjectNode.modelNode().metaInfo().propertyTypeName(name) == "qreal") {
                bool ok;
                qreal realValue = value->expression().toDouble(&ok);
                if (ok) {
                    qmlObjectNode.setVariantProperty(name, realValue);
                    return;
                }
            }
        }

        if (value->expression().isEmpty()) {
            value->resetValue();
            return;
        }

        if (qmlObjectNode.expression(name) != value->expression() || !qmlObjectNode.propertyAffectedByCurrentState(name))
            qmlObjectNode.setBindingProperty(name, value->expression());

    }); /* end of transaction */
}

void PropertyEditorView::exportPopertyAsAlias(const QString &name)
{
    if (name.isNull())
        return;

    if (locked())
        return;

    if (noValidSelection())
        return;

    executeInTransaction("PropertyEditorView::exportPopertyAsAlias", [this, name](){
        const QString id = m_selectedNode.validId();
        QString upperCasePropertyName = name;
        upperCasePropertyName.replace(0, 1, upperCasePropertyName.at(0).toUpper());
        QString aliasName = id + upperCasePropertyName;
        aliasName.replace(".", ""); //remove all dots

        PropertyName propertyName = aliasName.toUtf8();
        if (rootModelNode().hasProperty(propertyName)) {
            Core::AsynchronousMessageBox::warning(tr("Cannot Export Property as Alias"),
                                                  tr("Property %1 does already exist for root item.").arg(aliasName));
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

    executeInTransaction("PropertyEditorView::exportPopertyAsAlias", [this, name](){
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


void PropertyEditorView::setupQmlBackend()
{
    TypeName specificsClassName;

    const NodeMetaInfo commonAncestor = PropertyEditorQmlBackend::findCommonAncestor(m_selectedNode);

    const QUrl qmlFile(PropertyEditorQmlBackend::getQmlUrlForMetaInfo(commonAncestor, specificsClassName));
    QUrl qmlSpecificsFile;

    TypeName diffClassName;
    if (commonAncestor.isValid()) {
        diffClassName = commonAncestor.typeName();
        foreach (const NodeMetaInfo &metaInfo, commonAncestor.classHierarchy()) {
            if (PropertyEditorQmlBackend::checkIfUrlExists(qmlSpecificsFile))
                break;
            qmlSpecificsFile = PropertyEditorQmlBackend::getQmlFileUrl(metaInfo.typeName() + "Specifics", metaInfo);
            diffClassName = metaInfo.typeName();
        }
    }

    if (!PropertyEditorQmlBackend::checkIfUrlExists(qmlSpecificsFile))
        diffClassName = specificsClassName;

    QString specificQmlData;

    if (commonAncestor.isValid() && m_selectedNode.metaInfo().isValid() && diffClassName != m_selectedNode.type())
        specificQmlData = PropertyEditorQmlBackend::templateGeneration(commonAncestor, model()->metaInfo(diffClassName), m_selectedNode);

    PropertyEditorQmlBackend *currentQmlBackend = m_qmlBackendHash.value(qmlFile.toString());

    QString currentStateName = currentState().isBaseState() ? currentState().name() : QStringLiteral("invalid state");

    if (!currentQmlBackend) {
        currentQmlBackend = new PropertyEditorQmlBackend(this);

        m_stackedWidget->addWidget(currentQmlBackend->widget());
        m_qmlBackendHash.insert(qmlFile.toString(), currentQmlBackend);

        QmlObjectNode qmlObjectNode;
        if (m_selectedNode.isValid()) {
            qmlObjectNode = QmlObjectNode(m_selectedNode);
            Q_ASSERT(qmlObjectNode.isValid());
            currentQmlBackend->setup(qmlObjectNode, currentStateName, qmlSpecificsFile, this);
        }
        currentQmlBackend->context()->setContextProperty("finishedNotify", QVariant(false));
        if (specificQmlData.isEmpty())
            currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);

        currentQmlBackend->contextObject()->setGlobalBaseUrl(qmlFile);
        currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
        currentQmlBackend->setSource(qmlFile);
        currentQmlBackend->context()->setContextProperty("finishedNotify", QVariant(true));
    } else {
        QmlObjectNode qmlObjectNode;
        if (m_selectedNode.isValid())
            qmlObjectNode = QmlObjectNode(m_selectedNode);

        currentQmlBackend->context()->setContextProperty("finishedNotify", QVariant(false));
        if (specificQmlData.isEmpty())
            currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
        currentQmlBackend->setup(qmlObjectNode, currentStateName, qmlSpecificsFile, this);
        currentQmlBackend->contextObject()->setGlobalBaseUrl(qmlFile);
        currentQmlBackend->contextObject()->setSpecificQmlData(specificQmlData);
    }

    m_stackedWidget->setCurrentWidget(currentQmlBackend->widget());

    currentQmlBackend->context()->setContextProperty("finishedNotify", QVariant(true));

    currentQmlBackend->contextObject()->triggerSelectionChanged();

    m_qmlBackEndForCurrentType = currentQmlBackend;

}

void PropertyEditorView::commitVariantValueToModel(const PropertyName &propertyName, const QVariant &value)
{
    m_locked = true;
    try {
        RewriterTransaction transaction = beginRewriterTransaction("PropertyEditorView::commitVariantValueToMode");

        for (const ModelNode &node : m_selectedNode.view()->selectedModelNodes()) {
            if (QmlObjectNode::isValidQmlObjectNode(node))
                QmlObjectNode(node).setVariantProperty(propertyName, value);
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
                node.setAuxiliaryData(name, value);
            }
        } else {
            for (const ModelNode &node : m_selectedNode.view()->selectedModelNodes()) {
                node.removeAuxiliaryData(name);
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

    if (debug)
        qDebug() << Q_FUNC_INFO;

    m_locked = true;

    if (!m_setupCompleted) {
        QTimer::singleShot(50, this, [this]{
            PropertyEditorView::setupPanes();
            /* workaround for QTBUG-75847 */
            reloadQml();
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

void PropertyEditorView::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    if (noValidSelection())
        return;

    foreach (const AbstractProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (node.isRootNode() && !m_selectedNode.isRootNode())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedNode).isAliasExported());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));

            if (propertyIsAttachedLayoutProperty(property.name())) {
                m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, property.name());

                if (property.name() == "Layout.margins") {
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.topMargin");
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.bottomMargin");
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.leftMargin");
                    m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, "Layout.rightMargin");

                }
            }

            if ("width" == property.name() || "height" == property.name()) {
                const QmlItemNode qmlItemNode = m_selectedNode;
                if (qmlItemNode.isValid() && qmlItemNode.isInLayout())
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

    foreach (const VariantProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (propertyIsAttachedLayoutProperty(property.name()))
            m_qmlBackEndForCurrentType->setValueforLayoutAttachedProperties(m_selectedNode, property.name());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditorView::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags /*propertyChange*/)
{
    if (noValidSelection())
        return;

    foreach (const BindingProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (property.isAliasExport())
            m_qmlBackEndForCurrentType->contextObject()->setHasAliasExport(QmlObjectNode(m_selectedNode).isAliasExported());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if (property.name().contains("anchor"))
                m_qmlBackEndForCurrentType->backendAnchorBinding().invalidate(m_selectedNode);
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));

        }
    }
}

void PropertyEditorView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &)
{

    if (noValidSelection())
        return;

    if (!node.isSelected())
        return;

    m_qmlBackEndForCurrentType->setValueforAuxiliaryProperties(m_selectedNode, name);

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
    return createWidgetInfo(m_stackedWidget, nullptr, QStringLiteral("Properties"), WidgetInfo::RightPane, 0, tr("Properties"));
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
    foreach (const ModelNodePropertyPair &propertyPair, propertyList) {
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

void PropertyEditorView::setValue(const QmlObjectNode &qmlObjectNode, const PropertyName &name, const QVariant &value)
{
    m_locked = true;
    m_qmlBackEndForCurrentType->setValue(qmlObjectNode, name, value);
    m_locked = false;
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


} //QmlDesigner

