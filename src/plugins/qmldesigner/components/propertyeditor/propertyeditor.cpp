/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <metainfo.h>

#include <propertymetainfo.h>

#include <invalididexception.h>
#include <rewritingexception.h>
#include <invalidnodestateexception.h>
#include <variantproperty.h>
#include <propertymetainfo.h>

#include "propertyeditorvalue.h"
#include "basiclayouts.h"
#include "basicwidgets.h"
#include "resetwidget.h"
#include "qlayoutobject.h"
#include "colorwidget.h"
#include "behaviordialog.h"
#include "qproxylayoutitem.h"
#include "fontwidget.h"
#include "siblingcombobox.h"
#include "propertyeditortransaction.h"

#include <coreplugin/sidebar.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtDeclarative/QDeclarativeView>
#include <QtDeclarative/QDeclarativeContext>
#include <QtGui/QVBoxLayout>
#include <QtGui/QShortcut>
#include <QtGui/QStackedWidget>
#include <QDeclarativeEngine>
#include <private/qdeclarativemetatype_p.h>
#include <QMessageBox>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QToolBar>

enum {
    debug = false
};

const int collapseButtonOffset = 114;

namespace QmlDesigner {

PropertyEditor::NodeType::NodeType(PropertyEditor *propertyEditor) :
        m_view(new QDeclarativeView), m_propertyEditorTransaction(new PropertyEditorTransaction(propertyEditor)), m_dummyPropertyEditorValue(new PropertyEditorValue())
{
    Q_ASSERT(QFileInfo(":/images/button_normal.png").exists());

    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    QDeclarativeContext *ctxt = m_view->rootContext();
    m_dummyPropertyEditorValue->setValue("#000000");
    ctxt->setContextProperty("dummyBackendValue", m_dummyPropertyEditorValue.data());

    connect(&m_backendValuesPropertyMap, SIGNAL(valueChanged(const QString&, const QVariant&)), propertyEditor, SLOT(changeValue(const QString&)));
}

PropertyEditor::NodeType::~NodeType()
{
}

void setupPropertyEditorValue(const QString &name, QDeclarativePropertyMap *propertyMap, PropertyEditor *propertyEditor, const QString &type)
{
    QString propertyName(name);
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(propertyMap->value(propertyName)));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(propertyMap);
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), propertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        QObject::connect(valueObject, SIGNAL(expressionChanged(QString)), propertyEditor, SLOT(changeExpression(QString)));
        propertyMap->insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setName(propertyName);
    if (type == "QColor")
        valueObject->setValue(QVariant("#000000"));
    else
        valueObject->setValue(QVariant(1));

}

void createPropertyEditorValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value, QDeclarativePropertyMap *propertyMap, PropertyEditor *propertyEditor)
{
    QString propertyName(name);
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(propertyMap->value(propertyName)));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(propertyMap);
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), propertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        QObject::connect(valueObject, SIGNAL(expressionChanged(QString)), propertyEditor, SLOT(changeExpression(QString)));
        propertyMap->insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setName(name);
    valueObject->setModelNode(fxObjectNode);

    if (fxObjectNode.propertyAffectedByCurrentState(name) && !(fxObjectNode.modelNode().property(name).isBindingProperty())) {
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
}

void PropertyEditor::NodeType::setValue(const QmlObjectNode & fxObjectNode, const QString &name, const QVariant &value)
{
    QString propertyName = name;
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *propertyValue = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value(propertyName)));
    if (propertyValue) {
        propertyValue->setValue(value);
        if (!fxObjectNode.hasBindingProperty(name))
            propertyValue->setExpression(value.toString());
    }
}

void PropertyEditor::NodeType::setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor)
{
    if (!fxObjectNode.isValid())
        return;

    QDeclarativeContext *ctxt = m_view->rootContext();

    if (fxObjectNode.isValid()) {
        foreach (const QString &propertyName, fxObjectNode.modelNode().metaInfo().properties(true).keys())
            createPropertyEditorValue(fxObjectNode, propertyName, fxObjectNode.instanceValue(propertyName), &m_backendValuesPropertyMap, propertyEditor);

        // className
        PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("className")));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("className");
        valueObject->setModelNode(fxObjectNode.modelNode());
        valueObject->setValue(fxObjectNode.modelNode().simplifiedTypeName());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        m_backendValuesPropertyMap.insert("className", QVariant::fromValue(valueObject));

        // id
        valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("id")));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("id");
        valueObject->setValue(fxObjectNode.id());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
        m_backendValuesPropertyMap.insert("id", QVariant::fromValue(valueObject));

        // anchors
        m_backendAnchorBinding.setup(QmlItemNode(fxObjectNode.modelNode()));

        ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);
        QApplication::processEvents();
        ctxt->setContextProperty("transaction", m_propertyEditorTransaction.data());
        QApplication::processEvents();
        ctxt->setContextProperty("backendValues", &m_backendValuesPropertyMap);
        QApplication::processEvents();
        ctxt->setContextProperty("specificsUrl", QVariant(qmlSpecificsFile));
        QApplication::processEvents();
        ctxt->setContextProperty("stateName", QVariant(stateName));
        QApplication::processEvents();
        if (!fxObjectNode.isValid())
            return;
        ctxt->setContextProperty("propertyCount", QVariant(fxObjectNode.modelNode().properties().count()));
        ctxt->setContextProperty("isBaseState", QVariant(fxObjectNode.isInBaseState()));
        ctxt->setContextProperty("selectionChanged", QVariant(false));
    } else {
        qWarning() << "PropertyEditor: invalid node for setup";
    }
}

void PropertyEditor::NodeType::initialSetup(const QString &typeName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor)
{
    QDeclarativeContext *ctxt = m_view->rootContext();

    NodeMetaInfo metaInfo = propertyEditor->model()->metaInfo().nodeMetaInfo(typeName, 4, 6);

    foreach (const QString &propertyName, metaInfo.properties(true).keys())
        setupPropertyEditorValue(propertyName, &m_backendValuesPropertyMap, propertyEditor, metaInfo.property(propertyName, true).type());

    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("className")));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("className");

    valueObject->setValue(typeName);
    QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
    m_backendValuesPropertyMap.insert("className", QVariant::fromValue(valueObject));

    // id
    valueObject = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_backendValuesPropertyMap.value("id")));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("id");
    valueObject->setValue("id");
    QObject::connect(valueObject, SIGNAL(valueChanged(QString, const QVariant&)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString, const QVariant&)));
    m_backendValuesPropertyMap.insert("id", QVariant::fromValue(valueObject));

    ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);
    ctxt->setContextProperty("transaction", m_propertyEditorTransaction.data());
    ctxt->setContextProperty("backendValues", &m_backendValuesPropertyMap);

    ctxt->setContextProperty("specificsUrl", QVariant(qmlSpecificsFile));
    ctxt->setContextProperty("stateName", QVariant(QLatin1String("basestate")));
    ctxt->setContextProperty("isBaseState", QVariant(true));
}

PropertyEditor::PropertyEditor(QWidget *parent) :
        QmlModelView(parent),
        m_parent(parent),
        m_updateShortcut(0),
        m_timerId(0),
        m_stackedWidget(new StackedWidget(parent)),
        m_collapseButton(new CollapseButton(m_stackedWidget)),
        m_currentType(0),
        m_locked(false)
{
    m_updateShortcut = new QShortcut(QKeySequence("F5"), m_stackedWidget);
    connect(m_updateShortcut, SIGNAL(activated()), this, SLOT(reloadQml()));

    QFile file(":/qmldesigner/stylesheet.css");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    m_stackedWidget->setStyleSheet(styleSheet);
    m_stackedWidget->setMinimumWidth(300);
    m_collapseButton->raise();
    m_collapseButton->show();
    connect(m_collapseButton, SIGNAL(expand()), this, SLOT(expand()));
    connect(m_collapseButton, SIGNAL(collapse()), this, SLOT(collapse()));
    connect(m_stackedWidget, SIGNAL(resized()), this, SLOT(updateCollapseButton()));

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));


    static bool declarativeTypesRegistered = false;
    if (!declarativeTypesRegistered) {
        declarativeTypesRegistered = true;
        BasicWidgets::registerDeclarativeTypes();
        BasicLayouts::registerDeclarativeTypes();
        ResetWidget::registerDeclarativeType();
        QLayoutObject::registerDeclarativeType();
        ColorWidget::registerDeclarativeTypes();
        BehaviorDialog::registerDeclarativeType();
        QProxyLayoutItem::registerDeclarativeTypes();
        PropertyEditorValue::registerDeclarativeTypes();
        FontWidget::registerDeclarativeTypes();
        SiblingComboBox::registerDeclarativeTypes();
    }
}

PropertyEditor::~PropertyEditor()
{
    delete m_stackedWidget;
    qDeleteAll(m_typeHash);
}

void PropertyEditor::setupPane(const QString &typeName)
{

    QUrl qmlFile = fileToUrl(locateQmlFile(QLatin1String("Qt/ItemPane.qml")));
    QUrl qmlSpecificsFile;

    qmlSpecificsFile = fileToUrl(locateQmlFile(typeName + "Specifics.qml"));
    NodeType *type = m_typeHash.value(qmlFile.toString());

    if (!type) {
        type = new NodeType(this);

        QDeclarativeContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("finishedNotify", QVariant(false) );
        type->initialSetup(typeName, qmlSpecificsFile, this);
        type->m_view->setSource(qmlFile);
        ctxt->setContextProperty("finishedNotify", QVariant(true) );

        m_stackedWidget->addWidget(type->m_view);
        m_typeHash.insert(qmlFile.toString(), type);
    } else {
        QDeclarativeContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("finishedNotify", QVariant(false) );

        type->initialSetup(typeName, qmlSpecificsFile, this);
        ctxt->setContextProperty("finishedNotify", QVariant(true) );
    }
    m_stackedWidget->setCurrentWidget(type->m_view);
}

void PropertyEditor::changeValue(const QString &propertyName)
{
    if (propertyName.isNull())
        return;

    if (m_locked)
        return;

    if (propertyName == "type")
        return;

    if (!m_selectedNode.isValid())
        return;

    if (propertyName == "id") {
        PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(propertyName)));
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

    //.replace(QLatin1Char('.'), QLatin1Char('_'))
    QString underscoreName(propertyName);
    underscoreName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(underscoreName)));

    if (value ==0) {
        return;
    }

    QmlObjectNode fxObjectNode(m_selectedNode);

    QVariant castedValue;

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().property(propertyName, true).isValid()) {
        castedValue = fxObjectNode.modelNode().metaInfo().property(propertyName, true).castedValue(value->value());
    } else {
        qWarning() << "PropertyEditor:" <<propertyName << "cannot be casted (metainfo)";
        return ;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << "PropertyEditor:" << propertyName << "not properly casted (metainfo)";
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

    try {
        if (!value->value().isValid()) { //reset
            fxObjectNode.removeVariantProperty(propertyName);
        } else {
            if (castedValue.isValid() && !castedValue.isNull())
                m_locked = true;
                fxObjectNode.setVariantProperty(propertyName, castedValue);
                m_locked = false;
        }
    }
    catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void PropertyEditor::changeExpression(const QString &name)
{
    if (name.isNull())
        return;

    if (m_locked)
        return;

    QString underscoreName(name);
    underscoreName.replace(QLatin1Char('.'), QLatin1Char('_'));

    QmlObjectNode fxObjectNode(m_selectedNode);
    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(QDeclarativeMetaType::toQObject(m_currentType->m_backendValuesPropertyMap.value(underscoreName)));

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().property(name).isValid()) {
        if (fxObjectNode.modelNode().metaInfo().property(name).type() == QLatin1String("QColor")) {
            if (QColor(value->expression().remove('"')).isValid()) {
                fxObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                return;
            }
        } else if (fxObjectNode.modelNode().metaInfo().property(name).type() == QLatin1String("bool")) {
            if (value->expression().compare("false", Qt::CaseInsensitive) == 0 || value->expression().compare("true", Qt::CaseInsensitive) == 0) {
                if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                    fxObjectNode.setVariantProperty(name, true);
                else
                    fxObjectNode.setVariantProperty(name, false);
                return;
            }
        } else if (fxObjectNode.modelNode().metaInfo().property(name).type() == QLatin1String("int")) {
            bool ok;
            int intValue = value->expression().toInt(&ok);
            if (ok) {
                fxObjectNode.setVariantProperty(name, intValue);
                return;
            }
        } else if (fxObjectNode.modelNode().metaInfo().property(name).type() == QLatin1String("qreal")) {
            bool ok;
            qreal realValue = value->expression().toFloat(&ok);
            if (ok) {
                fxObjectNode.setVariantProperty(name, realValue);
                return;
            }
        }
    }

    if (!value) {
        qWarning() << "PropertyEditor::changeExpression no value for " << underscoreName;
        return;
    }

    try {
        if (fxObjectNode.currentState().isBaseState()) {
            if (fxObjectNode.modelNode().bindingProperty(name).expression() != value->expression())
                fxObjectNode.modelNode().bindingProperty(name).setExpression(value->expression());
        }
    }

    catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}


void PropertyEditor::expand()
{
    m_stackedWidget->setMinimumWidth(300);
    m_stackedWidget->setMaximumWidth(1000);
    m_stackedWidget->parentWidget()->layout()->update();

    QWidget * parentWidget = m_stackedWidget->parentWidget();
    QWidget *childWidget = m_stackedWidget;

    while (parentWidget) {
        if (Core::SideBar *sideBar  = qobject_cast<Core::SideBar*>(parentWidget)) {
            childWidget->setMaximumWidth(2000);
            foreach (QObject* child, sideBar->children()) {
                if (QWidget* otherWidget = qobject_cast<QWidget*>(child))
                    if (otherWidget != childWidget && !qobject_cast<QComboBox*>(otherWidget))
                    otherWidget->show();
            }
            QList<QComboBox*> list = sideBar->findChildren<QComboBox*>();
            foreach (QComboBox* box, list) {
                if (qobject_cast<QToolBar*>(box->parentWidget()))
                    box->parentWidget()->show();
            }
            sideBar->refresh();
            sideBar->update();
            break;
        }
        childWidget = parentWidget;
        parentWidget = parentWidget->parentWidget();
    }
    m_collapseButton->move(0, m_stackedWidget->height() - collapseButtonOffset);
    resetView();
}

void PropertyEditor::collapse()
{
    m_stackedWidget->setCurrentIndex(0);
    m_stackedWidget->setMinimumWidth(0);

    QWidget * parentWidget = m_stackedWidget->parentWidget();
    QWidget *childWidget = m_stackedWidget;

    while (parentWidget) {
        if (Core::SideBar *sideBar  = qobject_cast<Core::SideBar*>(parentWidget)) {
            childWidget->setMaximumWidth(22);
            foreach (QObject* child, sideBar->children()) {
                if (QWidget* otherWidget = qobject_cast<QWidget*>(child))
                    if (otherWidget != childWidget && !qobject_cast<QComboBox*>(otherWidget))
                    otherWidget->hide();
            }

            QSplitter* parentSplitter = qobject_cast<QSplitter*>(sideBar->parentWidget());
            QList<int> s = parentSplitter->sizes();
            s.removeLast();
            s.append(25);
            parentSplitter->setSizes(s);
            parentSplitter->refresh();
            parentSplitter->update();
            QApplication::processEvents();
            QList<QComboBox*> list = sideBar->findChildren<QComboBox*>();
            foreach (QComboBox* box, list) {
                if (qobject_cast<QToolBar*>(box->parentWidget())) {
                    box->parentWidget()->hide();
                }
            }
            break;
        }
        childWidget = parentWidget;
        parentWidget = parentWidget->parentWidget();
    }
    m_collapseButton->raise();
    m_stackedWidget->parentWidget()->layout()->activate();
    m_collapseButton->move(0, m_stackedWidget->height() - collapseButtonOffset + 20);
}

void PropertyEditor::updateCollapseButton()
{
    m_collapseButton->move(0, m_stackedWidget->height() - collapseButtonOffset + 20);
    m_collapseButton->update();
}

void PropertyEditor::otherPropertyChanged(const QmlObjectNode &fxObjectNode, const QString &propertyName)
{
    QmlModelView::otherPropertyChanged(fxObjectNode, propertyName);

    if (fxObjectNode.isValid() && m_currentType && fxObjectNode == m_selectedNode && fxObjectNode.currentState().isValid()) {
        AbstractProperty property = fxObjectNode.modelNode().property(propertyName);
        if (fxObjectNode == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == fxObjectNode) {
            if ( m_selectedNode.property(property.name()).isBindingProperty() || !m_selectedNode.hasProperty(propertyName))
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }
}

void PropertyEditor::transformChanged(const QmlObjectNode &fxObjectNode, const QString &propertyName)
{
    QmlModelView::transformChanged(fxObjectNode, propertyName);

    if (fxObjectNode.isValid() && m_currentType && fxObjectNode == m_selectedNode && fxObjectNode.currentState().isValid()) {
        AbstractProperty property = fxObjectNode.modelNode().property(propertyName);
        if (fxObjectNode == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == fxObjectNode) {
            if ( m_selectedNode.property(property.name()).isBindingProperty() || !m_selectedNode.hasProperty(propertyName))
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
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
        m_timerId = startTimer(200);
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

    if (m_collapseButton->isCollapsed())
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

    m_locked = true;

    NodeType *type = m_typeHash.value(qmlFile.toString());

    if (!type) {
        type = new NodeType(this);

        m_stackedWidget->addWidget(type->m_view);
        m_typeHash.insert(qmlFile.toString(), type);

        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid()) {
            fxObjectNode = QmlObjectNode(m_selectedNode);
            Q_ASSERT(fxObjectNode.isValid());
        }
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile, this);

        QDeclarativeContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("finishedNotify", QVariant(false));
        type->m_view->setSource(qmlFile);
        ctxt->setContextProperty("finishedNotify", QVariant(true));
    } else {
        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid()) {
            fxObjectNode = QmlObjectNode(m_selectedNode);
        }
        QDeclarativeContext *ctxt = type->m_view->rootContext();
        ctxt->setContextProperty("selectionChanged", QVariant(false));
        ctxt->setContextProperty("selectionChanged", QVariant(true));
        ctxt->setContextProperty("selectionChanged", QVariant(false));
        ctxt->setContextProperty("finishedNotify", QVariant(false));
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile, this);
    }

    m_stackedWidget->setCurrentWidget(type->m_view);

    m_collapseButton->raise();
    m_collapseButton->move(0, m_stackedWidget->height() - collapseButtonOffset);

    QDeclarativeContext *ctxt = type->m_view->rootContext();
    ctxt->setContextProperty("finishedNotify", QVariant(true));
    ctxt->setContextProperty("selectionChanged", QVariant(false));
    ctxt->setContextProperty("selectionChanged", QVariant(true));
    ctxt->setContextProperty("selectionChanged", QVariant(false));

    m_currentType = type;

    m_locked = false;

    if (m_timerId)
        m_timerId = 0;
}

void PropertyEditor::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                          const QList<ModelNode> &lastSelectedNodeList)
{
    Q_UNUSED(lastSelectedNodeList);

    if (selectedNodeList.isEmpty() || selectedNodeList.count() > 1)
        select(ModelNode());
    else if (m_selectedNode != selectedNodeList.first())
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

    setupPane("Qt/Rectangle");
    setupPane("Qt/Text");
    setupPane("Qt/TextInput");
    setupPane("Qt/TextEdit");
    resetView();
}

void PropertyEditor::modelAboutToBeDetached(Model *model)
{
    QmlModelView::modelAboutToBeDetached(model);

    resetView();
}


void PropertyEditor::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesRemoved(propertyList);

    if (!m_selectedNode.isValid())
        return;

    foreach (const AbstractProperty &property, propertyList) {
        ModelNode node(property.parentModelNode());
        if (node == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == node) {
            setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            if (property.name().contains("anchor", Qt::CaseInsensitive))
                m_currentType->m_backendAnchorBinding.invalidate(m_selectedNode);
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
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
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
            if (property.name().contains("anchor", Qt::CaseInsensitive))
                m_currentType->m_backendAnchorBinding.invalidate(m_selectedNode);
            if ( QmlObjectNode(m_selectedNode).modelNode().property(property.name()).isBindingProperty())
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
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
            setValue(node, "id", newId);
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

void PropertyEditor::setValue(const QmlObjectNode &fxObjectNode, const QString &name, const QVariant &value)
{
    m_locked = true;
    m_currentType->setValue(fxObjectNode, name, value);
    m_locked = false;
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

    if (filePath.startsWith(QLatin1Char(':'))) {
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

CollapseButton::CollapseButton(QWidget *parent) : QWidget(parent), m_collapsed(false), m_hovered(false),
                                  m_pixmap_normal(":/qmldesigner/images/collapse-button-normal.png"),
                                  m_pixmap_hover(":/qmldesigner/images/collapse-button-hover.png"),
                                  m_pixmap_normal_mirrored(":/qmldesigner/images/collapse-button-normal-mirrored.png"),
                                  m_pixmap_hover_mirrored(":/qmldesigner/images/collapse-button-hover-mirrored.png")
{
    setFixedWidth(20);
    setFixedHeight(81);
    setMouseTracking(true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    QGraphicsOpacityEffect* graphicsOpacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(graphicsOpacityEffect);
    graphicsOpacityEffect->setOpacity(0.4);
    setToolTip(QLatin1String("Properties"));
}


void CollapseButton::setOpacity(qreal opacity)
{
    qobject_cast<QGraphicsOpacityEffect*>(graphicsEffect())->setOpacity(opacity);
}

void CollapseButton::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter p(this);

    if (m_hovered) {
        if (m_collapsed)
            p.drawPixmap(0,0, m_pixmap_hover_mirrored);
        else
            p.drawPixmap(0,0, m_pixmap_hover);
    } else {
        if (m_collapsed)
            p.drawPixmap(0,0, m_pixmap_normal_mirrored);
        else
            p.drawPixmap(0,0, m_pixmap_normal);
    }
}

} //QmlDesigner

