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

#include "propertyeditorqmlbackend.h"

#include "propertyeditorvalue.h"
#include "propertyeditortransaction.h"
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <qmlobjectnode.h>
#include <nodemetainfo.h>
#include <variantproperty.h>
#include <bindingproperty.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <qmljs/qmljssimplereader.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(propertyEditorBenchmark, "qtc.propertyeditor.load")

static QmlJS::SimpleReaderNode::Ptr s_templateConfiguration = QmlJS::SimpleReaderNode::Ptr();

static inline QString propertyTemplatesPath()
{
    return QmlDesigner::PropertyEditorQmlBackend::propertyEditorResourcesPath() + QStringLiteral("/PropertyTemplates/");
}

QmlJS::SimpleReaderNode::Ptr templateConfiguration()
{
    if (!s_templateConfiguration) {
        QmlJS::SimpleReader reader;
        const QString fileName = propertyTemplatesPath() + QStringLiteral("TemplateTypes.qml");
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

static QObject *variantToQObject(const QVariant &value)
{
    if (value.userType() == QMetaType::QObjectStar || value.userType() > QMetaType::User)
        return *(QObject **)value.constData();

    return 0;
}

namespace QmlDesigner {

PropertyEditorQmlBackend::PropertyEditorQmlBackend(PropertyEditorView *propertyEditor) :
        m_view(new Quick2PropertyEditorView), m_propertyEditorTransaction(new PropertyEditorTransaction(propertyEditor)), m_dummyPropertyEditorValue(new PropertyEditorValue()),
        m_contextObject(new PropertyEditorContextObject())
{
    m_view->engine()->setOutputWarningsToStandardError(QmlDesignerPlugin::instance()
        ->settings().value(DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS).toBool());

    m_view->engine()->addImportPath(propertyEditorResourcesPath());
    m_dummyPropertyEditorValue->setValue(QLatin1String("#000000"));
    context()->setContextProperty(QLatin1String("dummyBackendValue"), m_dummyPropertyEditorValue.data());
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    m_contextObject->setModel(propertyEditor->model());
    m_contextObject->insertInQmlContext(context());

    QObject::connect(&m_backendValuesPropertyMap, &DesignerPropertyMap::valueChanged,
                     propertyEditor, &PropertyEditorView::changeValue);
}

PropertyEditorQmlBackend::~PropertyEditorQmlBackend()
{
}

void PropertyEditorQmlBackend::setupPropertyEditorValue(const PropertyName &name, PropertyEditorView *propertyEditor, const QString &type)
{
    QmlDesigner::PropertyName propertyName(name);
    propertyName.replace('.', '_');
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(backendValuesPropertyMap().value(QString::fromUtf8(propertyName))));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(&backendValuesPropertyMap());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        QObject::connect(valueObject, &PropertyEditorValue::expressionChanged, propertyEditor, &PropertyEditorView::changeExpression);
        backendValuesPropertyMap().insert(QString::fromUtf8(propertyName), QVariant::fromValue(valueObject));
    }
    valueObject->setName(propertyName);
    if (type == QLatin1String("QColor"))
        valueObject->setValue(QVariant(QLatin1String("#000000")));
    else
        valueObject->setValue(QVariant(1));

}

static PropertyNameList layoutAttachedPropertiesNames()
{
    PropertyNameList propertyNames;
    propertyNames << "alignment" << "column" << "columnSpan" << "fillHeight" << "fillWidth"
                  << "maximumHeight" << "maximumWidth" << "minimumHeight" << "minimumWidth"
                  << "preferredHeight" << "preferredWidth" << "row"<< "rowSpan";

    return propertyNames;
}

QVariant properDefaultLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, const PropertyName &propertyName)
{
    QVariant value = qmlObjectNode.modelValue(propertyName);

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

    return QVariant();
}

void PropertyEditorQmlBackend::setupLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor)
{
    if (QmlItemNode(qmlObjectNode).isInLayout()) {

        static const PropertyNameList propertyNames = layoutAttachedPropertiesNames();

        foreach (const PropertyName &propertyName, propertyNames) {
            createPropertyEditorValue(qmlObjectNode, "Layout." + propertyName, properDefaultLayoutAttachedProperties(qmlObjectNode, propertyName), propertyEditor);
        }
    }
}

void PropertyEditorQmlBackend::createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                             const PropertyName &name,
                                             const QVariant &value,
                                             PropertyEditorView *propertyEditor)
{
    PropertyName propertyName(name);
    propertyName.replace('.', '_');
    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(backendValuesPropertyMap().value(QString::fromUtf8(propertyName))));
    if (!valueObject) {
        valueObject = new PropertyEditorValue(&backendValuesPropertyMap());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        QObject::connect(valueObject, &PropertyEditorValue::expressionChanged, propertyEditor, &PropertyEditorView::changeExpression);
        QObject::connect(valueObject, &PropertyEditorValue::exportPopertyAsAliasRequested, propertyEditor, &PropertyEditorView::exportPopertyAsAlias);
        QObject::connect(valueObject, &PropertyEditorValue::removeAliasExportRequested, propertyEditor, &PropertyEditorView::removeAliasExport);
        backendValuesPropertyMap().insert(QString::fromUtf8(propertyName), QVariant::fromValue(valueObject));
    }
    valueObject->setName(name);
    valueObject->setModelNode(qmlObjectNode);

    if (qmlObjectNode.propertyAffectedByCurrentState(name) && !(qmlObjectNode.modelNode().property(name).isBindingProperty()))
        valueObject->setValue(qmlObjectNode.modelValue(name));

    else
        valueObject->setValue(value);

    if (propertyName != "id" &&
        qmlObjectNode.currentState().isBaseState() &&
        qmlObjectNode.modelNode().property(propertyName).isBindingProperty()) {
        valueObject->setExpression(qmlObjectNode.modelNode().bindingProperty(propertyName).expression());
    } else {
        if (qmlObjectNode.hasBindingProperty(name))
            valueObject->setExpression(qmlObjectNode.expression(name));
        else
            valueObject->setExpression(qmlObjectNode.instanceValue(name).toString());
    }
}

void PropertyEditorQmlBackend::setValue(const QmlObjectNode & qmlObjectNode, const PropertyName &name, const QVariant &value)
{
    PropertyName propertyName = name;
    propertyName.replace('.', '_');
    PropertyEditorValue *propertyValue = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(propertyName))));
    if (propertyValue) {
        propertyValue->setValue(value);

        if (!qmlObjectNode.hasBindingProperty(name))
            propertyValue->setExpression(value.toString());
        else
            propertyValue->setExpression(qmlObjectNode.expression(name));
    }
}

QQmlContext *PropertyEditorQmlBackend::context() {
    return m_view->rootContext();
}

PropertyEditorContextObject* PropertyEditorQmlBackend::contextObject() {
    return m_contextObject.data();
}

QWidget *PropertyEditorQmlBackend::widget() {
    return m_view;
}

void PropertyEditorQmlBackend::setSource(const QUrl& url) {
    m_view->setSource(url);
}

Internal::QmlAnchorBindingProxy &PropertyEditorQmlBackend::backendAnchorBinding() {
    return m_backendAnchorBinding;
}

DesignerPropertyMap &PropertyEditorQmlBackend::backendValuesPropertyMap() {
    return m_backendValuesPropertyMap;
}

PropertyEditorTransaction *PropertyEditorQmlBackend::propertyEditorTransaction() {
    return m_propertyEditorTransaction.data();
}

PropertyEditorValue *PropertyEditorQmlBackend::propertyValueForName(const QString &propertyName)
{
     return qobject_cast<PropertyEditorValue*>(variantToQObject(backendValuesPropertyMap().value(propertyName)));
}

void PropertyEditorQmlBackend::setup(const QmlObjectNode &qmlObjectNode, const QString &stateName, const QUrl &qmlSpecificsFile, PropertyEditorView *propertyEditor)
{
    if (!qmlObjectNode.isValid())
        return;

    if (qmlObjectNode.isValid()) {

        qCInfo(propertyEditorBenchmark) << Q_FUNC_INFO;

        QTime time;
        if (propertyEditorBenchmark().isInfoEnabled())
            time.start();

        foreach (const PropertyName &propertyName, qmlObjectNode.modelNode().metaInfo().propertyNames())
            createPropertyEditorValue(qmlObjectNode, propertyName, qmlObjectNode.instanceValue(propertyName), propertyEditor);

        setupLayoutAttachedProperties(qmlObjectNode, propertyEditor);

        // className
        PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("className"))));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("className");
        valueObject->setModelNode(qmlObjectNode.modelNode());
        valueObject->setValue(qmlObjectNode.modelNode().simplifiedTypeName());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        m_backendValuesPropertyMap.insert(QLatin1String("className"), QVariant::fromValue(valueObject));

        // id
        valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("id"))));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("id");
        valueObject->setValue(qmlObjectNode.id());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        m_backendValuesPropertyMap.insert(QLatin1String("id"), QVariant::fromValue(valueObject));

        QmlItemNode itemNode(qmlObjectNode.modelNode());

        // anchors
        m_backendAnchorBinding.setup(qmlObjectNode.modelNode());
        context()->setContextProperty(QLatin1String("anchorBackend"), &m_backendAnchorBinding);

        context()->setContextProperty(QLatin1String("transaction"), m_propertyEditorTransaction.data());

        qCInfo(propertyEditorBenchmark) << "anchors:" << time.elapsed();

        // model node
        m_backendModelNode.setup(qmlObjectNode.modelNode());
        context()->setContextProperty(QLatin1String("modelNodeBackend"), &m_backendModelNode);

        qCInfo(propertyEditorBenchmark) << "context:" << time.elapsed();

        contextObject()->setSpecificsUrl(qmlSpecificsFile);

        qCInfo(propertyEditorBenchmark) << "specifics:" << time.elapsed();

        contextObject()->setStateName(stateName);
        if (!qmlObjectNode.isValid())
            return;
        context()->setContextProperty(QLatin1String("propertyCount"), QVariant(qmlObjectNode.modelNode().properties().count()));

        contextObject()->setIsBaseState(qmlObjectNode.isInBaseState());

        contextObject()->setHasAliasExport(qmlObjectNode.isAliasExported());
        contextObject()->setSelectionChanged(false);

        contextObject()->setSelectionChanged(false);

        NodeMetaInfo metaInfo = qmlObjectNode.modelNode().metaInfo();

        if (metaInfo.isValid()) {
            contextObject()->setMajorVersion(metaInfo.majorVersion());
            contextObject()->setMinorVersion(metaInfo.minorVersion());
        } else {
            contextObject()->setMajorVersion(-1);
            contextObject()->setMinorVersion(-1);
            contextObject()->setMajorQtQuickVersion(-1);
            contextObject()->setMinorQtQuickVersion(-1);
        }

        contextObject()->setMajorQtQuickVersion(qmlObjectNode.view()->majorQtQuickVersion());
        contextObject()->setMinorQtQuickVersion(qmlObjectNode.view()->minorQtQuickVersion());

        qCInfo(propertyEditorBenchmark) << "final:" << time.elapsed();
    } else {
        qWarning() << "PropertyEditor: invalid node for setup";
    }
}

void PropertyEditorQmlBackend::initialSetup(const TypeName &typeName, const QUrl &qmlSpecificsFile, PropertyEditorView *propertyEditor)
{
    NodeMetaInfo metaInfo = propertyEditor->model()->metaInfo(typeName);

    foreach (const PropertyName &propertyName, metaInfo.propertyNames())
        setupPropertyEditorValue(propertyName, propertyEditor, QString::fromUtf8(metaInfo.propertyTypeName(propertyName)));

    PropertyEditorValue *valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("className"))));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("className");

    valueObject->setValue(typeName);
    QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
    m_backendValuesPropertyMap.insert(QLatin1String("className"), QVariant::fromValue(valueObject));

    // id
    valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("id"))));
    if (!valueObject)
        valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
    valueObject->setName("id");
    valueObject->setValue("id");
    QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
    m_backendValuesPropertyMap.insert(QLatin1String("id"), QVariant::fromValue(valueObject));

    context()->setContextProperty(QLatin1String("anchorBackend"), &m_backendAnchorBinding);
    context()->setContextProperty(QLatin1String("modelNodeBackend"), &m_backendModelNode);
    context()->setContextProperty(QLatin1String("transaction"), m_propertyEditorTransaction.data());

    contextObject()->setSpecificsUrl(qmlSpecificsFile);

    contextObject()->setStateName(QStringLiteral("basestate"));

    contextObject()->setIsBaseState(true);

    contextObject()->setSpecificQmlData(QStringLiteral(""));

    contextObject()->setGlobalBaseUrl(QUrl());
}

QString PropertyEditorQmlBackend::propertyEditorResourcesPath() {
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/propertyEditorQmlSources");
}

QString PropertyEditorQmlBackend::templateGeneration(NodeMetaInfo type,
                                                     NodeMetaInfo superType,
                                                     const QmlObjectNode &objectNode)
{
    if (!templateConfiguration() || !templateConfiguration()->isValid())
        return QString();

    QStringList imports = variantToStringList(templateConfiguration()->property(QStringLiteral("imports")));

    QString qmlTemplate = imports.join(QLatin1Char('\n')) + QLatin1Char('\n');
    qmlTemplate += QStringLiteral("Section {\n");
    qmlTemplate += QStringLiteral("caption: \"%1\"\n").arg(objectNode.modelNode().simplifiedTypeName());
    qmlTemplate += QStringLiteral("SectionLayout {\n");

    QList<PropertyName> orderedList = type.propertyNames();
    Utils::sort(orderedList);

    bool emptyTemplate = true;

    foreach (const PropertyName &name, orderedList) {

        if (name.startsWith("__"))
            continue; //private API
        PropertyName properName = name;

        properName.replace('.', '_');

        TypeName typeName = type.propertyTypeName(name);
        //alias resolution only possible with instance
        if (typeName == "alias" && objectNode.isValid())
            typeName = objectNode.instanceType(name);

        if (!superType.hasProperty(name) && type.propertyIsWritable(name) && !name.contains(".")) {
            foreach (const QmlJS::SimpleReaderNode::Ptr &node, templateConfiguration()->children())
                if (variantToStringList(node->property(QStringLiteral("typeNames"))).contains(QString::fromLatin1(typeName))) {
                    const QString fileName = propertyTemplatesPath() + node->property(QStringLiteral("sourceFile")).toString();
                    QFile file(fileName);
                    if (file.open(QIODevice::ReadOnly)) {
                        QString source = QString::fromUtf8(file.readAll());
                        file.close();
                        qmlTemplate += source.arg(QString::fromUtf8(name)).arg(QString::fromUtf8(properName));
                        emptyTemplate = false;
                    } else {
                        qWarning().nospace() << "template definition source file not found:" << fileName;
                    }
                }
        }
    }
    qmlTemplate += QStringLiteral("}\n"); //Section
    qmlTemplate += QStringLiteral("}\n"); //SectionLayout

    if (emptyTemplate)
        return QString();

    return qmlTemplate;
}

QUrl PropertyEditorQmlBackend::getQmlFileUrl(const TypeName &relativeTypeName, const NodeMetaInfo &info)
{
    return fileToUrl(locateQmlFile(info, QString::fromUtf8(fixTypeNameForPanes(relativeTypeName) + ".qml")));
}

TypeName PropertyEditorQmlBackend::fixTypeNameForPanes(const TypeName &typeName)
{
    TypeName fixedTypeName = typeName;
    fixedTypeName.replace('.', '/');
    return fixedTypeName;
}

TypeName PropertyEditorQmlBackend::qmlFileName(const NodeMetaInfo &nodeInfo)
{
    const TypeName fixedTypeName = fixTypeNameForPanes(nodeInfo.typeName());
    return fixedTypeName + "Pane.qml";
}

QUrl PropertyEditorQmlBackend::fileToUrl(const QString &filePath)  {
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

QString PropertyEditorQmlBackend::fileFromUrl(const QUrl &url)
{
    if (url.scheme() == QStringLiteral("qrc")) {
        const QString &path = url.path();
        return QStringLiteral(":") + path;
    }

    return url.toLocalFile();
}

bool PropertyEditorQmlBackend::checkIfUrlExists(const QUrl &url)
{
    return QFileInfo::exists(fileFromUrl(url));
}

void PropertyEditorQmlBackend::emitSelectionToBeChanged()
{
    m_backendModelNode.emitSelectionToBeChanged();
}

void PropertyEditorQmlBackend::emitSelectionChanged()
{
    m_backendModelNode.emitSelectionChanged();
}

void PropertyEditorQmlBackend::setValueforLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, const PropertyName &name)
{
    PropertyName propertyName = name;
    propertyName.replace("Layout.", "");
    setValue(qmlObjectNode,  name, properDefaultLayoutAttachedProperties(qmlObjectNode, propertyName));
}

QUrl PropertyEditorQmlBackend::getQmlUrlForModelNode(const ModelNode &modelNode, TypeName &className)
{
    if (modelNode.isValid()) {
        foreach (const NodeMetaInfo &info, modelNode.metaInfo().classHierarchy()) {
            QUrl fileUrl = fileToUrl(locateQmlFile(info, QString::fromUtf8(qmlFileName(info))));
            if (fileUrl.isValid()) {
                className = info.typeName();
                return fileUrl;
            }
        }
    }
    return fileToUrl(QDir(propertyEditorResourcesPath()).filePath(QLatin1String("QtQuick/emptyPane.qml")));
}

QString PropertyEditorQmlBackend::locateQmlFile(const NodeMetaInfo &info, const QString &relativePath)
{
    static const QDir fileSystemDir(PropertyEditorQmlBackend::propertyEditorResourcesPath());

    const QDir resourcesDir(QStringLiteral(":/propertyEditorQmlSources"));
    const QDir importDir(info.importDirectoryPath() + Constants::QML_DESIGNER_SUBFOLDER);
    const QDir importDirVersion(info.importDirectoryPath() + QStringLiteral(".") + QString::number(info.majorVersion()) + Constants::QML_DESIGNER_SUBFOLDER);

    const QString relativePathWithoutEnding = relativePath.left(relativePath.count() - 4);
    const QString relativePathWithVersion = QString("%1_%2_%3.qml").arg(relativePathWithoutEnding
        ).arg(info.majorVersion()).arg(info.minorVersion());

    //Check for qml files with versions first

    const QString withoutDir = relativePath.split(QStringLiteral("/")).last();

    if (importDirVersion.exists(withoutDir))
        return importDirVersion.absoluteFilePath(withoutDir);

    const QString withoutDirWithVersion = relativePathWithVersion.split(QStringLiteral("/")).last();

    const QStringList possiblePaths = {
        importDir.absoluteFilePath(relativePathWithVersion),
        //Since we are in a subfolder of the import we do not require the directory
        importDir.absoluteFilePath(withoutDirWithVersion),
        fileSystemDir.absoluteFilePath(relativePathWithVersion),
        resourcesDir.absoluteFilePath(relativePathWithVersion),

        importDir.absoluteFilePath(relativePath),
        //Since we are in a subfolder of the import we do not require the directory
        importDir.absoluteFilePath(withoutDir),
        fileSystemDir.absoluteFilePath(relativePath),
        resourcesDir.absoluteFilePath(relativePath)
    };
    return Utils::findOrDefault(possiblePaths, [](const QString &possibleFilePath) {
        return QFile::exists(possibleFilePath);
    });
}


} //QmlDesigner
