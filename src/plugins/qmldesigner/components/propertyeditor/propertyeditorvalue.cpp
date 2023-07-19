// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "propertyeditorvalue.h"

#include "abstractview.h"
#include "bindingproperty.h"
#include "createtexture.h"
#include "designermcumanager.h"
#include "designmodewidget.h"
#include "nodemetainfo.h"
#include "nodeproperty.h"
#include "qmlitemnode.h"
#include "qmlobjectnode.h"
#include "qmldesignerplugin.h"

#include <enumeration.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>
#include <QScopedPointer>
#include <QUrl>

namespace QmlDesigner {

PropertyEditorValue::PropertyEditorValue(QObject *parent)
    : QObject(parent),
      m_complexNode(new PropertyEditorNodeWrapper(this))
{
}

QVariant PropertyEditorValue::value() const
{
    QVariant returnValue = m_value;
    if (auto metaInfo = modelNode().metaInfo(); metaInfo.property(name()).propertyType().isUrl())
        returnValue = returnValue.toUrl().toString();

    return returnValue;
}

static bool cleverDoubleCompare(const QVariant &value1, const QVariant &value2)
{
    if (value1.typeId() == QVariant::Double && value2.typeId() == QVariant::Double) {
        // ignore slight changes on doubles
        if (qFuzzyCompare(value1.toDouble(), value2.toDouble()))
            return true;
    }
    return false;
}

static bool cleverColorCompare(const QVariant &value1, const QVariant &value2)
{
    if (value1.typeId() == QVariant::Color && value2.typeId() == QVariant::Color) {
        QColor c1 = value1.value<QColor>();
        QColor c2 = value2.value<QColor>();
        return c1.name() == c2.name() && c1.alpha() == c2.alpha();
    }

    if (value1.typeId() == QVariant::String && value2.typeId() == QVariant::Color)
        return cleverColorCompare(QVariant(QColor(value1.toString())), value2);

    if (value1.typeId() == QVariant::Color && value2.typeId() == QVariant::String)
        return cleverColorCompare(value1, QVariant(QColor(value2.toString())));

    return false;
}

// "red" is the same color as "#ff0000"
// To simplify editing we convert all explicit color names in the hash format
static void fixAmbigousColorNames(const ModelNode &modelNode, const PropertyName &name, QVariant *value)
{
    if (auto metaInfo = modelNode.metaInfo(); metaInfo.property(name).propertyType().isColor()) {
        if (value->typeId() == QVariant::Color) {
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

static void fixUrl(const ModelNode &modelNode, const PropertyName &name, QVariant *value)
{
    if (auto metaInfo = modelNode.metaInfo(); metaInfo.property(name).propertyType().isUrl()) {
        if (!value->isValid())
            *value = QStringLiteral("");
    }
}

/* The comparison of variants is not symmetric because of implicit conversion.
 * QVariant(string) == QVariant(QColor) does for example ignore the alpha channel,
 * because the color is converted to a string ignoring the alpha channel.
 * By comparing the variants in both directions we gain a symmetric comparison.
 */
static bool compareVariants(const QVariant &value1, const QVariant &value2)
{
    return value1 == value2 && value2 == value1;
}

void PropertyEditorValue::setValueWithEmit(const QVariant &value)
{
    if (!compareVariants(value, m_value) || isBound()) {
        QVariant newValue = value;
        if (auto metaInfo = modelNode().metaInfo(); metaInfo.property(name()).propertyType().isUrl())
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
    const bool colorsEqual = cleverColorCompare(value, m_value);

    if (!compareVariants(m_value, value) && !cleverDoubleCompare(value, m_value) && !colorsEqual)
        m_value = value;

    fixAmbigousColorNames(modelNode(), name(), &m_value);
    fixUrl(modelNode(), name(), &m_value);

    if (!colorsEqual)
        emit valueChangedQml();

    emit isExplicitChanged();
    emit isBoundChanged();
}

QString PropertyEditorValue::enumeration() const
{
    return m_value.value<Enumeration>().nameToString();
}

QString PropertyEditorValue::expression() const
{
    return m_expression;
}

void PropertyEditorValue::setExpressionWithEmit(const QString &expression)
{
    if (m_expression != expression) {
        setExpression(expression);
        m_value.clear();
        emit expressionChanged(nameAsQString()); // Note that we set the name in this case
    }
}

void PropertyEditorValue::setExpression(const QString &expression)
{
    if ( m_expression != expression) {
        m_expression = expression;
        emit expressionChanged(QString());
    }
}

QString PropertyEditorValue::valueToString() const
{
    return value().toString();
}

bool PropertyEditorValue::isInSubState() const
{
    const QmlObjectNode objectNode(modelNode());
    return objectNode.isValid() && objectNode.currentState().isValid()
           && objectNode.propertyAffectedByCurrentState(name());
}

bool PropertyEditorValue::isBound() const
{
    const QmlObjectNode objectNode(modelNode());
    return objectNode.isValid() && objectNode.hasBindingProperty(name());
}

bool PropertyEditorValue::isInModel() const
{
    return modelNode().hasProperty(name());
}

PropertyName PropertyEditorValue::name() const
{
    return m_name;
}

QString PropertyEditorValue::nameAsQString() const
{
    return QString::fromUtf8(m_name);
}

void PropertyEditorValue::setName(const PropertyName &name)
{
    m_name = name;
}

bool PropertyEditorValue::isValid() const
{
    return m_isValid;
}

void PropertyEditorValue::setIsValid(bool valid)
{
    m_isValid = valid;
}

bool PropertyEditorValue::isTranslated() const
{
    if (modelNode().isValid()) {
        if (auto metaInfo = modelNode().metaInfo();
            metaInfo.isValid() && metaInfo.hasProperty(name())
            && metaInfo.property(name()).propertyType().isString()) {
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
    }
    return false;
}

bool PropertyEditorValue::hasActiveDrag() const
{
    return m_hasActiveDrag;
}

void PropertyEditorValue::setHasActiveDrag(bool val)
{
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
    if (!m_modelNode.isValid())
        return true;

    const DesignerMcuManager &mcuManager = DesignerMcuManager::instance();

    if (mcuManager.isMCUProject()) {
        const QSet<QString> nonMcuProperties = mcuManager.bannedProperties();
        const auto mcuAllowedItemProperties = mcuManager.allowedItemProperties();
        const auto mcuBannedComplexProperties = mcuManager.bannedComplexProperties();

        const QList<QByteArray> list = name().split('.');
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
    return m_modelNode;
}

void PropertyEditorValue::setModelNode(const ModelNode &modelNode)
{
    if (modelNode != m_modelNode) {
        m_modelNode = modelNode;
        m_complexNode->update();
        emit modelNodeChanged();
    }
}

PropertyEditorNodeWrapper *PropertyEditorValue::complexNode()
{
    return m_complexNode;
}

void PropertyEditorValue::resetValue()
{
    if (m_value.isValid() || !m_expression.isEmpty() || isBound()) {
        m_value = QVariant();
        m_isBound = false;
        m_expression = QString();
        emit valueChanged(nameAsQString(), QVariant());
        emit expressionChanged({});
    }
}

void PropertyEditorValue::setEnumeration(const QString &scope, const QString &name)
{
    Enumeration newEnumeration(scope.toUtf8(), name.toUtf8());

    setValueWithEmit(QVariant::fromValue(newEnumeration));
}

void PropertyEditorValue::exportPropertyAsAlias()
{
    emit exportPropertyAsAliasRequested(nameAsQString());
}

bool PropertyEditorValue::hasPropertyAlias() const
{
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
    return !nameAsQString().isEmpty() && nameAsQString().at(0).isUpper();
}

void PropertyEditorValue::removeAliasExport()
{
    emit removeAliasExportRequested(nameAsQString());
}

QString PropertyEditorValue::getTranslationContext() const
{
    if (modelNode().isValid()) {
        if (auto metaInfo = modelNode().metaInfo();
            metaInfo.isValid() && metaInfo.hasProperty(name())
            && metaInfo.property(name()).propertyType().isString()) {
            const QmlObjectNode objectNode(modelNode());
            if (objectNode.hasBindingProperty(name())) {
                const QRegularExpression rx(QRegularExpression::anchoredPattern(
                    "qsTranslate\\(\"(.*)\"\\s*,\\s*\".*\"\\s*\\)"));
                const QRegularExpressionMatch match = rx.match(expression());
                if (match.hasMatch())
                    return match.captured(1);
            }
        }
    }
    return QString();
}

bool PropertyEditorValue::isIdList() const
{
    if (modelNode().isValid() && modelNode().metaInfo().isValid() && modelNode().metaInfo().hasProperty(name())) {
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
    return generateStringList(expression());
}

bool PropertyEditorValue::idListAdd(const QString &value)
{
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
    if (m_modelNode.metaInfo().isQtQuick3DMaterial()
        && m_modelNode.metaInfo().property(m_name).propertyType().isQtQuick3DTexture()) {
        m_modelNode.view()->executeInTransaction(__FUNCTION__, [&] {
            ModelNode texture = m_modelNode.view()->modelNodeForInternalId(dropData.toInt());
            if (!texture || !texture.metaInfo().isQtQuick3DTexture()) {
                auto texCreator = new CreateTexture(m_modelNode.view());
                texture = texCreator->execute(dropData, AddTextureMode::Texture);
                texCreator->deleteLater();
            }

            // assign the texture to the property
            setExpressionWithEmit(texture.id());
        });
    }

    m_modelNode.view()->model()->endDrag();
}

void PropertyEditorValue::openMaterialEditor(int idx)
{
    QmlDesignerPlugin::instance()->mainWidget()->showDockWidget("MaterialEditor", true);
    m_modelNode.view()->emitCustomNotification("select_material", {}, {idx});
}

QStringList PropertyEditorValue::generateStringList(const QString &string) const
{
    QString copy = string;
    copy = copy.remove("[").remove("]");

    QStringList tmp = copy.split(',', Qt::SkipEmptyParts);
    for (QString &str : tmp)
        str = str.trimmed();

    return tmp;
}

QString PropertyEditorValue::generateString(const QStringList &stringList) const
{
    if (stringList.size() > 1)
        return "[" + stringList.join(",") + "]";
    else if (stringList.isEmpty())
        return QString();
    else
        return stringList.first();
}

void PropertyEditorValue::registerDeclarativeTypes()
{
    qmlRegisterType<PropertyEditorValue>("HelperWidgets", 2, 0, "PropertyEditorValue");
    qmlRegisterType<PropertyEditorNodeWrapper>("HelperWidgets", 2, 0, "PropertyEditorNodeWrapper");
    qmlRegisterType<QQmlPropertyMap>("HelperWidgets", 2, 0, "QQmlPropertyMap");
}

PropertyEditorNodeWrapper::PropertyEditorNodeWrapper(PropertyEditorValue *parent)
    : QObject(parent),
      m_valuesPropertyMap(this)
{
    m_editorValue = parent;
    connect(m_editorValue, &PropertyEditorValue::modelNodeChanged, this, &PropertyEditorNodeWrapper::update);
}

PropertyEditorNodeWrapper::PropertyEditorNodeWrapper(QObject *parent)
    : QObject(parent)
{
}

bool PropertyEditorNodeWrapper::exists() const
{
    return m_editorValue && m_editorValue->modelNode().isValid() && m_modelNode.isValid();
}

QString PropertyEditorNodeWrapper::type() const
{
    return m_modelNode.simplifiedTypeName();
}

ModelNode PropertyEditorNodeWrapper::parentModelNode() const
{
    return  m_editorValue->modelNode();
}

PropertyName PropertyEditorNodeWrapper::propertyName() const
{
    return m_editorValue->name();
}

QQmlPropertyMap *PropertyEditorNodeWrapper::properties()
{
    return &m_valuesPropertyMap;
}

void PropertyEditorNodeWrapper::add(const QString &type)
{
    TypeName propertyType = type.toUtf8();

    if ((m_editorValue && m_editorValue->modelNode().isValid())) {
        if (propertyType.isEmpty()) {
            propertyType = m_editorValue->modelNode()
                               .metaInfo()
                               .property(m_editorValue->name())
                               .propertyType()
                               .typeName();
        }
        while (propertyType.contains('*')) // strip star
            propertyType.chop(1);
        m_modelNode = m_editorValue->modelNode().view()->createModelNode(propertyType, 4, 7);
        m_editorValue->modelNode().nodeAbstractProperty(m_editorValue->name()).reparentHere(m_modelNode);
        if (!m_modelNode.isValid())
            qWarning("PropertyEditorNodeWrapper::add failed");
    } else {
        qWarning("PropertyEditorNodeWrapper::add failed - node invalid");
    }
    setup();
}

void PropertyEditorNodeWrapper::remove()
{
    if ((m_editorValue && m_editorValue->modelNode().isValid())) {
        QmlObjectNode(m_modelNode).destroy();
        m_editorValue->modelNode().removeProperty(m_editorValue->name());
    } else {
        qWarning("PropertyEditorNodeWrapper::remove failed - node invalid");
    }
    m_modelNode = ModelNode();

    const QStringList propertyNames = m_valuesPropertyMap.keys();
    for (const QString &propertyName : propertyNames)
        m_valuesPropertyMap.clear(propertyName);
    qDeleteAll(m_valuesPropertyMap.children());
    emit propertiesChanged();
    emit existsChanged();
}

void PropertyEditorNodeWrapper::changeValue(const QString &propertyName)
{
    const PropertyName name = propertyName.toUtf8();

    if (name.isNull())
        return;

    if (auto qmlObjectNode = QmlObjectNode{m_modelNode}) {
        auto valueObject = qvariant_cast<PropertyEditorValue *>(m_valuesPropertyMap.value(QString::fromLatin1(name)));

        if (valueObject->value().isValid())
            qmlObjectNode.setVariantProperty(name, valueObject->value());
        else
            qmlObjectNode.removeProperty(name);
    }
}

void PropertyEditorNodeWrapper::setup()
{
    Q_ASSERT(m_editorValue);
    Q_ASSERT(m_editorValue->modelNode().isValid());

    if (m_editorValue->modelNode().isValid() && m_modelNode.isValid()) {
        const QStringList propertyNames = m_valuesPropertyMap.keys();
        for (const QString &propertyName : propertyNames)
            m_valuesPropertyMap.clear(propertyName);
        qDeleteAll(m_valuesPropertyMap.children());

        if (QmlObjectNode qmlObjectNode = m_modelNode) {
            const PropertyMetaInfos props = m_modelNode.metaInfo().properties();
            for (const auto &property : props) {
                const auto &propertyName = property.name();
                auto valueObject = new PropertyEditorValue(&m_valuesPropertyMap);
                valueObject->setName(propertyName);
                valueObject->setValue(qmlObjectNode.instanceValue(propertyName));
                connect(valueObject, &PropertyEditorValue::valueChanged, &m_valuesPropertyMap, &QQmlPropertyMap::valueChanged);
                m_valuesPropertyMap.insert(QString::fromUtf8(propertyName), QVariant::fromValue(valueObject));
            }
        }
    }
    connect(&m_valuesPropertyMap, &QQmlPropertyMap::valueChanged, this, &PropertyEditorNodeWrapper::changeValue);

    emit propertiesChanged();
    emit existsChanged();
}

void PropertyEditorNodeWrapper::update()
{
    if (!m_editorValue || !m_editorValue->modelNode().isValid())
        return;

    if (m_editorValue->modelNode().hasProperty(m_editorValue->name())
        && m_editorValue->modelNode().property(m_editorValue->name()).isNodeProperty()) {
        m_modelNode = m_editorValue->modelNode().nodeProperty(m_editorValue->name()).modelNode();
    }

    setup();
    emit existsChanged();
    emit typeChanged();
}

} // namespace QmlDesigner
