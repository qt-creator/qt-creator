/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "propertyeditor.h"

#include <nodemetainfo.h>

#include <propertymetainfo.h>

#include <invalididexception.h>
#include <invalidnodestateexception.h>
#include <variantproperty.h>

#include "propertyeditorvalue.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtDeclarative/QmlView>
#include <QtDeclarative/QmlContext>
#include <QtGui/QVBoxLayout>
#include <QtGui/QShortcut>
#include <QtGui/QStackedWidget>
#include <QmlEngine>
#include <QmlMetaType>

enum {
    debug = false
};

namespace QmlDesigner {

PropertyEditor::NodeType::NodeType(const QUrl &qmlFile, PropertyEditor *propertyEditor) :
        m_view(new QmlView)
{
    Q_ASSERT(QFileInfo(":/images/button_normal.png").exists());

    m_view->setContentResizable(true);
    m_view->setUrl(qmlFile);

    connect(&m_backendValuesPropertyMap, SIGNAL(valueChanged(const QString&)), propertyEditor, SLOT(changeValue(const QString&)));
}

PropertyEditor::NodeType::~NodeType()
{
}

void createPropertyEditorValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value, QmlPropertyMap *propertyMap)
{
    QString propertyName(name);
    propertyName.replace(".", "_");
    PropertyEditorValue *valueObject = new PropertyEditorValue(propertyMap);
    valueObject->setName(propertyName);
    valueObject->setIsInModel(fxObjectNode.modelNode().hasProperty(name));
    valueObject->setIsInSubState(fxObjectNode.propertyAffectedByCurrentState(name));
    valueObject->setModelNode(fxObjectNode.modelNode());

    if (fxObjectNode.propertyAffectedByCurrentState(name) && !(fxObjectNode.modelNode().property(propertyName).isBindingProperty())) {
        valueObject->setValue(fxObjectNode.modelValue(name));

    } else {
        valueObject->setValue(value);
    }

    if (propertyName != QLatin1String("id") &&
        fxObjectNode.currentState().isBaseState() &&
        fxObjectNode.modelNode().property(propertyName).isBindingProperty()) {
        valueObject->setExpression(fxObjectNode.modelNode().bindingProperty(propertyName).expression());
    } else {
        valueObject->setExpression(fxObjectNode.instanceValue(name).toString());
    }

    QObject::connect(valueObject, SIGNAL(valueChanged(QString)), propertyMap, SIGNAL(valueChanged(QString)));
    propertyMap->insert(propertyName, QmlMetaType::qmlType(valueObject->metaObject())->fromObject(valueObject));
}

void PropertyEditor::NodeType::setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value)
{
    createPropertyEditorValue(fxObjectNode, name, value, &m_backendValuesPropertyMap);
}

void PropertyEditor::NodeType::setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile)
{
    if (!fxObjectNode.isValid())
        return;

    QmlContext *ctxt = m_view->rootContext();

    // First remove complex objects from backend, so that we don't trigger a flood of updates
    ctxt->setContextProperty("anchorBackend", 0);
    ctxt->setContextProperty("backendValues", 0);

    //foreach (const QString &propertyName, m_backendValuesPropertyMap.keys())
    //    m_backendValuesPropertyMap.clear(propertyName);
    //qDeleteAll(m_backendValuesPropertyMap.children());

    if (fxObjectNode.isValid()) {
        foreach (const QString &propertyName, fxObjectNode.modelNode().metaInfo().properties(true).keys())
            createPropertyEditorValue(fxObjectNode, propertyName, fxObjectNode.instanceValue(propertyName), &m_backendValuesPropertyMap);

        // className
        PropertyEditorValue *valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("className");
        valueObject->setModelNode(fxObjectNode.modelNode());
        valueObject->setValue(fxObjectNode.modelNode().simplifiedTypeName());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString)));
        m_backendValuesPropertyMap.insert("className", QmlMetaType::qmlType(valueObject->metaObject())->fromObject(valueObject));

        // id
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("id");
        valueObject->setIsInModel(!fxObjectNode.modelNode().id().isEmpty());
        valueObject->setModelNode(fxObjectNode.modelNode());
        valueObject->setValue(fxObjectNode.modelNode().id());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString)));
        m_backendValuesPropertyMap.insert("id", QmlMetaType::qmlType(valueObject->metaObject())->fromObject(valueObject));

        // anchors
        m_backendAnchorBinding.setup(QmlItemNode(fxObjectNode.modelNode()));

        ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);
        ctxt->setContextProperty("backendValues", &m_backendValuesPropertyMap);

        ctxt->setContextProperty("specificsUrl", QVariant(qmlSpecificsFile));
        ctxt->setContextProperty("stateName", QVariant(stateName));
        ctxt->setContextProperty("propertyCount", QVariant(fxObjectNode.modelNode().properties().count()));
        ctxt->setContextProperty("isBaseState", QVariant(fxObjectNode.isInBaseState()));
    } else {
        qWarning() << "PropertyEditor: invalid node for setup";
    }
}

PropertyEditor::PropertyEditor(QWidget *parent) :
          QmlModelView(parent),
          m_parent(parent),
          m_updateShortcut(0),
          m_timerId(0),
          m_stackedWidget(new QStackedWidget(parent)),
          m_currentType(0)
{
    m_updateShortcut = new QShortcut(QKeySequence("F5"), m_stackedWidget);
    connect(m_updateShortcut, SIGNAL(activated()), this, SLOT(reloadQml()));

    QFile file(":/qmldesigner/stylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    m_stackedWidget->setStyleSheet(styleSheet);
    m_stackedWidget->setMinimumWidth(320);
}

PropertyEditor::~PropertyEditor()
{
    delete m_stackedWidget;
    qDeleteAll(m_typeHash);
}

void PropertyEditor::changeValue(const QString &name)
{
    if (name == "type")
        return;

    if (name == "id") {
        PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QmlMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(name)));
        const QString newId = value->value().toString();

        try {
            if (ModelNode::isValidId(newId))
                m_selectedNode.setId(newId);
            else
                value->setValue(m_selectedNode.id());
        } catch (InvalidIdException &) {
            value->setValue(m_selectedNode.id());
        }

        return;
    }

    QString propertyName(name);
    propertyName.replace("_", ".");

    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QmlMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(name)));


    if (value ==0) {
        qWarning() << "PropertyEditor:" <<name << " - value is null";
        return;
    }

    QmlObjectNode fxObjectNode(m_selectedNode);

    QVariant castedValue;
    qreal castedExpressionValue;
    bool converted = false;


    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().property(propertyName, true).isValid()) {
        castedValue = fxObjectNode.modelNode().metaInfo().property(propertyName, true).castedValue(value->value());
        castedExpressionValue = value->expression().toDouble(&converted);
    } else {
        qWarning() << "PropertyEditor:" <<name << "cannot be casted (metainfo)";
        return ;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << "PropertyEditor:" <<name << "not properly casted (metainfo)";
        return ;
    }

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().property(propertyName).isValid())
        if (fxObjectNode.modelNode().metaInfo().property(propertyName).type() == QLatin1String("QUrl")) { //turn absolute local file paths into relative paths
        QString filePath = castedValue.toUrl().toString();
        if (QFileInfo(filePath).exists() && QFileInfo(filePath).isAbsolute()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            castedValue = QUrl(fileDir.relativeFilePath(filePath));
        }
    }


    if (value->expression() == value->value().toString() || !value->value().isValid() || converted) {
        if (converted)
            castedValue = castedExpressionValue;

        if (!value->value().isValid()) {
            fxObjectNode.removeVariantProperty(propertyName);
        } else {
            if (value->value().canConvert<ModelNode>()) {
                ; //fxObjectNode.setVariantProperty(propertyName, value->value().value<NodeState>().modelNode().toVariant()); ### hmmm only in basestate blah blah
            } else {
                if (castedValue.isValid() && !castedValue.isNull())
                    fxObjectNode.setVariantProperty(propertyName, castedValue);
            }
        }
    } else { //expression
        changeExpression(name);
    }
}

void PropertyEditor::changeExpression(const QString &name)
{
    QmlObjectNode fxObjectNode(m_selectedNode);
    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QmlMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(name)));
    if (fxObjectNode.currentState().isBaseState()) {
        fxObjectNode.modelNode().bindingProperty(name).setExpression(value->expression());
    }
}

void PropertyEditor::otherPropertyChanged(const QmlObjectNode &fxObjectNode)
{
    if (fxObjectNode.isValid() && m_currentType && fxObjectNode == m_selectedNode && fxObjectNode.currentState().isValid()) {
        foreach (const QString &propertyName, fxObjectNode.modelNode().metaInfo().properties(true).keys()) {
            if ( propertyName != "id" && propertyName != "objectName") {
                QString name(propertyName);
                name.replace(".", "_");
                QVariant backendValue(m_currentType->m_backendValuesPropertyMap.value(propertyName));
                PropertyEditorValue *valueObject = 0;
                if (backendValue.isValid())
                    valueObject = qobject_cast<PropertyEditorValue*>(QmlMetaType::toQObject(backendValue));
                else
                    valueObject = new PropertyEditorValue(&m_currentType->m_backendValuesPropertyMap);

                if (valueObject == 0) {
                    qWarning() << "PropertyEditor: you propably assigned a wrong value to backendValues";
                    return;
                }

                valueObject->setName(propertyName);
                valueObject->setIsInModel(fxObjectNode.hasProperty(propertyName));
                valueObject->setIsInSubState(fxObjectNode.propertyAffectedByCurrentState(propertyName));
                valueObject->setModelNode(fxObjectNode.modelNode());

                if (fxObjectNode.modelNode().property(propertyName).isBindingProperty())
                    valueObject->setValue(fxObjectNode.instanceValue(propertyName));
                else
                    valueObject->setValue(fxObjectNode.modelValue(propertyName));

                connect(valueObject, SIGNAL(valueChanged(QString)), &m_currentType->m_backendValuesPropertyMap, SIGNAL(valueChanged(QString)));
                m_currentType->m_backendValuesPropertyMap.insert(propertyName, QmlMetaType::qmlType(valueObject->metaObject())->fromObject(valueObject));
            }
        }
    }
}

void PropertyEditor::setQmlDir(const QString &qmlDir)
{
    m_qmlDir = qmlDir;

    QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
    watcher->addPath(m_qmlDir);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(reloadQml()));
}

void PropertyEditor::delayedResetView()
{
    if (m_timerId == 0)
       m_timerId = startTimer(50);
}

void PropertyEditor::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId()) {
        resetView();
    }
}

void PropertyEditor::resetView()
{
    if (model() == 0)
        return;

    if (debug)
        qDebug() << "________________ RELOADING PROPERTY EDITOR QML _______________________";

    if (m_timerId)
        killTimer(m_timerId);

    if (m_selectedNode.isValid() && model() != m_selectedNode.model())
        m_selectedNode = ModelNode();

    QUrl qmlFile(qmlForNode(m_selectedNode));
    QUrl qmlSpecificsFile;
    if (m_selectedNode.isValid())
        qmlSpecificsFile = fileToUrl(locateQmlFile(m_selectedNode.type() + "Specifics.qml"));

    NodeType *type = m_typeHash.value(qmlFile.toString());
    if (!type) {
        type = new NodeType(qmlFile, this);

        m_stackedWidget->addWidget(type->m_view);
        m_typeHash.insert(qmlFile.toString(), type);

        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid()) {
            fxObjectNode = QmlObjectNode(m_selectedNode);
            Q_ASSERT(fxObjectNode.isValid());
        }
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile);

        QmlContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("finishedNotify", QVariant(false));
        type->m_view->execute();
        ctxt->setContextProperty("finishedNotify", QVariant(true));
    } else {
        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid()) {
            fxObjectNode = QmlObjectNode(m_selectedNode);
        }
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile);
    }

    m_stackedWidget->setCurrentWidget(type->m_view);
    m_currentType = type;

    if (m_timerId)
        m_timerId = 0;
}

void PropertyEditor::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                               const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList);

    if (m_selectedNode.isValid() && selectedNodeList.contains(m_selectedNode))
        return;

    if (selectedNodeList.isEmpty())
        select(ModelNode());
    else
        select(selectedNodeList.first());
}

void PropertyEditor::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    QmlModelView::nodeAboutToBeRemoved(removedNode);
    if (m_selectedNode.isValid() && removedNode.isValid() && m_selectedNode == removedNode)
        select(m_selectedNode.parentProperty().parentModelNode());
}

void PropertyEditor::modelAttached(Model *model)
{
    QmlModelView::modelAttached(model);

    if (debug)
        qDebug() << Q_FUNC_INFO;

    resetView();
}

void PropertyEditor::modelAboutToBeDetached(Model *model)
{
    QmlModelView::modelAboutToBeDetached(model);

    resetView();
}


void PropertyEditor::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesAboutToBeRemoved(propertyList);

    if (!m_selectedNode.isValid())
        return;

    foreach (const AbstractProperty &property, propertyList) {
        if (property.isVariantProperty() || property.isBindingProperty()) {
            ModelNode node(property.parentModelNode());
            m_currentType->setValue(node, property.name(), QmlObjectNode(node).instanceValue(property.name()));
        }
    }
}


void PropertyEditor::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::variantPropertiesChanged(propertyList, propertyChange);

    if (!m_selectedNode.isValid())
        return;

    foreach (const VariantProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                m_currentType->setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                m_currentType->setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::bindingPropertiesChanged(propertyList, propertyChange);

    if (!m_selectedNode.isValid())
        return;

    foreach (const BindingProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());

        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                m_currentType->setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                m_currentType->setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    QmlModelView::nodeIdChanged(node, newId, oldId);

     if (!m_selectedNode.isValid())
        return;

     if (node == m_selectedNode) {

         if (m_currentType) {
             m_currentType->setValue(node, "id", newId);
         }
     }
}

void PropertyEditor::select(const ModelNode &node)
{
    if (node.isValid())
        m_selectedNode = node;
    else
        m_selectedNode = ModelNode();

    delayedResetView();
}

QWidget *PropertyEditor::createPropertiesPage()
{
    delayedResetView();
    return m_stackedWidget;
}

void PropertyEditor::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    QmlModelView::stateChanged(newQmlModelState, oldQmlModelState);
    Q_ASSERT(newQmlModelState.isValid());
    if (debug)
        qDebug() << Q_FUNC_INFO << newQmlModelState.name();
    delayedResetView();
}

void PropertyEditor::reloadQml()
{
    m_typeHash.clear();
    while (QWidget *widget = m_stackedWidget->widget(0)) {
        m_stackedWidget->removeWidget(widget);
        delete widget;
    }
    m_currentType = 0;

    delayedResetView();
}

QString PropertyEditor::qmlFileName(const NodeMetaInfo &nodeInfo) const
{
    return nodeInfo.typeName() + QLatin1String("Pane.qml");
}

QUrl PropertyEditor::fileToUrl(const QString &filePath) const {
    QUrl fileUrl;

    if (filePath.isEmpty())
        return fileUrl;

    if (filePath.startsWith(":")) {
        fileUrl.setScheme("qrc");
        QString path = filePath;
        path.remove(0, 1); // remove trailing ':'
        fileUrl.setPath(path);
    } else {
        fileUrl = QUrl::fromLocalFile(filePath);
    }

    return fileUrl;
}

QUrl PropertyEditor::qmlForNode(const ModelNode &modelNode) const
{
    if (modelNode.isValid()) {
        QList<NodeMetaInfo> hierarchy;
        hierarchy << modelNode.metaInfo();
        hierarchy << modelNode.metaInfo().superClasses();

        foreach (const NodeMetaInfo &info, hierarchy) {
            QUrl fileUrl = fileToUrl(locateQmlFile(qmlFileName(info)));
            if (fileUrl.isValid())
                return fileUrl;
        }
    }
    return fileToUrl(QDir(m_qmlDir).filePath("Qt/emptyPane.qml"));
}

QString PropertyEditor::locateQmlFile(const QString &relativePath) const
{
    QDir fileSystemDir(m_qmlDir);
    static QDir resourcesDir(":/propertyeditor");

    if (fileSystemDir.exists(relativePath))
        return fileSystemDir.absoluteFilePath(relativePath);
    if (resourcesDir.exists(relativePath))
        return resourcesDir.absoluteFilePath(relativePath);

    return QString();
}

} //QmlDesigner

