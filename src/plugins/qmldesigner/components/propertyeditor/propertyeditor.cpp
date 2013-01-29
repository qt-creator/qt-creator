/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "propertyeditor.h"

#include <qmldesignerconstants.h>

#include <nodemetainfo.h>
#include <metainfo.h>

#include <invalididexception.h>
#include <rewritingexception.h>
#include <variantproperty.h>

#include <bindingproperty.h>

#include <nodeabstractproperty.h>
#include <rewriterview.h>

#include "propertyeditorvalue.h"
#include "basiclayouts.h"
#include "basicwidgets.h"
#include "resetwidget.h"
#include "qlayoutobject.h"
#include <qmleditorwidgets/colorwidgets.h>
#include "gradientlineqmladaptor.h"
#include "behaviordialog.h"
#include "qproxylayoutitem.h"
#include "fontwidget.h"
#include "siblingcombobox.h"
#include "propertyeditortransaction.h"
#include "originwidget.h"

#include <qmljs/qmljssimplereader.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDebug>
#include <QTimer>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QVBoxLayout>
#include <QShortcut>
#include <QStackedWidget>
#include <QDeclarativeEngine>
#include <QMessageBox>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QToolBar>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

enum {
    debug = false
};

const int collapseButtonOffset = 114;

namespace QmlDesigner {

const char resourcePropertyEditorPath[] = ":/propertyeditor";

static QString applicationDirPath()
{
#ifdef Q_OS_WIN
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return Utils::normalizePathName(QCoreApplication::applicationDirPath());
#else
    return QCoreApplication::applicationDirPath();
#endif
}

#ifdef Q_OS_MAC
#  define SHARE_PATH "/../Resources/qmldesigner"
#else
#  define SHARE_PATH "/../share/qtcreator/qmldesigner"
#endif

static inline QString sharedDirPath()
{
    QString appPath = applicationDirPath();

    return QFileInfo(appPath + SHARE_PATH).absoluteFilePath();
}

static inline QString propertyTemplatesPath()
{
    return sharedDirPath() + QLatin1String("/propertyeditor/PropertyTemplates/");
}

static QObject *variantToQObject(const QVariant &v)
{
    if (v.userType() == QMetaType::QObjectStar || v.userType() > QMetaType::User)
        return *(QObject **)v.constData();

    return 0;
}

static QmlJS::SimpleReaderNode::Ptr s_templateConfiguration;

QmlJS::SimpleReaderNode::Ptr templateConfiguration()
{
    if (!s_templateConfiguration) {
        QmlJS::SimpleReader reader;
        const QString fileName = propertyTemplatesPath() + QLatin1String("TemplateTypes.qml");
        s_templateConfiguration = reader.readFile(fileName);

        if (!s_templateConfiguration)
            qWarning().nospace() << "template definitions:" << reader.errors();
    }

    return s_templateConfiguration;
}

QStringList variantToStringList(const QVariant &variant) {
    QStringList stringList;

    foreach (const QVariant &singleValue, variant.toList())
        stringList << singleValue.toString();

    return stringList;
}

PropertyEditor::NodeType::NodeType(PropertyEditor *propertyEditor) :
        m_view(new DeclarativeWidgetView), m_propertyEditorTransaction(new PropertyEditorTransaction(propertyEditor)), m_dummyPropertyEditorValue(new PropertyEditorValue()),
        m_contextObject(new PropertyEditorContextObject())
{
    Q_ASSERT(QFileInfo(":/images/button_normal.png").exists());

    QDeclarativeContext *ctxt = m_view->rootContext();
    m_view->engine()->setOutputWarningsToStandardError(debug);
    m_view->engine()->addImportPath(sharedDirPath() + QLatin1String("/propertyeditor"));
    m_dummyPropertyEditorValue->setValue("#000000");
    ctxt->setContextProperty("dummyBackendValue", m_dummyPropertyEditorValue.data());
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    ctxt->setContextObject(m_contextObject.data());

    connect(&m_backendValuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)), propertyEditor, SLOT(changeValue(QString)));
}

PropertyEditor::NodeType::~NodeType()
{
}

void setupPropertyEditorValue(const QString &name, QDeclarativePropertyMap *propertyMap, PropertyEditor *propertyEditor, const QString &type)
{
    QString propertyName(name);
    propertyName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(propertyMap->value(propertyName)));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(propertyMap);
        QObject::connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), propertyMap, SIGNAL(valueChanged(QString,QVariant)));
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
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(propertyMap->value(propertyName)));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(propertyMap);
        QObject::connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), propertyMap, SIGNAL(valueChanged(QString,QVariant)));
        QObject::connect(valueObject, SIGNAL(expressionChanged(QString)), propertyEditor, SLOT(changeExpression(QString)));
        propertyMap->insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setName(name);
    valueObject->setModelNode(fxObjectNode);

    if (fxObjectNode.propertyAffectedByCurrentState(name) && !(fxObjectNode.modelNode().property(name).isBindingProperty()))
        valueObject->setValue(fxObjectNode.modelValue(name));

    else
        valueObject->setValue(value);

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
    PropertyEditorValue *propertyValue = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(propertyName)));
    if (propertyValue) {
        propertyValue->setValue(value);
        if (!fxObjectNode.hasBindingProperty(name))
            propertyValue->setExpression(value.toString());
        else
            propertyValue->setExpression(fxObjectNode.expression(name));
    }
}

void PropertyEditor::NodeType::setup(const QmlObjectNode &fxObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor)
{
    if (!fxObjectNode.isValid())
        return;

    QDeclarativeContext *ctxt = m_view->rootContext();

    if (fxObjectNode.isValid()) {
        foreach (const QString &propertyName, fxObjectNode.modelNode().metaInfo().propertyNames())
            createPropertyEditorValue(fxObjectNode, propertyName, fxObjectNode.instanceValue(propertyName), &m_backendValuesPropertyMap, propertyEditor);

        // className
        PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value("className")));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("className");
        valueObject->setModelNode(fxObjectNode.modelNode());
        valueObject->setValue(fxObjectNode.modelNode().simplifiedTypeName());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)));
        m_backendValuesPropertyMap.insert("className", QVariant::fromValue(valueObject));

        // id
        valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value("id")));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("id");
        valueObject->setValue(fxObjectNode.id());
        QObject::connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)));
        m_backendValuesPropertyMap.insert("id", QVariant::fromValue(valueObject));

        // anchors
        m_backendAnchorBinding.setup(QmlItemNode(fxObjectNode.modelNode()));

        ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);

        ctxt->setContextProperty("transaction", m_propertyEditorTransaction.data());

        m_contextObject->setSpecificsUrl(qmlSpecificsFile);

        m_contextObject->setStateName(stateName);
        if (!fxObjectNode.isValid())
            return;
        ctxt->setContextProperty("propertyCount", QVariant(fxObjectNode.modelNode().properties().count()));

        m_contextObject->setIsBaseState(fxObjectNode.isInBaseState());
        m_contextObject->setSelectionChanged(false);

        m_contextObject->setSelectionChanged(false);
    } else {
        qWarning() << "PropertyEditor: invalid node for setup";
    }
}

void PropertyEditor::NodeType::initialSetup(const QString &typeName, const QUrl &qmlSpecificsFile, PropertyEditor *propertyEditor)
{
    QDeclarativeContext *ctxt = m_view->rootContext();

    NodeMetaInfo metaInfo = propertyEditor->model()->metaInfo(typeName, 4, 7);

    foreach (const QString &propertyName, metaInfo.propertyNames())
        setupPropertyEditorValue(propertyName, &m_backendValuesPropertyMap, propertyEditor, metaInfo.propertyTypeName(propertyName));

    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value("className")));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("className");

    valueObject->setValue(typeName);
    QObject::connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)));
    m_backendValuesPropertyMap.insert("className", QVariant::fromValue(valueObject));

    // id
    valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value("id")));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("id");
    valueObject->setValue("id");
    QObject::connect(valueObject, SIGNAL(valueChanged(QString,QVariant)), &m_backendValuesPropertyMap, SIGNAL(valueChanged(QString,QVariant)));
    m_backendValuesPropertyMap.insert("id", QVariant::fromValue(valueObject));

    ctxt->setContextProperty("anchorBackend", &m_backendAnchorBinding);
    ctxt->setContextProperty("transaction", m_propertyEditorTransaction.data());

    m_contextObject->setSpecificsUrl(qmlSpecificsFile);

    m_contextObject->setStateName(QLatin1String("basestate"));

    m_contextObject->setIsBaseState(true);

    m_contextObject->setSpecificQmlData(QLatin1String(""));

    m_contextObject->setGlobalBaseUrl(QUrl());
}

PropertyEditor::PropertyEditor(QWidget *parent) :
        QmlModelView(parent),
        m_parent(parent),
        m_updateShortcut(0),
        m_timerId(0),
        m_stackedWidget(new StackedWidget(parent)),
        m_currentType(0),
        m_locked(false),
        m_setupCompleted(false),
        m_singleShotTimer(new QTimer(this))
{
    m_updateShortcut = new QShortcut(QKeySequence("F5"), m_stackedWidget);
    connect(m_updateShortcut, SIGNAL(activated()), this, SLOT(reloadQml()));

    m_stackedWidget->setStyleSheet(
            QLatin1String(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css")));
    m_stackedWidget->setMinimumWidth(320);
    m_stackedWidget->move(0, 0);
    connect(m_stackedWidget, SIGNAL(resized()), this, SLOT(updateSize()));

    m_stackedWidget->insertWidget(0, new QWidget(m_stackedWidget));


    static bool declarativeTypesRegistered = false;
    if (!declarativeTypesRegistered) {
        declarativeTypesRegistered = true;
        BasicWidgets::registerDeclarativeTypes();
        BasicLayouts::registerDeclarativeTypes();
        ResetWidget::registerDeclarativeType();
        QLayoutObject::registerDeclarativeType();
        QmlEditorWidgets::ColorWidgets::registerDeclarativeTypes();
        BehaviorDialog::registerDeclarativeType();
        QProxyLayoutItem::registerDeclarativeTypes();
        PropertyEditorValue::registerDeclarativeTypes();
        FontWidget::registerDeclarativeTypes();
        SiblingComboBox::registerDeclarativeTypes();
        OriginWidget::registerDeclarativeType();
        GradientLineQmlAdaptor::registerDeclarativeType();
    }
    setQmlDir(sharedDirPath() + QLatin1String("/propertyeditor"));
    m_stackedWidget->setWindowTitle(tr("Properties"));
}

PropertyEditor::~PropertyEditor()
{
    qDeleteAll(m_typeHash);
}

static inline QString fixTypeNameForPanes(const QString &typeName)
{
    QString fixedTypeName = typeName;
    fixedTypeName.replace('.', '/');
    return fixedTypeName;
}

void PropertyEditor::setupPane(const QString &typeName)
{
    NodeMetaInfo metaInfo = model()->metaInfo(typeName);

    QUrl qmlFile = fileToUrl(locateQmlFile(metaInfo, QLatin1String("Qt/ItemPane.qml")));
    QUrl qmlSpecificsFile;

    qmlSpecificsFile = fileToUrl(locateQmlFile(metaInfo, fixTypeNameForPanes(typeName) + "Specifics.qml"));
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
        PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(variantToQObject(m_currentType->m_backendValuesPropertyMap.value(propertyName)));
        const QString newId = value->value().toString();

        if (newId == m_selectedNode.id())
            return;

        if (m_selectedNode.isValidId(newId)  && !modelNodeForId(newId).isValid() ) {
            if (m_selectedNode.id().isEmpty() || newId.isEmpty()) { //no id
                try {
                    m_selectedNode.setId(newId);
                } catch (InvalidIdException &e) { //better save then sorry
                    m_locked = true;
                    value->setValue(m_selectedNode.id());
                    m_locked = false;
                    QMessageBox::warning(0, tr("Invalid Id"), e.description());
                }
            } else { //there is already an id, so we refactor
                if (rewriterView())
                    rewriterView()->renameId(m_selectedNode.id(), newId);
            }
        } else {
            m_locked = true;
            value->setValue(m_selectedNode.id());
            m_locked = false;
            if (!m_selectedNode.isValidId(newId))
                QMessageBox::warning(0, tr("Invalid Id"),  tr("%1 is an invalid id").arg(newId));
            else
                QMessageBox::warning(0, tr("Invalid Id"),  tr("%1 already exists").arg(newId));
        }
        return;
    }

    //.replace(QLatin1Char('.'), QLatin1Char('_'))
    QString underscoreName(propertyName);
    underscoreName.replace(QLatin1Char('.'), QLatin1Char('_'));
    PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(variantToQObject(m_currentType->m_backendValuesPropertyMap.value(underscoreName)));

    if (value ==0)
        return;

    QmlObjectNode fxObjectNode(m_selectedNode);

    QVariant castedValue;

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().hasProperty(propertyName)) {
        castedValue = fxObjectNode.modelNode().metaInfo().propertyCastedValue(propertyName, value->value());
    } else {
        qWarning() << "PropertyEditor:" <<propertyName << "cannot be casted (metainfo)";
        return ;
    }

    if (value->value().isValid() && !castedValue.isValid()) {
        qWarning() << "PropertyEditor:" << propertyName << "not properly casted (metainfo)";
        return ;
    }

    if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().hasProperty(propertyName))
        if (fxObjectNode.modelNode().metaInfo().propertyTypeName(propertyName) == QLatin1String("QUrl")
                || fxObjectNode.modelNode().metaInfo().propertyTypeName(propertyName) == QLatin1String("url")) { //turn absolute local file paths into relative paths
            QString filePath = castedValue.toUrl().toString();
        if (QFileInfo(filePath).exists() && QFileInfo(filePath).isAbsolute()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            castedValue = QUrl(fileDir.relativeFilePath(filePath));
        }
    }

        if (castedValue.type() == QVariant::Color) {
            QColor color = castedValue.value<QColor>();
            QColor newColor = QColor(color.name());
            newColor.setAlpha(color.alpha());
            castedValue = QVariant(newColor);
        }

        try {
            if (!value->value().isValid()) { //reset
                fxObjectNode.removeVariantProperty(propertyName);
            } else {
                if (castedValue.isValid() && !castedValue.isNull()) {
                    m_locked = true;
                    fxObjectNode.setVariantProperty(propertyName, castedValue);
                    m_locked = false;
                }
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

    RewriterTransaction transaction = beginRewriterTransaction();

    try {
        QString underscoreName(name);
        underscoreName.replace(QLatin1Char('.'), QLatin1Char('_'));

        QmlObjectNode fxObjectNode(m_selectedNode);
        PropertyEditorValue *value = qobject_cast<PropertyEditorValue*>(variantToQObject(m_currentType->m_backendValuesPropertyMap.value(underscoreName)));

        if (fxObjectNode.modelNode().metaInfo().isValid() && fxObjectNode.modelNode().metaInfo().hasProperty(name)) {
            if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("QColor")) {
                if (QColor(value->expression().remove('"')).isValid()) {
                    fxObjectNode.setVariantProperty(name, QColor(value->expression().remove('"')));
                    transaction.commit(); //committing in the try block
                    return;
                }
            } else if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("bool")) {
                if (value->expression().compare("false", Qt::CaseInsensitive) == 0 || value->expression().compare("true", Qt::CaseInsensitive) == 0) {
                    if (value->expression().compare("true", Qt::CaseInsensitive) == 0)
                        fxObjectNode.setVariantProperty(name, true);
                    else
                        fxObjectNode.setVariantProperty(name, false);
                    transaction.commit(); //committing in the try block
                    return;
                }
            } else if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("int")) {
                bool ok;
                int intValue = value->expression().toInt(&ok);
                if (ok) {
                    fxObjectNode.setVariantProperty(name, intValue);
                    transaction.commit(); //committing in the try block
                    return;
                }
            } else if (fxObjectNode.modelNode().metaInfo().propertyTypeName(name) == QLatin1String("qreal")) {
                bool ok;
                qreal realValue = value->expression().toFloat(&ok);
                if (ok) {
                    fxObjectNode.setVariantProperty(name, realValue);
                    transaction.commit(); //committing in the try block
                    return;
                }
            }
        }

        if (!value) {
            qWarning() << "PropertyEditor::changeExpression no value for " << underscoreName;
            return;
        }

        if (value->expression().isEmpty())
            return;

        if (fxObjectNode.expression(name) != value->expression() || !fxObjectNode.propertyAffectedByCurrentState(name))
            fxObjectNode.setBindingProperty(name, value->expression());

        transaction.commit(); //committing in the try block
    }

    catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void PropertyEditor::updateSize()
{
    if (!m_currentType)
        return;
    QWidget* frame = m_currentType->m_view->findChild<QWidget*>("propertyEditorFrame");
    if (frame)
        frame->resize(m_stackedWidget->size());
}

void PropertyEditor::setupPanes()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    setupPane("QtQuick.Rectangle");
    setupPane("QtQuick.Text");
    resetView();
    m_setupCompleted = true;
    QApplication::restoreOverrideCursor();
}

void PropertyEditor::otherPropertyChanged(const QmlObjectNode &fxObjectNode, const QString &propertyName)
{
    QmlModelView::otherPropertyChanged(fxObjectNode, propertyName);

    if (!m_selectedNode.isValid())
        return;

    m_locked = true;

    if (fxObjectNode.isValid() && m_currentType && fxObjectNode == m_selectedNode && fxObjectNode.currentState().isValid()) {
        AbstractProperty property = fxObjectNode.modelNode().property(propertyName);
        if (fxObjectNode == m_selectedNode || QmlObjectNode(m_selectedNode).propertyChangeForCurrentState() == fxObjectNode) {
            if ( !m_selectedNode.hasProperty(propertyName) || m_selectedNode.property(property.name()).isBindingProperty() )
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).instanceValue(property.name()));
            else
                setValue(m_selectedNode, property.name(), QmlObjectNode(m_selectedNode).modelValue(property.name()));
        }
    }

    m_locked = false;
}

void PropertyEditor::transformChanged(const QmlObjectNode &fxObjectNode, const QString &propertyName)
{
    QmlModelView::transformChanged(fxObjectNode, propertyName);

    if (!m_selectedNode.isValid())
        return;

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
        m_timerId = startTimer(100);
}

void PropertyEditor::timerEvent(QTimerEvent *timerEvent)
{
    if (m_timerId == timerEvent->timerId())
        resetView();
}

QString templateGeneration(NodeMetaInfo type, NodeMetaInfo superType, const QmlObjectNode &objectNode)
{
    if (!templateConfiguration() && templateConfiguration()->isValid())
        return QString();

    QStringList imports = variantToStringList(templateConfiguration()->property(QLatin1String("imports")));

    QString qmlTemplate = imports.join(QLatin1String("\n")) + QLatin1Char('\n');
    qmlTemplate += QLatin1String("GroupBox {\n");
    qmlTemplate += QString(QLatin1String("caption: \"%1\"\n")).arg(objectNode.modelNode().simplifiedTypeName());
    qmlTemplate += QLatin1String("layout: VerticalLayout {\n");

    QList<QString> orderedList;
    orderedList = type.propertyNames();
    qSort(orderedList);

    bool emptyTemplate = true;

    foreach (const QString &name, orderedList) {

        if (name.startsWith(QLatin1String("__")))
            continue; //private API
        QString properName = name;

        properName.replace('.', '_');

        QString typeName = type.propertyTypeName(name);
        //alias resolution only possible with instance
        if (typeName == QLatin1String("alias") && objectNode.isValid())
            typeName = objectNode.instanceType(name);

        if (!superType.hasProperty(name) && type.propertyIsWritable(name) && !name.contains(QLatin1String("."))) {
            foreach (const QmlJS::SimpleReaderNode::Ptr &node, templateConfiguration()->children())
                if (variantToStringList(node->property(QLatin1String("typeNames"))).contains(typeName)) {
                    const QString fileName = propertyTemplatesPath() + node->property(QLatin1String("sourceFile")).toString();
                    QFile file(fileName);
                    if (file.open(QIODevice::ReadOnly)) {
                        QString source = file.readAll();
                        file.close();
                        qmlTemplate += source.arg(name).arg(properName);
                        emptyTemplate = false;
                    } else {
                        qWarning().nospace() << "template definition source file not found:" << fileName;
                    }
                }
        }
    }
    qmlTemplate += QLatin1String("}\n"); //VerticalLayout
    qmlTemplate += QLatin1String("}\n"); //GroupBox

    if (emptyTemplate)
        return QString();

    return qmlTemplate;
}

void PropertyEditor::resetView()
{
    if (model() == 0)
        return;

    m_locked = true;

    if (debug)
        qDebug() << "________________ RELOADING PROPERTY EDITOR QML _______________________";

    if (m_timerId)
        killTimer(m_timerId);

    if (m_selectedNode.isValid() && model() != m_selectedNode.model())
        m_selectedNode = ModelNode();

    QString specificsClassName;
    QUrl qmlFile(qmlForNode(m_selectedNode, specificsClassName));
    QUrl qmlSpecificsFile;

    QString diffClassName;
    if (m_selectedNode.isValid()) {
        diffClassName = m_selectedNode.metaInfo().typeName();
        QList<NodeMetaInfo> hierarchy;
        hierarchy << m_selectedNode.metaInfo();
        hierarchy << m_selectedNode.metaInfo().superClasses();

        foreach (const NodeMetaInfo &info, hierarchy) {
            if (QFileInfo(fileFromUrl(qmlSpecificsFile)).exists())
                break;
            qmlSpecificsFile = fileToUrl(locateQmlFile(info, fixTypeNameForPanes(info.typeName()) + "Specifics.qml"));
            diffClassName = info.typeName();
        }
    }

    if (!QFileInfo(fileFromUrl(qmlSpecificsFile)).exists())
        diffClassName = specificsClassName;

    QString specificQmlData;

    if (m_selectedNode.isValid() && m_selectedNode.metaInfo().isValid() && diffClassName != m_selectedNode.type()) {
        //do magic !!
        specificQmlData = templateGeneration(m_selectedNode.metaInfo(), model()->metaInfo(diffClassName), m_selectedNode);
    }

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
        QDeclarativeContext *ctxt = type->m_view->rootContext();
        type->setup(fxObjectNode, currentState().name(), qmlSpecificsFile, this);
        ctxt->setContextProperty("finishedNotify", QVariant(false));
        if (specificQmlData.isEmpty())
            type->m_contextObject->setSpecificQmlData(specificQmlData);

        type->m_contextObject->setGlobalBaseUrl(qmlFile);
        type->m_contextObject->setSpecificQmlData(specificQmlData);
        type->m_view->setSource(qmlFile);
        ctxt->setContextProperty("finishedNotify", QVariant(true));
    } else {
        QmlObjectNode fxObjectNode;
        if (m_selectedNode.isValid())
            fxObjectNode = QmlObjectNode(m_selectedNode);
        QDeclarativeContext *ctxt = type->m_view->rootContext();

        ctxt->setContextProperty("finishedNotify", QVariant(false));
        if (specificQmlData.isEmpty())
            type->m_contextObject->setSpecificQmlData(specificQmlData);
        QString currentStateName = currentState().isValid() ? currentState().name() : QLatin1String("invalid state");
        type->setup(fxObjectNode, currentStateName, qmlSpecificsFile, this);
        type->m_contextObject->setGlobalBaseUrl(qmlFile);
        type->m_contextObject->setSpecificQmlData(specificQmlData);
    }

    m_stackedWidget->setCurrentWidget(type->m_view);


    QDeclarativeContext *ctxt = type->m_view->rootContext();
    ctxt->setContextProperty("finishedNotify", QVariant(true));
    /*ctxt->setContextProperty("selectionChanged", QVariant(false));
    ctxt->setContextProperty("selectionChanged", QVariant(true));
    ctxt->setContextProperty("selectionChanged", QVariant(false));*/
    type->m_contextObject->triggerSelectionChanged();

    m_currentType = type;

    m_locked = false;

    if (m_timerId)
        m_timerId = 0;

    updateSize();
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

    m_locked = true;

    resetView();
    if (!m_setupCompleted) {
        m_singleShotTimer->setSingleShot(true);
        m_singleShotTimer->setInterval(100);
        connect(m_singleShotTimer, SIGNAL(timeout()), this, SLOT(setupPanes()));
        m_singleShotTimer->start();
    }

    m_locked = false;
}

void PropertyEditor::modelAboutToBeDetached(Model *model)
{
    QmlModelView::modelAboutToBeDetached(model);
    m_currentType->m_propertyEditorTransaction->end();

    resetView();
}


void PropertyEditor::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesRemoved(propertyList);

    if (!m_selectedNode.isValid())
        return;

    if (!QmlObjectNode(m_selectedNode).isValid())
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

    if (!QmlObjectNode(m_selectedNode).isValid())
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

       if (!QmlObjectNode(m_selectedNode).isValid())
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


void PropertyEditor::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (!m_selectedNode.isValid())
        return;

    m_locked = true;
    QList<InformationName> informationNameList = informationChangeHash.values(m_selectedNode);
    if (informationNameList.contains(Anchor))
        m_currentType->m_backendAnchorBinding.setup(QmlItemNode(m_selectedNode));
    m_locked = false;
}

void PropertyEditor::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    QmlModelView::nodeIdChanged(node, newId, oldId);

    if (!m_selectedNode.isValid())
        return;

    if (!QmlObjectNode(m_selectedNode).isValid())
        return;

    if (node == m_selectedNode) {

        if (m_currentType)
            setValue(node, "id", newId);
    }
}

void PropertyEditor::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{
    QmlModelView::scriptFunctionsChanged(node, scriptFunctionList);
}

void PropertyEditor::select(const ModelNode &node)
{
    if (QmlItemNode(node).isValid())
        m_selectedNode = node;
    else
        m_selectedNode = ModelNode();

    delayedResetView();
}

QWidget *PropertyEditor::widget()
{
    return m_stackedWidget;
}


void PropertyEditor::actualStateChanged(const ModelNode &node)
{
    QmlModelView::actualStateChanged(node);
    QmlModelState newQmlModelState(node);
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
    if (nodeInfo.typeName().split('.').last() == "QDeclarativeItem")
        return "QtQuick/ItemPane.qml";
    const QString fixedTypeName = fixTypeNameForPanes(nodeInfo.typeName());
    return fixedTypeName + QLatin1String("Pane.qml");
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

QString PropertyEditor::fileFromUrl(const QUrl &url) const
{
    if (url.scheme() == QLatin1String("qrc")) {
        const QString &path = url.path();
        return QLatin1String(":") + path;
    }

    return url.toLocalFile();
}

QUrl PropertyEditor::qmlForNode(const ModelNode &modelNode, QString &className) const
{
    if (modelNode.isValid()) {
        QList<NodeMetaInfo> hierarchy;
        hierarchy << modelNode.metaInfo();
        hierarchy << modelNode.metaInfo().superClasses();

        foreach (const NodeMetaInfo &info, hierarchy) {
            QUrl fileUrl = fileToUrl(locateQmlFile(info, qmlFileName(info)));
            if (fileUrl.isValid()) {
                className = info.typeName();
                return fileUrl;
            }
        }
    }
    return fileToUrl(QDir(m_qmlDir).filePath("QtQuick/emptyPane.qml"));
}

QString PropertyEditor::locateQmlFile(const NodeMetaInfo &info, const QString &relativePath) const
{
    QDir fileSystemDir(m_qmlDir);
    static QDir resourcesDir(resourcePropertyEditorPath);
    QDir importDir(info.importDirectoryPath() + QLatin1String(Constants::QML_DESIGNER_SUBFOLDER));

    if (importDir.exists(relativePath))
        return importDir.absoluteFilePath(relativePath);
    if (fileSystemDir.exists(relativePath))
        return fileSystemDir.absoluteFilePath(relativePath);
    if (resourcesDir.exists(relativePath))
        return resourcesDir.absoluteFilePath(relativePath);

    return QString();
}


} //QmlDesigner

