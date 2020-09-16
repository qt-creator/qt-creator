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
#include <qmltimeline.h>

#include <qmlobjectnode.h>
#include <nodemetainfo.h>
#include <variantproperty.h>
#include <bindingproperty.h>

#include <theme.h>

#include <coreplugin/icore.h>
#include <qmljs/qmljssimplereader.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QVector3D>
#include <QVector2D>

#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(propertyEditorBenchmark, "qtc.propertyeditor.load", QtWarningMsg)

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

    return nullptr;
}

namespace QmlDesigner {

PropertyEditorQmlBackend::PropertyEditorQmlBackend(PropertyEditorView *propertyEditor) :
        m_view(new Quick2PropertyEditorView), m_propertyEditorTransaction(new PropertyEditorTransaction(propertyEditor)), m_dummyPropertyEditorValue(new PropertyEditorValue()),
        m_contextObject(new PropertyEditorContextObject())
{
    m_view->engine()->setOutputWarningsToStandardError(QmlDesignerPlugin::instance()
        ->settings().value(DesignerSettingsKey::SHOW_PROPERTYEDITOR_WARNINGS).toBool());

    m_view->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_dummyPropertyEditorValue->setValue(QLatin1String("#000000"));
    context()->setContextProperty(QLatin1String("dummyBackendValue"), m_dummyPropertyEditorValue.data());
    m_contextObject->setBackendValues(&m_backendValuesPropertyMap);
    m_contextObject->setModel(propertyEditor->model());
    m_contextObject->insertInQmlContext(context());

    QObject::connect(&m_backendValuesPropertyMap, &DesignerPropertyMap::valueChanged,
                     propertyEditor, &PropertyEditorView::changeValue);
}

PropertyEditorQmlBackend::~PropertyEditorQmlBackend() = default;

void PropertyEditorQmlBackend::setupPropertyEditorValue(const PropertyName &name, PropertyEditorView *propertyEditor, const QString &type)
{
    QmlDesigner::PropertyName propertyName(name);
    propertyName.replace('.', '_');
    auto valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(backendValuesPropertyMap().value(QString::fromUtf8(propertyName))));
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

PropertyName auxNamePostFix(const PropertyName &propertyName)
{
    return propertyName + "__AUX";
}

QVariant properDefaultAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                          const PropertyName &propertyName)
{
    const ModelNode node = qmlObjectNode.modelNode();
    const PropertyName auxName = propertyName;

    if (node.hasAuxiliaryData(auxName))
        return node.auxiliaryData(auxName);
    if (propertyName == "transitionColor")
        return QColor(Qt::red);
    if (propertyName == "areaColor")
        return QColor(Qt::red);
    if (propertyName == "blockColor")
        return QColor(Qt::red);
    if (propertyName == "areaFillColor")
        return QColor(Qt::transparent);
    else if (propertyName == "color")
        return QColor(Qt::red);
    else if (propertyName == "fillColor")
        return QColor(Qt::transparent);
    else if (propertyName == "width")
        return 4;
    else if (propertyName == "dash")
        return false;
    else if (propertyName == "inOffset")
        return 0;
    else if (propertyName == "outOffset")
        return 0;
    else if (propertyName == "breakPoint")
        return 50;
    else if (propertyName == "transitionType")
        return 0;
    else if (propertyName == "type")
        return 0;
    else if (propertyName == "transitionRadius")
        return 8;
    else if (propertyName == "radius")
        return 8;
    else if (propertyName == "transitionBezier")
        return 50;
    else if (propertyName == "bezier")
        return 50;
    else if (propertyName == "labelPosition")
        return 50.0;
    else if (propertyName == "labelFlipSide")
        return false;
    else if (propertyName == "customId")
        return QString();
    else if (propertyName == "joinConnection")
        return false;
    else if (propertyName == "blockSize")
        return 200;
    else if (propertyName == "blockRadius")
        return 18;
    else if (propertyName == "showDialogLabel")
        return false;
    else if (propertyName == "dialogLabelPosition")
        return Qt::TopRightCorner;

    return {};
}

QVariant properDefaultLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode,
                                               const PropertyName &propertyName)
{
    const QVariant value = qmlObjectNode.modelValue("Layout." + propertyName);
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

void PropertyEditorQmlBackend::setupLayoutAttachedProperties(const QmlObjectNode &qmlObjectNode, PropertyEditorView *propertyEditor)
{
    if (QmlItemNode(qmlObjectNode).isInLayout()) {

        static const PropertyNameList propertyNames =
            {"alignment", "column", "columnSpan", "fillHeight", "fillWidth", "maximumHeight", "maximumWidth",
                "minimumHeight", "minimumWidth", "preferredHeight", "preferredWidth", "row", "rowSpan",
                "topMargin", "bottomMargin", "leftMargin", "rightMargin", "margins"};

        for (const PropertyName &propertyName : propertyNames) {
            createPropertyEditorValue(qmlObjectNode, "Layout." + propertyName, properDefaultLayoutAttachedProperties(qmlObjectNode, propertyName), propertyEditor);
        }
    }
}

void PropertyEditorQmlBackend::setupAuxiliaryProperties(const QmlObjectNode &qmlObjectNode,
                                                        PropertyEditorView *propertyEditor)
{

    const QmlItemNode itemNode(qmlObjectNode);

    PropertyNameList propertyNames;

    propertyNames.append("customId");

    if (itemNode.isFlowTransition()) {
        propertyNames.append({"color", "width", "inOffset", "outOffset", "dash", "breakPoint", "type", "radius", "bezier", "labelPosition", "labelFlipSide"});
    } else if (itemNode.isFlowItem()) {
        propertyNames.append({"color", "width", "inOffset", "outOffset", "joinConnection"});
    } else if (itemNode.isFlowActionArea()) {
        propertyNames.append({"color", "width", "fillColor", "outOffset", "dash"});
    } else if (itemNode.isFlowDecision()) {
        propertyNames.append({"color", "width", "fillColor", "dash", "blockSize", "blockRadius", "showDialogLabel", "dialogLabelPosition"});
    } else if (itemNode.isFlowWildcard()) {
        propertyNames.append({"color", "width", "fillColor", "dash", "blockSize", "blockRadius"});
    } else if (itemNode.isFlowView()) {
        propertyNames.append({"transitionColor", "areaColor", "areaFillColor", "blockColor", "transitionType", "transitionRadius", "transitionBezier"});
    }

    for (const PropertyName &propertyName : propertyNames) {
        createPropertyEditorValue(qmlObjectNode, auxNamePostFix(propertyName),
                                  properDefaultAuxiliaryProperties(qmlObjectNode, propertyName), propertyEditor);
    }

}

void PropertyEditorQmlBackend::createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                             const PropertyName &name,
                                             const QVariant &value,
                                             PropertyEditorView *propertyEditor)
{
    PropertyName propertyName(name);
    propertyName.replace('.', '_');
    auto valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(backendValuesPropertyMap().value(QString::fromUtf8(propertyName))));
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

void PropertyEditorQmlBackend::setValue(const QmlObjectNode & , const PropertyName &name, const QVariant &value)
{
    // Vector*D values need to be split into their subcomponents
    if (value.type() == QVariant::Vector2D) {
        const char *suffix[2] = {"_x", "_y"};
        auto vecValue = value.value<QVector2D>();
        for (int i = 0; i < 2; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(subPropName))));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else if (value.type() == QVariant::Vector3D) {
        const char *suffix[3] = {"_x", "_y", "_z"};
        auto vecValue = value.value<QVector3D>();
        for (int i = 0; i < 3; ++i) {
            PropertyName subPropName(name.size() + 2, '\0');
            subPropName.replace(0, name.size(), name);
            subPropName.replace(name.size(), 2, suffix[i]);
            auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(subPropName))));
            if (propertyValue)
                propertyValue->setValue(QVariant(vecValue[i]));
        }
    } else {
        PropertyName propertyName = name;
        propertyName.replace('.', '_');
        auto propertyValue = qobject_cast<PropertyEditorValue *>(variantToQObject(m_backendValuesPropertyMap.value(QString::fromUtf8(propertyName))));
        if (propertyValue)
            propertyValue->setValue(value);
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
    if (qmlObjectNode.isValid()) {

        m_contextObject->setModel(propertyEditor->model());

        qCInfo(propertyEditorBenchmark) << Q_FUNC_INFO;

        QElapsedTimer time;
        if (propertyEditorBenchmark().isInfoEnabled())
            time.start();

        foreach (const PropertyName &propertyName, qmlObjectNode.modelNode().metaInfo().propertyNames())
            createPropertyEditorValue(qmlObjectNode, propertyName, qmlObjectNode.instanceValue(propertyName), propertyEditor);

        setupLayoutAttachedProperties(qmlObjectNode, propertyEditor);
        setupAuxiliaryProperties(qmlObjectNode, propertyEditor);

        // model node
        m_backendModelNode.setup(qmlObjectNode.modelNode());
        context()->setContextProperty(QLatin1String("modelNodeBackend"), &m_backendModelNode);

        // className
        auto valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("className"))));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("className");
        valueObject->setModelNode(qmlObjectNode.modelNode());
        valueObject->setValue(m_backendModelNode.simplifiedTypeName());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        m_backendValuesPropertyMap.insert(QLatin1String("className"), QVariant::fromValue(valueObject));

        // id
        valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("id"))));
        if (!valueObject)
            valueObject = new PropertyEditorValue(&m_backendValuesPropertyMap);
        valueObject->setName("id");
        valueObject->setValue(m_backendModelNode.nodeId());
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, &backendValuesPropertyMap(), &DesignerPropertyMap::valueChanged);
        m_backendValuesPropertyMap.insert(QLatin1String("id"), QVariant::fromValue(valueObject));

        QmlItemNode itemNode(qmlObjectNode.modelNode());

        // anchors
        m_backendAnchorBinding.setup(qmlObjectNode.modelNode());
        context()->setContextProperties(
            QVector<QQmlContext::PropertyPair>{
                {{"anchorBackend"}, QVariant::fromValue(&m_backendAnchorBinding)},
                {{"transaction"}, QVariant::fromValue(m_propertyEditorTransaction.data())}
            }
        );

        qCInfo(propertyEditorBenchmark) << "anchors:" << time.elapsed();

        qCInfo(propertyEditorBenchmark) << "context:" << time.elapsed();

        contextObject()->setSpecificsUrl(qmlSpecificsFile);

        qCInfo(propertyEditorBenchmark) << "specifics:" << time.elapsed();

        contextObject()->setStateName(stateName);
        if (!qmlObjectNode.isValid())
            return;

        context()->setContextProperty(QLatin1String("propertyCount"), QVariant(qmlObjectNode.modelNode().properties().count()));

        QStringList stateNames = qmlObjectNode.allStateNames();
        stateNames.prepend("base state");
        contextObject()->setAllStateNames(stateNames);

        contextObject()->setIsBaseState(qmlObjectNode.isInBaseState());

        contextObject()->setHasAliasExport(qmlObjectNode.isAliasExported());

        contextObject()->setHasActiveTimeline(QmlTimeline::hasActiveTimeline(qmlObjectNode.view()));

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

    auto valueObject = qobject_cast<PropertyEditorValue*>(variantToQObject(m_backendValuesPropertyMap.value(QLatin1String("className"))));
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

    context()->setContextProperties(
        QVector<QQmlContext::PropertyPair>{
            {{"anchorBackend"}, QVariant::fromValue(&m_backendAnchorBinding)},
            {{"modelNodeBackend"}, QVariant::fromValue(&m_backendModelNode)},
            {{"transaction"}, QVariant::fromValue(m_propertyEditorTransaction.data())}
        }
    );

    contextObject()->setSpecificsUrl(qmlSpecificsFile);

    contextObject()->setStateName(QStringLiteral("basestate"));

    contextObject()->setIsBaseState(true);

    contextObject()->setSpecificQmlData(QStringLiteral(""));

    contextObject()->setGlobalBaseUrl(QUrl());
}

QString PropertyEditorQmlBackend::propertyEditorResourcesPath() {
    return Core::ICore::resourcePath() + QStringLiteral("/qmldesigner/propertyEditorQmlSources");
}

inline bool dotPropertyHeuristic(const QmlObjectNode &node, const NodeMetaInfo &type, const PropertyName &name)
{
    if (!name.contains("."))
        return true;

    if (name.count('.') > 1)
        return false;

    QList<QByteArray> list =name.split('.');
    const PropertyName parentProperty = list.first();
    const PropertyName itemProperty = list.last();

    TypeName typeName = type.propertyTypeName(parentProperty);

    NodeMetaInfo itemInfo = node.view()->model()->metaInfo("QtQuick.Item");
    NodeMetaInfo textInfo = node.view()->model()->metaInfo("QtQuick.Text");
    NodeMetaInfo rectangleInfo = node.view()->model()->metaInfo("QtQuick.Rectangle");

    if (itemInfo.hasProperty(itemProperty))
        return false;

    if (typeName == "font")
        return false;

    if (textInfo.isSubclassOf(typeName))
            return false;

    if (rectangleInfo.isSubclassOf(typeName))
            return false;

    return true;
}

QString PropertyEditorQmlBackend::templateGeneration(const NodeMetaInfo &type,
                                                     const NodeMetaInfo &superType,
                                                     const QmlObjectNode &node)
{
    if (!templateConfiguration() || !templateConfiguration()->isValid())
        return QString();

    const auto nodes = templateConfiguration()->children();

    QStringList sectorTypes;

    for (const QmlJS::SimpleReaderNode::Ptr &node : nodes) {
        if (node->propertyNames().contains("separateSection"))
            sectorTypes.append(variantToStringList(node->property("typeNames")));
    }

    QStringList imports = variantToStringList(templateConfiguration()->property(QStringLiteral("imports")));

    QString qmlTemplate = imports.join(QLatin1Char('\n')) + QLatin1Char('\n');

    qmlTemplate += "Column {\n";
    qmlTemplate += "anchors.left: parent.left\n";
    qmlTemplate += "anchors.right: parent.right\n";

    QList<PropertyName> orderedList = type.propertyNames();
    Utils::sort(orderedList, [type, &sectorTypes](const PropertyName &left, const PropertyName &right){
        const QString typeNameLeft = QString::fromLatin1(type.propertyTypeName(left));
        const QString typeNameRight = QString::fromLatin1(type.propertyTypeName(right));
        if (typeNameLeft == typeNameRight)
            return left > right;

        if (sectorTypes.contains(typeNameLeft)) {
            if (sectorTypes.contains(typeNameRight))
                return left > right;
            return true;
        } else if (sectorTypes.contains(typeNameRight)) {
            return false;
        }
        return left > right;
    });

    bool emptyTemplate = true;

    bool sectionStarted = false;

    foreach (const PropertyName &name, orderedList) {

        if (name.startsWith("__"))
            continue; //private API
        PropertyName properName = name;

        properName.replace('.', '_');

        TypeName typeName = type.propertyTypeName(name);
        //alias resolution only possible with instance
        if (typeName == "alias" && node.isValid())
            typeName = node.instanceType(name);

        auto nodes = templateConfiguration()->children();

        if (!superType.hasProperty(name) && type.propertyIsWritable(name) && dotPropertyHeuristic(node, type, name)) {

            for (const QmlJS::SimpleReaderNode::Ptr &node : nodes) {
                if (variantToStringList(node->property(QStringLiteral("typeNames"))).contains(QString::fromLatin1(typeName))) {
                    const QString fileName = propertyTemplatesPath() + node->property(QStringLiteral("sourceFile")).toString();
                    QFile file(fileName);
                    if (file.open(QIODevice::ReadOnly)) {
                        QString source = QString::fromUtf8(file.readAll());
                        file.close();
                        const bool section = node->propertyNames().contains("separateSection");
                        if (section) {
                        } else if (!sectionStarted) {
                            qmlTemplate += QStringLiteral("Section {\n");
                            qmlTemplate += QStringLiteral("caption: \"%1\"\n").arg(QString::fromUtf8(type.simplifiedTypeName()));
                            qmlTemplate += "anchors.left: parent.left\n";
                            qmlTemplate += "anchors.right: parent.right\n";
                            qmlTemplate += QStringLiteral("SectionLayout {\n");
                            sectionStarted = true;
                        }

                        qmlTemplate += source.arg(QString::fromUtf8(name)).arg(QString::fromUtf8(properName));
                        emptyTemplate = false;
                    } else {
                        qWarning().nospace() << "template definition source file not found:" << fileName;
                    }
                }
            }
        }
    }
    if (sectionStarted) {
        qmlTemplate += QStringLiteral("}\n"); //Section
        qmlTemplate += QStringLiteral("}\n"); //SectionLayout
    }

    qmlTemplate += "}\n";

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

static NodeMetaInfo findCommonSuperClass(const NodeMetaInfo &first, const NodeMetaInfo &second)
{
    for (const NodeMetaInfo &info : first.superClasses()) {
        if (second.isSubclassOf(info.typeName()))
            return info;
    }
    return first;
}

NodeMetaInfo PropertyEditorQmlBackend::findCommonAncestor(const ModelNode &node)
{
    if (!node.isValid())
        return {};

    QTC_ASSERT(node.metaInfo().isValid(), return {});

    AbstractView *view = node.view();

    if (view->selectedModelNodes().count() > 1) {
        NodeMetaInfo commonClass = node.metaInfo();
        for (const ModelNode &currentNode :  view->selectedModelNodes()) {
            if (currentNode.metaInfo().isValid() && !currentNode.isSubclassOf(commonClass.typeName(), -1, -1))
              commonClass = findCommonSuperClass(currentNode.metaInfo(), commonClass);
        }
        return commonClass;
    }

    return node.metaInfo();
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
    const QString &file = fileFromUrl(url);
    return !file.isEmpty() && QFileInfo::exists(file);
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
    setValue(qmlObjectNode, name, properDefaultLayoutAttachedProperties(qmlObjectNode, propertyName));

    if (propertyName == "margins") {
        const QVariant marginsValue = properDefaultLayoutAttachedProperties(qmlObjectNode, "margins");
        setValue(qmlObjectNode, "Layout.topMargin", marginsValue);
        setValue(qmlObjectNode, "Layout.bottomMargin", marginsValue);
        setValue(qmlObjectNode, "Layout.leftMargin", marginsValue);
        setValue(qmlObjectNode, "Layout.rightMargin", marginsValue);
    }
}

void PropertyEditorQmlBackend::setValueforAuxiliaryProperties(const QmlObjectNode &qmlObjectNode, const PropertyName &name)
{
    const PropertyName propertyName = auxNamePostFix(name);
     setValue(qmlObjectNode, propertyName, qmlObjectNode.modelNode().auxiliaryData(name));
}

QUrl PropertyEditorQmlBackend::getQmlUrlForMetaInfo(const NodeMetaInfo &metaInfo, TypeName &className)
{
    if (metaInfo.isValid()) {
        foreach (const NodeMetaInfo &info, metaInfo.classHierarchy()) {
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

    const QString withoutDir = relativePath.split(QStringLiteral("/")).constLast();

    if (importDirVersion.exists(withoutDir))
        return importDirVersion.absoluteFilePath(withoutDir);

    const QString withoutDirWithVersion = relativePathWithVersion.split(QStringLiteral("/")).constLast();

    QStringList possiblePaths = {
        fileSystemDir.absoluteFilePath(relativePathWithVersion),
        resourcesDir.absoluteFilePath(relativePathWithVersion),
        fileSystemDir.absoluteFilePath(relativePath),
        resourcesDir.absoluteFilePath(relativePath)
    };

    if (!importDir.isEmpty())
        possiblePaths.append({
            importDir.absoluteFilePath(relativePathWithVersion),
            //Since we are in a subfolder of the import we do not require the directory
            importDir.absoluteFilePath(withoutDirWithVersion),
            importDir.absoluteFilePath(relativePath),
            //Since we are in a subfolder of the import we do not require the directory
            importDir.absoluteFilePath(withoutDir),
        });

    return Utils::findOrDefault(possiblePaths, [](const QString &possibleFilePath) {
        return QFileInfo::exists(possibleFilePath);
    });
}


} //QmlDesigner

