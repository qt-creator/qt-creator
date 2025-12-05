// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorvalue.h"

#include "propertyeditortracing.h"
#include "propertyeditorview.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <createtexture.h>
#include <designermcumanager.h>
#include <designmodewidget.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <qmlobjectnode.h>
#include <rewritertransaction.h>
#include <rewritingexception.h>

#include <enumeration.h>
#include <utils3d.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QUrl>

namespace QmlDesigner {

using QmlDesigner::PropertyEditorTracing::category;

namespace {

bool cleverDoubleCompare(const QVariant &value1, const QVariant &value2)
{
    if (value1.typeId() == QMetaType::Double && value2.typeId() == QMetaType::Double) {
        // ignore slight changes on doubles
        if (qFuzzyCompare(value1.toDouble(), value2.toDouble()))
            return true;
    }
    return false;
}

bool cleverColorCompare(const QVariant &value1, const QVariant &value2)
{
    if (value1.typeId() == QMetaType::QColor && value2.typeId() == QMetaType::QColor) {
        QColor c1 = value1.value<QColor>();
        QColor c2 = value2.value<QColor>();
        return c1.name() == c2.name() && c1.alpha() == c2.alpha();
    }

    if (value1.typeId() == QMetaType::QString && value2.typeId() == QMetaType::QColor)
        return cleverColorCompare(QVariant(QColor(value1.toString())), value2);

    if (value1.typeId() == QMetaType::QColor && value2.typeId() == QMetaType::QString)
        return cleverColorCompare(value1, QVariant(QColor(value2.toString())));

    return false;
}

// "red" is the same color as "#ff0000"
// To simplify editing we convert all explicit color names in the hash format
void fixAmbigousColorNames(const NodeMetaInfo &metaInfo, QVariant *value)
{
    if (metaInfo.isColor()) {
        if (value->typeId() == QMetaType::QColor) {
            QColor color = value->value<QColor>();
            int alpha = color.alpha();
            color = QColor(color.name());
            color.setAlpha(alpha);
            *value = color;
        } else if (value->toString() != QStringLiteral("transparent")) {
            *value = QColor(value->toString()).name(QColor::HexArgb);
        }
    }
}

void fixUrl(const NodeMetaInfo &metaInfo, QVariant *value)
{
    if (metaInfo.isUrl()) {
        if (!value->isValid())
            *value = QStringLiteral("");
    }
}

/* The comparison of variants is not symmetric because of implicit conversion.
 * QVariant(string) == QVariant(QColor) does for example ignore the alpha channel,
 * because the color is converted to a string ignoring the alpha channel.
 * By comparing the variants in both directions we gain a symmetric comparison.
 */
bool compareVariants(const QVariant &value1, const QVariant &value2)
{
    return value1 == value2 && value2 == value1;
}

bool modelHasNodePropertyWithSimilarName(const ModelNode &modelNode, PropertyNameView name)
{
    if (modelNode.hasProperty(name) && modelNode.property(name).isNodeProperty()) {
        const ModelNode nodePropertyModelNode = modelNode.nodeProperty(name).modelNode();
        return nodePropertyModelNode.isValid();
    }
    return false;
}

} // namespace

PropertyEditorValue::PropertyEditorValue(QObject *parent)
    : QObject(parent)
{
    NanotraceHR::Tracer tracer{"property editor value constructor", category()};
}

QVariant PropertyEditorValue::value() const
{
    NanotraceHR::Tracer tracer{"property editor value value", category()};

    if (m_propertyType.isUrl())
        return m_value.toUrl().toString();

    return m_value;
}

void PropertyEditorValue::setValueWithEmit(const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor value set value with emit", category()};

    if (!compareVariants(value, m_value) || isBound()) {
        QVariant newValue = value;
        if (m_propertyType.isUrl())
            newValue = QUrl(newValue.toString());

        if (cleverDoubleCompare(newValue, m_value) || cleverColorCompare(newValue, m_value))
            return;

        setValue(newValue);
        m_isBound = false;
        m_expression.clear();
        emit valueChanged(nameAsQString(), value);
        emit valueChangedQml();
        emit isBoundChanged();
        emit isExplicitChanged();
    }
}

void PropertyEditorValue::setValue(const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor value set value", category()};

    const bool colorsEqual = cleverColorCompare(value, m_value);

    if (!compareVariants(m_value, value) && !cleverDoubleCompare(value, m_value) && !colorsEqual)
        m_value = value;

    fixAmbigousColorNames(m_propertyType, &m_value);
    fixUrl(m_propertyType, &m_value);

    if (!colorsEqual)
        emit valueChangedQml();

    emit isExplicitChanged();
    emit isBoundChanged();
}

QString PropertyEditorValue::enumeration() const
{
    NanotraceHR::Tracer tracer{"property editor value enumeration", category()};

    return m_value.value<Enumeration>().nameToString();
}

QString PropertyEditorValue::expression() const
{
    NanotraceHR::Tracer tracer{"property editor value expression", category()};

    return m_expression;
}

void PropertyEditorValue::setExpressionWithEmit(const QString &expression)
{
    NanotraceHR::Tracer tracer{"property editor value set expression with emit", category()};

    if (m_expression != expression) {
        setExpression(expression);
        m_value.clear();
        emit expressionChanged(nameAsQString());
        emit expressionChangedQml();// Note that we set the name in this case
    }
    emit isBoundChanged();
}

void PropertyEditorValue::setExpression(const QString &expression)
{
    NanotraceHR::Tracer tracer{"property editor value set expression", category()};

    if (m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(QString());
    }
}

QString PropertyEditorValue::valueToString() const
{
    NanotraceHR::Tracer tracer{"property editor value value to string", category()};

    return value().toString();
}

bool PropertyEditorValue::isInSubState() const
{
    NanotraceHR::Tracer tracer{"property editor value is in sub state", category()};

    const QmlObjectNode objectNode(modelNode());
    return objectNode.isValid() && objectNode.currentState().isValid()
           && objectNode.propertyAffectedByCurrentState(name());
}

bool PropertyEditorValue::isBound() const
{
    NanotraceHR::Tracer tracer{"property editor value is bound", category()};

    const QmlObjectNode objectNode(modelNode());
    return m_forceBound || (objectNode.isValid() && objectNode.hasBindingProperty(name()));
}

bool PropertyEditorValue::isInModel() const
{
    NanotraceHR::Tracer tracer{"property editor value is in model", category()};

    return modelNode().hasProperty(name());
}

PropertyNameView PropertyEditorValue::name() const
{
    NanotraceHR::Tracer tracer{"property editor value name", category()};

    return m_name;
}

QString PropertyEditorValue::nameAsQString() const
{
    NanotraceHR::Tracer tracer{"property editor value name as QString", category()};

    return QString::fromUtf8(m_name);
}

bool PropertyEditorValue::isValid() const
{
    NanotraceHR::Tracer tracer{"property editor value is valid", category()};

    return m_isValid;
}

void PropertyEditorValue::setIsValid(bool valid)
{
    NanotraceHR::Tracer tracer{"property editor value set is valid", category()};

    m_isValid = valid;
}

bool PropertyEditorValue::isTranslated() const
{
    NanotraceHR::Tracer tracer{"property editor value is translated", category()};

    auto isDynamicString = [&] {
        return modelNode().property(name()).dynamicTypeName() == TypeNameView("string");
    };

    if (m_propertyType.isString() || isDynamicString()) {
        const QmlObjectNode objectNode(modelNode());
        if (objectNode.hasBindingProperty(name())) {
            const QRegularExpression rx(
                QRegularExpression::anchoredPattern("qsTr(|Id|anslate)\\(\".*\"\\)"));
            //qsTr()
            if (objectNode.propertyAffectedByCurrentState(name()))
                return m_expression.contains(rx);
            else
                return modelNode().bindingProperty(name()).expression().contains(rx);
        }
    }

    return false;
}

bool PropertyEditorValue::hasActiveDrag() const
{
    NanotraceHR::Tracer tracer{"property editor value has active drag", category()};

    return m_hasActiveDrag;
}

void PropertyEditorValue::setHasActiveDrag(bool val)
{
    NanotraceHR::Tracer tracer{"property editor value set has active drag", category()};

    if (m_hasActiveDrag != val) {
        m_hasActiveDrag = val;
        emit hasActiveDragChanged();
    }
}

static bool isAllowedSubclassType(const QString &type, const NodeMetaInfo &metaInfo, Model *model)
{
    if (!metaInfo.isValid())
        return false;

    auto base = model->metaInfo(type.toUtf8());

    return metaInfo.isBasedOn(base);
}

bool PropertyEditorValue::isAvailable() const
{
    NanotraceHR::Tracer tracer{"property editor value is available", category()};

    if (!m_modelNode.isValid())
        return true;

    const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();

    if (mcuManager.isMCUProject()) {
        const QSet<QString> nonMcuProperties = mcuManager.bannedProperties();
        const auto mcuAllowedItemProperties = mcuManager.allowedItemProperties();
        const auto mcuBannedComplexProperties = mcuManager.bannedComplexProperties();

        const QList<QByteArray> list = name().toByteArray().split('.');
        const QByteArray pureName = list.constFirst();
        const QString pureNameStr = QString::fromUtf8(pureName);

        const QByteArray ending = list.constLast();
        const QString endingStr = QString::fromUtf8(ending);

        //allowed item properties:
        const auto itemTypes = mcuAllowedItemProperties.keys();
        for (const auto &itemType : itemTypes) {
            if (isAllowedSubclassType(itemType, m_modelNode.metaInfo(), m_modelNode.model())) {
                const DesignerMcuManager::ItemProperties allowedItemProps =
                        mcuAllowedItemProperties.value(itemType);
                if (allowedItemProps.properties.contains(pureNameStr)) {
                    if (QmlItemNode::isValidQmlItemNode(m_modelNode)) {
                        const bool itemHasChildren = QmlItemNode(m_modelNode).hasChildren();

                        if (itemHasChildren)
                            return allowedItemProps.allowChildren;

                        return true;
                    }
                }
            }
        }

        // banned properties, with prefixes:
        if (mcuBannedComplexProperties.value(pureNameStr).contains(endingStr))
            return false;

        // general group:
        if (nonMcuProperties.contains(pureNameStr))
            return false;
    }

    return true;
}

ModelNode PropertyEditorValue::modelNode() const
{
    NanotraceHR::Tracer tracer{"property editor value model node", category()};

    return m_modelNode;
}

void PropertyEditorValue::setModelNodeAndProperty(const ModelNode &modelNode,
                                                  PropertyNameView name,
                                                  const PropertyMetaInfo &propertyMetaInfo)
{
    NanotraceHR::Tracer tracer{"property editor value set model node and name", category()};

    m_name = name;

    if (modelNode != m_modelNode) {
        m_modelNode = modelNode;

        bool isComplex = modelHasNodePropertyWithSimilarName(m_modelNode, name);
        if (m_isComplexNode != isComplex) {
            m_isComplexNode = isComplex;
            emit isComplexNodeChanged();
        }

        emit modelNodeChanged();
    }

    if (propertyMetaInfo != m_propertyMetaInfo) {
        m_propertyMetaInfo = propertyMetaInfo;
        m_propertyType = m_propertyMetaInfo.propertyType();
    }
}

bool PropertyEditorValue::isComplexNode() const
{
    return m_isComplexNode;
}

void PropertyEditorValue::resetValue()
{
    NanotraceHR::Tracer tracer{"property editor value reset value", category()};

    if (m_value.isValid() || !m_expression.isEmpty() || isBound()) {
        m_value = QVariant();
        m_isBound = false;
        m_expression = QString();
        emit valueChanged(nameAsQString(), QVariant());
        emit expressionChanged({});
        emit expressionChangedQml();
    }
}

void PropertyEditorValue::setEnumeration(const QString &scope, const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor value set enumeration", category()};

    Enumeration newEnumeration(scope.toUtf8(), name.toUtf8());

    setValueWithEmit(QVariant::fromValue(newEnumeration));
}

void PropertyEditorValue::exportPropertyAsAlias()
{
    NanotraceHR::Tracer tracer{"property editor value export property as alias", category()};

    emit exportPropertyAsAliasRequested(nameAsQString());
}

bool PropertyEditorValue::hasPropertyAlias() const
{
    NanotraceHR::Tracer tracer{"property editor value has property alias", category()};

    if (!modelNode().isValid())
        return false;

    if (modelNode().isRootNode())
        return false;

    if (!modelNode().hasId())
        return false;

    QString id = modelNode().id();

    const QList<BindingProperty> bindingProps = modelNode().view()->rootModelNode().bindingProperties();
    for (const BindingProperty &property : bindingProps) {
        if (property.expression() == (id + '.' + nameAsQString()))
            return true;
    }

    return false;
}

bool PropertyEditorValue::isAttachedProperty() const
{
    NanotraceHR::Tracer tracer{"property editor value is attached property", category()};

    return !nameAsQString().isEmpty() && nameAsQString().at(0).isUpper();
}

void PropertyEditorValue::removeAliasExport()
{
    NanotraceHR::Tracer tracer{"property editor value remove alias export", category()};

    emit removeAliasExportRequested(nameAsQString());
}

QString PropertyEditorValue::getTranslationContext() const
{
    NanotraceHR::Tracer tracer{"property editor value get translation context", category()};

    if (m_propertyType.isString()) {
        const QmlObjectNode objectNode(modelNode());
        if (objectNode.hasBindingProperty(name())) {
            const QRegularExpression rx(QRegularExpression::anchoredPattern(
                "qsTranslate\\(\"(.*)\"\\s*,\\s*\".*\"\\s*\\)"));
            const QRegularExpressionMatch match = rx.match(expression());
            if (match.hasMatch())
                return match.captured(1);
        }
    }

    return QString();
}

bool PropertyEditorValue::isIdList() const
{
    NanotraceHR::Tracer tracer{"property editor value is id list", category()};

    if (m_propertyType) {
        const QmlObjectNode objectNode(modelNode());
        if (objectNode.hasBindingProperty(name())) {
            static const QRegularExpression rx(QRegularExpression::anchoredPattern(
                                                   "^[a-z_]\\w*|^[A-Z]\\w*\\.{1}([a-z_]\\w*\\.?)+"));
            const QString exp = objectNode.propertyAffectedByCurrentState(name())
                                ? expression() : modelNode().bindingProperty(name()).expression();
            const QStringList idList = generateStringList(exp);
            for (const auto &id : idList) {
                if (!id.contains(rx))
                    return false;
            }
            return true;
        }
    }
    return false;
}

QStringList PropertyEditorValue::getExpressionAsList() const
{
    NanotraceHR::Tracer tracer{"property editor value get expression as list", category()};

    return generateStringList(expression());
}

QVector<double> PropertyEditorValue::getExpressionAsVector() const
{
    NanotraceHR::Tracer tracer{"property editor value get expression as vector", category()};

    const QRegularExpression rx(
        QRegularExpression::anchoredPattern("Qt.vector(2|3|4)d\\((.*?)\\)"));
    const QRegularExpressionMatch match = rx.match(expression());
    if (!match.hasMatch())
        return {};

    const QStringList floats = match.captured(2).split(',', Qt::SkipEmptyParts);

    bool ok;

    const int num = match.captured(1).toInt();

    if (num != floats.count())
        return {};

    QVector<double> ret;
    for (const QString &number : floats) {
        ret.append(number.toDouble(&ok));

        if (!ok)
            return {};
    }

    return ret;
}

bool PropertyEditorValue::idListAdd(const QString &value)
{
    NanotraceHR::Tracer tracer{"property editor value id list add", category()};

    const QmlObjectNode objectNode(modelNode());
    if (!isIdList() && objectNode.isValid() && objectNode.hasProperty(name()))
        return false;

    static const QRegularExpression rx(QRegularExpression::anchoredPattern(
                                           "^[a-z_]\\w*|^[A-Z]\\w*\\.{1}([a-z_]\\w*\\.?)+"));
    if (!value.contains(rx))
        return false;

    auto stringList = generateStringList(expression());
    stringList.append(value);
    setExpressionWithEmit(generateString(stringList));

    return true;
}

bool PropertyEditorValue::idListRemove(int idx)
{
    NanotraceHR::Tracer tracer{"property editor value id list remove", category()};

    QTC_ASSERT(isIdList(), return false);

    auto stringList = generateStringList(expression());
    if (idx < 0 || idx >= stringList.size())
        return false;

    if (stringList.size() == 1) {
        resetValue();
    } else {
        stringList.removeAt(idx);
        setExpressionWithEmit(generateString(stringList));
    }

    return true;
}

bool PropertyEditorValue::idListReplace(int idx, const QString &value)
{
    NanotraceHR::Tracer tracer{"property editor value id list replace", category()};

    QTC_ASSERT(isIdList(), return false);

    static const QRegularExpression rx(QRegularExpression::anchoredPattern(
                                           "^[a-z_]\\w*|^[A-Z]\\w*\\.{1}([a-z_]\\w*\\.?)+"));
    if (!value.contains(rx))
        return false;

    auto stringList = generateStringList(expression());

    if (idx < 0 || idx >= stringList.size())
        return false;

    stringList.replace(idx, value);
    setExpressionWithEmit(generateString(stringList));

    return true;
}

void PropertyEditorValue::commitDrop(const QString &dropData)
{
    NanotraceHR::Tracer tracer{"property editor value commit drop", category()};

    if (m_propertyType.isQtQuick3DTexture()) {
        m_modelNode.view()->executeInTransaction(__FUNCTION__, [&] {
            ModelNode texture = m_modelNode.view()->modelNodeForInternalId(dropData.toInt());
            if (!texture || !texture.metaInfo().isQtQuick3DTexture()) {
                CreateTexture texCreator(m_modelNode.view());
                texture = texCreator.execute(dropData, AddTextureMode::Texture);
            }

            // assign the texture to the property
            setExpressionWithEmit(texture.id());
        });
    }

    emit dropCommitted(dropData);

    if (!m_modelNode.model())
        return;

    m_modelNode.model()->endDrag();
}

void PropertyEditorValue::editMaterial(int idx)
{
    NanotraceHR::Tracer tracer{"property editor value edit material", category()};

    if (ModelNode material = Utils3D::getMaterialOfModel(m_modelNode, idx)) {
        QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("Properties", true);
        material.selectNode();
    }
}

void PropertyEditorValue::setForceBound(bool b)
{
    NanotraceHR::Tracer tracer{"property editor value set force bound", category()};

    if (m_forceBound == b)
        return;
    m_forceBound = b;

    emit isBoundChanged();
}

void PropertyEditorValue::insertKeyframe()
{
    NanotraceHR::Tracer tracer{"property editor value insert keyframe", category()};

    if (!m_modelNode.isValid())
        return;

    /*If we add more code here we have to forward the property editor view */
    AbstractView *view = m_modelNode.view();

    QmlTimeline timeline = view->currentTimelineNode();

    QTC_ASSERT(timeline.isValid(), return );
    QTC_ASSERT(m_modelNode.isValid(), return );

    view->executeInTransaction("PropertyEditorContextObject::insertKeyframe",
                               [&] { timeline.insertKeyframe(m_modelNode, name()); });
}

QStringList PropertyEditorValue::generateStringList(const QString &string) const
{
    NanotraceHR::Tracer tracer{"property editor value generate string list", category()};

    QString copy = string;
    copy = copy.remove("[").remove("]");

    QStringList tmp = copy.split(',', Qt::SkipEmptyParts);
    for (QString &str : tmp)
        str = str.trimmed();

    return tmp;
}

QString PropertyEditorValue::generateString(const QStringList &stringList) const
{
    NanotraceHR::Tracer tracer{"property editor value generate string", category()};

    if (stringList.size() > 1)
        return "[" + stringList.join(",") + "]";
    else if (stringList.isEmpty())
        return QString();
    else
        return stringList.first();
}

void PropertyEditorValue::registerDeclarativeTypes()
{
    NanotraceHR::Tracer tracer{"property editor value register declarative types", category()};

    qmlRegisterType<PropertyEditorValue>("HelperWidgets", 2, 0, "PropertyEditorValue");
    qmlRegisterType<QQmlPropertyMap>("HelperWidgets", 2, 0, "QQmlPropertyMap");
}

void PropertyEditorValue::resetMetaInfo()
{
    m_propertyType = {};
    m_propertyMetaInfo = {};
}

QQmlPropertyMap *PropertyEditorSubSelectionWrapper::properties()
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper properties", category()};

    return &m_valuesPropertyMap;
}

void PropertyEditorSubSelectionWrapper::createPropertyEditorValue(const QmlObjectNode &qmlObjectNode,
                                                                  PropertyNameView name,
                                                                  const QVariant &value,
                                                                  const PropertyMetaInfo &property)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper create property editor value",
                               category()};

    QString propertyName = QString::fromUtf8(name);
    propertyName.replace('.', '_');

    auto valueObject = variantToPropertyEditorValue(m_valuesPropertyMap.value(propertyName));

    if (!valueObject) {
        valueObject = new PropertyEditorValue(&m_valuesPropertyMap);
        QObject::connect(valueObject, &PropertyEditorValue::valueChanged, this, &PropertyEditorSubSelectionWrapper::changeValue);
        QObject::connect(valueObject, &PropertyEditorValue::expressionChanged, this, &PropertyEditorSubSelectionWrapper::changeExpression);
        QObject::connect(valueObject, &PropertyEditorValue::exportPropertyAsAliasRequested, this, &PropertyEditorSubSelectionWrapper::exportPropertyAsAlias);
        QObject::connect(valueObject, &PropertyEditorValue::removeAliasExportRequested, this, &PropertyEditorSubSelectionWrapper::removeAliasExport);
        m_valuesPropertyMap.insert(propertyName, QVariant::fromValue(valueObject));
    }
    valueObject->setModelNodeAndProperty(qmlObjectNode, name, property);

    if (qmlObjectNode.propertyAffectedByCurrentState(name) && !(qmlObjectNode.modelNode().property(name).isBindingProperty()))
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

void PropertyEditorSubSelectionWrapper::exportPropertyAsAlias(const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper export property as alias",
                               category()};

    if (name.isNull())
        return;

    if (locked())
        return;

    QTC_ASSERT(m_modelNode.isValid(), return);

    view()->executeInTransaction("PropertyEditorView::exportPropertyAsAlias", [this, name](){
        PropertyEditorView::generateAliasForProperty(m_modelNode, name);
    });
}

void PropertyEditorSubSelectionWrapper::removeAliasExport(const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper remove alias export",
                               category()};

    if (name.isNull())
        return;

    if (locked())
        return;

    QTC_ASSERT(m_modelNode.isValid(), return );

    view()->executeInTransaction("PropertyEditorView::exportPropertyAsAlias", [this, name]() {
        PropertyEditorView::removeAliasForProperty(m_modelNode, name);
    });
}

bool PropertyEditorSubSelectionWrapper::locked() const
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper locked", category()};

    return m_locked;
}

PropertyEditorSubSelectionWrapper::PropertyEditorSubSelectionWrapper(const ModelNode &modelNode)
    : m_modelNode(modelNode)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper constructor", category()};

    QmlObjectNode qmlObjectNode(modelNode);

    QTC_ASSERT(qmlObjectNode.isValid(), return );
    for (const auto &property : MetaInfoUtils::addInflatedValueAndReferenceProperties(
             qmlObjectNode.modelNode().metaInfo().properties())) {
        auto propertyName = property.name();
        createPropertyEditorValue(qmlObjectNode,
                                  propertyName,
                                  qmlObjectNode.instanceValue(propertyName),
                                  property.property);
    }
}

ModelNode PropertyEditorSubSelectionWrapper::modelNode() const
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper model node", category()};

    return m_modelNode;
}

void PropertyEditorSubSelectionWrapper::deleteModelNode()
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper delete model node", category()};

    QmlObjectNode objectNode(m_modelNode);

    view()->executeInTransaction("PropertyEditorView::changeExpression", [&] {
        if (objectNode.isValid())
            objectNode.destroy();
    });
}

void PropertyEditorSubSelectionWrapper::changeValue(const QString &name)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper change value", category()};

    QTC_ASSERT(m_modelNode.isValid(), return);

    if (name.isNull())
        return;

    if (locked())
        return;

    const QScopeGuard cleanup([&] { m_locked = false; });
    m_locked = true;

    const NodeMetaInfo metaInfo = m_modelNode.metaInfo();
    QVariant castedValue;
    auto value = variantToPropertyEditorValue(m_valuesPropertyMap.value(name));

    if (auto property = metaInfo.property(name.toUtf8())) {
        castedValue = property.castedValue(value->value());

        if (castedValue.typeId() == QMetaType::QColor) {
            QColor color = castedValue.value<QColor>();
            QColor newColor = QColor(color.name());
            newColor.setAlpha(color.alpha());
            castedValue = QVariant(newColor);
        }

        if (!value->value().isValid()) { // reset
            removePropertyFromModel(name.toUtf8());
        } else {
            if (castedValue.isValid())
                commitVariantValueToModel(name.toUtf8(), castedValue);
        }
    }
}

void PropertyEditorSubSelectionWrapper::setValueFromModel(PropertyNameView name, const QVariant &value)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper set value from model",
                               category()};

    m_locked = true;

    QmlObjectNode qmlObjectNode(m_modelNode);

    Utils::SmallString propertyName = name;
    propertyName.replace('.', '_');

    auto propertyValue = variantToPropertyEditorValue(
        m_valuesPropertyMap.value(QString::fromUtf8(propertyName)));

    if (propertyValue)
        propertyValue->setValue(value);
    m_locked = false;
}

void PropertyEditorSubSelectionWrapper::resetValue(PropertyNameView name)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper reset value", category()};

    auto propertyValue = variantToPropertyEditorValue(
        m_valuesPropertyMap.value(QString::fromUtf8(name)));

    if (propertyValue)
        propertyValue->resetValue();
}

bool PropertyEditorSubSelectionWrapper::isRelevantModelNode(const ModelNode &modelNode) const
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper is relevant model node",
                               category()};

    QmlObjectNode objectNode(m_modelNode);
    return modelNode == m_modelNode || objectNode.propertyChangeForCurrentState() == modelNode;
}

void PropertyEditorSubSelectionWrapper::changeExpression(const QString &propertyName)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper change expression", category()};

    PropertyName name = propertyName.toUtf8();

    QTC_ASSERT(m_modelNode.isValid(), return );

    if (name.isNull())
        return;

    if (locked())
        return;

    const QScopeGuard cleanup([&] { m_locked = false; });
    m_locked = true;

    view()->executeInTransaction("PropertyEditorView::changeExpression", [this, name, propertyName] {
        QmlObjectNode qmlObjectNode{m_modelNode};
        auto value = variantToPropertyEditorValue(m_valuesPropertyMap.value(propertyName));

        if (!value) {
            qWarning() << "PropertyEditor::changeExpression no value for " << propertyName;
            return;
        }

        if (value->expression().isEmpty()) {
            value->resetValue();
            return;
        }
        PropertyEditorView::setExpressionOnObjectNode(qmlObjectNode, name, value->expression());
    }); /* end of transaction */
}

void PropertyEditorSubSelectionWrapper::removePropertyFromModel(PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper remove property from model",
                               category()};

    QTC_ASSERT(m_modelNode.isValid(), return );

    m_locked = true;
    try {
        RewriterTransaction transaction = view()->beginRewriterTransaction(
            "PropertyEditorView::removePropertyFromModel");

        QmlObjectNode(m_modelNode).removeProperty(propertyName);

        transaction.commit();
    } catch (const RewritingException &e) {
        e.showException();
    }
    m_locked = false;
}

void PropertyEditorSubSelectionWrapper::commitVariantValueToModel(PropertyNameView propertyName,
                                                                  const QVariant &value)
{
    NanotraceHR::Tracer tracer{
        "property editor sub selection wrapper commit variant value to model", category()};

    QTC_ASSERT(m_modelNode.isValid(), return );

    try {
        RewriterTransaction transaction = view()->beginRewriterTransaction(
            "PropertyEditorView::commitVariantValueToMode");

        QmlObjectNode(m_modelNode).setVariantProperty(propertyName, value);

        transaction.commit();
    } catch (const RewritingException &e) {
        e.showException();
    }
}

AbstractView *PropertyEditorSubSelectionWrapper::view() const
{
    NanotraceHR::Tracer tracer{"property editor sub selection wrapper view", category()};
    QTC_CHECK(m_modelNode.isValid());

    return m_modelNode.view();
}

PropertyEditorValue *variantToPropertyEditorValue(const QVariant &value)
{
    if (auto object = get_if<PropertyEditorValue *>(&value))
        return *object;

    return nullptr;
}
} // namespace QmlDesigner
