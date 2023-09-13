// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientmodel.h"

#include "gradientpresetcustomlistmodel.h"
#include "gradientpresetitem.h"
#include "propertyeditorview.h"
#include "qmlanchorbindingproxy.h"

#include <abstractview.h>
#include <exception.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <rewritertransaction.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <QScopeGuard>
#include <QTimer>

namespace {
constexpr auto defaultValueLinearX1 = [](const QmlDesigner::QmlItemNode &) -> qreal { return 0.0; };
constexpr auto defaultValueLinearY1 = [](const QmlDesigner::QmlItemNode &) -> qreal { return 0.0; };
constexpr auto defaultValueLinearX2 = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return node.instanceValue("width").toReal();
};
constexpr auto defaultValueLinearY2 = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return node.instanceValue("height").toReal();
};
constexpr auto defaultValueRadialCenterRadius = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    const qreal width = node.instanceValue("width").toReal();
    const qreal height = node.instanceValue("height").toReal();
    return qMin(width, height) / 2.0;
};
constexpr auto defaultValueRadialCenterX = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return (node.instanceValue("width").toReal() / 2.0);
};
constexpr auto defaultValueRadialCenterY = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return (node.instanceValue("height").toReal() / 2.0);
};
constexpr auto defaultValueRadialFocalRadius = [](const QmlDesigner::QmlItemNode &) -> qreal {
    return 0.0;
};
constexpr auto defaultValueRadialFocalX = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return (node.instanceValue("width").toReal() / 2.0);
};
constexpr auto defaultValueRadialFocalY = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return (node.instanceValue("height").toReal() / 2.0);
};
constexpr auto defaultValueConicalAngle = [](const QmlDesigner::QmlItemNode &) -> qreal {
    return 0.0;
};
constexpr auto defaultValueConicalCenterX = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return (node.instanceValue("width").toReal() / 2.0);
};
constexpr auto defaultValueConicalCenterY = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return (node.instanceValue("height").toReal() / 2.0);
};

using DefaultValueFunctionVariant = std::variant<std::monostate,
                                                 decltype(defaultValueLinearX1),
                                                 decltype(defaultValueLinearY1),
                                                 decltype(defaultValueLinearX2),
                                                 decltype(defaultValueLinearY2),
                                                 decltype(defaultValueRadialCenterRadius),
                                                 decltype(defaultValueRadialCenterX),
                                                 decltype(defaultValueRadialCenterY),
                                                 decltype(defaultValueRadialFocalRadius),
                                                 decltype(defaultValueRadialFocalX),
                                                 decltype(defaultValueRadialFocalY),
                                                 decltype(defaultValueConicalAngle),
                                                 decltype(defaultValueConicalCenterX),
                                                 decltype(defaultValueConicalCenterY)>;
} // namespace

class ShapeGradientPropertyData
{
public:
    enum class UsePercents { No, Yes };

    constexpr ShapeGradientPropertyData() = default;

    constexpr ShapeGradientPropertyData(QmlDesigner::PropertyNameView name,
                                        QmlDesigner::PropertyNameView bindingProperty,
                                        UsePercents canPercent,
                                        DefaultValueFunctionVariant defVariant)
        : name(name)
        , bindingProperty(bindingProperty)
        , canUsePercentage(canPercent)
        , m_defaultValue(defVariant)
    {}

    QmlDesigner::PropertyNameView name;
    QmlDesigner::PropertyNameView bindingProperty;
    UsePercents canUsePercentage = UsePercents::No;

private:
    DefaultValueFunctionVariant m_defaultValue;

public:
    constexpr qreal getDefaultValue(const QmlDesigner::QmlItemNode &itemNode) const
    {
        return std::visit(
            [&](auto &defValue) -> qreal {
                using Type = std::decay_t<decltype(defValue)>;
                if constexpr (std::is_same_v<Type, std::monostate>)
                    return 0.0;
                else
                    return defValue(itemNode);
            },
            m_defaultValue);
    }
};

constexpr QmlDesigner::PropertyNameView linearX1Str = u8"x1";
constexpr QmlDesigner::PropertyNameView linearX2Str = u8"x2";
constexpr QmlDesigner::PropertyNameView linearY1Str = u8"y1";
constexpr QmlDesigner::PropertyNameView linearY2Str = u8"y2";

constexpr QmlDesigner::PropertyNameView radialCenterRadiusStr = u8"centerRadius";
constexpr QmlDesigner::PropertyNameView radialCenterXStr = u8"centerX";
constexpr QmlDesigner::PropertyNameView radialCenterYStr = u8"centerY";
constexpr QmlDesigner::PropertyNameView radialFocalRadiusStr = u8"focalRadius";
constexpr QmlDesigner::PropertyNameView radialFocalXStr = u8"focalX";
constexpr QmlDesigner::PropertyNameView radialFocalYStr = u8"focalY";

constexpr QmlDesigner::PropertyNameView conicalAngleStr = u8"angle";
constexpr QmlDesigner::PropertyNameView conicalCenterXStr = u8"centerX";
constexpr QmlDesigner::PropertyNameView conicalCenterYStr = u8"centerY";

constexpr ShapeGradientPropertyData defaultLinearShapeGradients[] = {
    {linearX1Str, u8"width", ShapeGradientPropertyData::UsePercents::Yes, defaultValueLinearX1},
    {linearX2Str, u8"width", ShapeGradientPropertyData::UsePercents::Yes, defaultValueLinearX2},
    {linearY1Str, u8"height", ShapeGradientPropertyData::UsePercents::Yes, defaultValueLinearY1},
    {linearY2Str, u8"height", ShapeGradientPropertyData::UsePercents::Yes, defaultValueLinearY2}};

constexpr ShapeGradientPropertyData defaultRadialShapeGradients[] = {
    {radialCenterRadiusStr, u8"", ShapeGradientPropertyData::UsePercents::No, defaultValueRadialCenterRadius},
    {radialCenterXStr, u8"width", ShapeGradientPropertyData::UsePercents::Yes, defaultValueRadialCenterX},
    {radialCenterYStr, u8"height", ShapeGradientPropertyData::UsePercents::Yes, defaultValueRadialCenterY},
    {radialFocalRadiusStr, u8"", ShapeGradientPropertyData::UsePercents::No, defaultValueRadialFocalRadius},
    {radialFocalXStr, u8"width", ShapeGradientPropertyData::UsePercents::Yes, defaultValueRadialFocalX},
    {radialFocalYStr, u8"height", ShapeGradientPropertyData::UsePercents::Yes, defaultValueRadialFocalY}};

constexpr ShapeGradientPropertyData defaultConicalShapeGradients[] = {
    {conicalAngleStr, u8"", ShapeGradientPropertyData::UsePercents::No, defaultValueConicalAngle},
    {conicalCenterXStr, u8"width", ShapeGradientPropertyData::UsePercents::Yes, defaultValueConicalCenterX},
    {conicalCenterYStr,
     u8"height",
     ShapeGradientPropertyData::UsePercents::Yes,
     defaultValueConicalCenterY}};

template<typename GradientArrayType>
const ShapeGradientPropertyData *findGradientInArray(const GradientArrayType &array,
                                                     const QmlDesigner::PropertyNameView propName)
{
    const auto found = std::find_if(std::begin(array), std::end(array), [&](const auto &entry) {
        return entry.name == propName;
    });
    if (found != std::end(array))
        return std::addressof(*found);

    return nullptr;
}

const ShapeGradientPropertyData *getDefaultGradientData(const QmlDesigner::PropertyNameView propName,
                                                        const QStringView &gradientType)
{
    if (gradientType == u"LinearGradient") {
        return findGradientInArray(defaultLinearShapeGradients, propName);
    } else if (gradientType == u"RadialGradient") {
        return findGradientInArray(defaultRadialShapeGradients, propName);
    } else if (gradientType == u"ConicalGradient") {
        return findGradientInArray(defaultConicalShapeGradients, propName);
    }

    return nullptr;
}

template<typename T>
void prepareGradient(const T &array,
                     const QmlDesigner::ModelNode &gradient,
                     const QmlDesigner::QmlItemNode &node)
{
    std::for_each(std::begin(array), std::end(array), [&](auto &a) {
        gradient.variantProperty(a.name.toByteArray()).setValue(a.getDefaultValue(node));
    });
}

GradientModel::GradientModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

int GradientModel::rowCount(const QModelIndex & /*parent*/) const
{
    if (m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
        QmlDesigner::ModelNode gradientNode = m_itemNode.modelNode()
                                                  .nodeProperty(gradientPropertyName().toUtf8())
                                                  .modelNode();

        if (gradientNode.hasNodeListProperty("stops"))
            return gradientNode.nodeListProperty("stops").count();
    }

    return 0;
}

QHash<int, QByteArray> GradientModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{
        {Qt::UserRole + 1, "position"},
        {Qt::UserRole + 2, "color"},
        {Qt::UserRole + 3, "readOnly"},
        {Qt::UserRole + 4, "index"}
    };

    return roleNames;
}

QVariant GradientModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {

        if (role == Qt::UserRole + 3) {
            if (index.row() == 0 || index.row() == (rowCount() - 1))
                return true;
            return false;
        } else if (role == Qt::UserRole + 4) {
            return index.row();
        }

        if (role == Qt::UserRole + 1)
            return getPosition(index.row());
        else if (role == Qt::UserRole + 2)
            return getColor(index.row());

        qWarning() << Q_FUNC_INFO << "invalid role";
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }

    return QVariant();
}

int GradientModel::addStop(qreal position, const QColor &color)
{
    if (m_locked)
        return -1;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return -1;

    if (m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
        int properPos = 0;

        try {
            QmlDesigner::ModelNode gradientNode = m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();

            QmlDesigner::ModelNode gradientStopNode = createGradientStopNode();

            gradientStopNode.variantProperty("position").setValue(position);
            gradientStopNode.variantProperty("color").setValue(color);
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

            const QList<QmlDesigner::ModelNode> stopNodes = gradientNode.nodeListProperty("stops").toModelNodeList();

            for (int i = 0; i < stopNodes.size(); i++) {
                if (QmlDesigner::QmlObjectNode(stopNodes.at(i)).modelValue("position").toReal() < position)
                    properPos = i + 1;
            }
            gradientNode.nodeListProperty("stops").slide(stopNodes.size() - 1, properPos);

            setupModel();
            resetPuppet();

            emit gradientCountChanged();
        } catch (const QmlDesigner::Exception &e) {
            e.showException();
        }

        return properPos;
    }

    return -1;
}

void GradientModel::addGradient()
{
    if (m_locked)
        return;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return;

    if (!m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {

        if (m_gradientTypeName != "Gradient")
            ensureShapesImport();

        view()->executeInTransaction("GradientModel::addGradient", [this](){
            QColor color = m_itemNode.instanceValue("color").value<QColor>();

            if (!color.isValid())
                color = QColor(Qt::white);

            QmlDesigner::ModelNode gradientNode = createGradientNode();

            m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).reparentHere(gradientNode);

            QmlDesigner::ModelNode gradientStopNode = createGradientStopNode();
            gradientStopNode.variantProperty("position").setValue(0.0);
            gradientStopNode.variantProperty("color").setValue(color);
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

            gradientStopNode = createGradientStopNode();
            gradientStopNode.variantProperty("position").setValue(1.0);
            gradientStopNode.variantProperty("color").setValue(QColor(Qt::black));
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);
        });
    }
    setupModel();

    if (m_gradientTypeName != "Gradient")
        resetPuppet(); /*Unfortunately required */

    emit hasGradientChanged();
    emit gradientTypeChanged();
}

void GradientModel::setColor(int index, const QColor &color)
{
    if (locked())
        return;

    if (!m_itemNode.isValid())
        return;

    if (!m_itemNode.modelNode().isSelected())
        return;

    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            stop.setVariantProperty("color", color);
        setupModel();
    }
}

void GradientModel::setPosition(int index, qreal positition)
{
    if (locked())
        return;

    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            stop.setVariantProperty("position", positition);
        setupModel();
    }
}

QColor GradientModel::getColor(int index) const
{
    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            return stop.modelValue("color").value<QColor>();
    }
    qWarning() << Q_FUNC_INFO << "invalid color index";
    return {};
}

qreal GradientModel::getPosition(int index) const
{
    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            return stop.modelValue("position").toReal();
    }
    qWarning() << Q_FUNC_INFO << "invalid position index";
    return 0.0;
}

void GradientModel::removeStop(int index)
{
    if (index < rowCount() - 1 && index != 0) {
        view()->executeInTransaction("GradientModel::removeStop", [this, index](){
            QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
            QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
            if (stop.isValid()) {
                stop.destroy();
                setupModel();
                resetPuppet();

                emit gradientCountChanged();
            }
        });

        return;
    }

    qWarning() << Q_FUNC_INFO << "invalid index";
}

void GradientModel::deleteGradient()
{
    if (!m_itemNode.isValid())
        return;

    if (!m_itemNode.modelNode().metaInfo().hasProperty(gradientPropertyName().toUtf8()))
        return;

    deleteGradientNode(true);

    emit hasGradientChanged();
    emit gradientTypeChanged();
}

void GradientModel::lock()
{
    m_locked = true;
}

void GradientModel::unlock()
{
    m_locked = false;
}

void GradientModel::registerDeclarativeType()
{
    qmlRegisterType<GradientModel>("HelperWidgets", 2, 0, "GradientModel");
}

qreal GradientModel::readGradientProperty(const QString &propertyName) const
{
    if (!m_itemNode.isValid())
        return 0;

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    if (!gradient.isValid())
        return 0;

    return gradient.modelValue(propertyName.toUtf8()).toReal();
}

qreal GradientModel::readGradientPropertyPercentage(const QString &propertyName) const
{
    return getPercentageGradientProperty(propertyName.toUtf8());
}

QString GradientModel::readGradientOrientation() const
{
    if (!m_itemNode.isValid())
        return QString();

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    if (!gradient.isValid())
        return QString();

    return gradient.modelValue("orientation").value<QmlDesigner::Enumeration>().nameToString();
}

GradientModel::GradientPropertyUnits GradientModel::readGradientPropertyUnits(
    const QString &propertyName) const
{
    if (hasPercentageGradientProperty(propertyName))
        return GradientPropertyUnits::Percentage;

    return GradientPropertyUnits::Pixels;
}

bool GradientModel::isPercentageSupportedByProperty(const QString &propertyName,
                                                    const QString &gradientTypeName) const
{
    const auto gradientPropertyData = getDefaultGradientPropertyData(propertyName.toUtf8(), gradientTypeName);
    if (!gradientPropertyData.name.isEmpty())
        return gradientPropertyData.canUsePercentage == ShapeGradientPropertyData::UsePercents::Yes;

    return false;
}

void GradientModel::setupModel()
{
    m_locked = true;
    const QScopeGuard cleanup([&] { m_locked = false; });

    beginResetModel();
    endResetModel();
}

void GradientModel::setAnchorBackend(const QVariant &anchorBackend)
{
    auto anchorBackendObject = anchorBackend.value<QObject*>();

    const auto backendCasted =
            qobject_cast<const QmlDesigner::Internal::QmlAnchorBindingProxy *>(anchorBackendObject);

    if (backendCasted)
        m_itemNode = backendCasted->getItemNode();

    if (m_itemNode.isValid()
            && m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8()))
        m_gradientTypeName = m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode().simplifiedTypeName();

    setupModel();

    m_locked = true;
    const QScopeGuard cleanup([&] { m_locked = false; });

    emit anchorBackendChanged();
    emit hasGradientChanged();
    emit gradientTypeChanged();
}

QString GradientModel::gradientPropertyName() const
{
    return m_gradientPropertyName;
}

void GradientModel::setGradientPropertyName(const QString &name)
{
    m_gradientPropertyName = name;
}

QString GradientModel::gradientTypeName() const
{
    return m_gradientTypeName;
}

void GradientModel::setGradientTypeName(const QString &name)
{
    m_gradientTypeName = name;
}

bool GradientModel::hasGradient() const
{
    return m_itemNode.isValid()
            && m_itemNode.modelNode().hasProperty(gradientPropertyName().toUtf8());
}

bool GradientModel::locked() const
{
    if (m_locked)
        return true;

    auto editorView = qobject_cast<QmlDesigner::PropertyEditorView*>(view());

    return editorView && editorView->locked();
}

bool GradientModel::hasShapesImport() const
{
    if (m_itemNode.isValid()) {
        QmlDesigner::Import import = QmlDesigner::Import::createLibraryImport("QtQuick.Shapes", "1.0");
        return model()->hasImport(import, true, true);
    }

    return false;
}

void GradientModel::ensureShapesImport()
{
    if (!hasShapesImport()) {
        QmlDesigner::Import timelineImport = QmlDesigner::Import::createLibraryImport("QtQuick.Shapes", "1.0");
        try {
            model()->changeImports({timelineImport}, {});
        } catch (const QmlDesigner::Exception &) {
            QTC_ASSERT(false, return);
        }
    }
}

void GradientModel::setupGradientProperties(const QmlDesigner::ModelNode &gradient)
{
    QTC_ASSERT(m_itemNode.isValid(), return);

    QTC_ASSERT(gradient.isValid(), return);

    if (m_gradientTypeName == u"Gradient") {
        gradient.variantProperty("orientation").setEnumeration("Gradient.Vertical");
    } else if (m_gradientTypeName == u"LinearGradient") {
        prepareGradient(defaultLinearShapeGradients, gradient, m_itemNode);
    } else if (m_gradientTypeName == u"RadialGradient") {
        prepareGradient(defaultRadialShapeGradients, gradient, m_itemNode);
    } else if (m_gradientTypeName == u"ConicalGradient") {
        prepareGradient(defaultConicalShapeGradients, gradient, m_itemNode);
    }
}

QmlDesigner::Model *GradientModel::model() const
{
    QTC_ASSERT(m_itemNode.isValid(), return nullptr);
    return m_itemNode.view()->model();
}

QmlDesigner::AbstractView *GradientModel::view() const
{
    QTC_ASSERT(m_itemNode.isValid(), return nullptr);
    return m_itemNode.view();
}

void GradientModel::resetPuppet()
{
    QTimer::singleShot(1000, view(), &QmlDesigner::AbstractView::resetPuppet);
}

QmlDesigner::ModelNode GradientModel::createGradientNode()
{
    QmlDesigner::TypeName fullTypeName = m_gradientTypeName.toUtf8();

    if (m_gradientTypeName == "Gradient") {
        fullTypeName.prepend("QtQuick.");
    } else {
        fullTypeName.prepend("QtQuick.Shapes.");
    }

    auto metaInfo = model()->metaInfo(fullTypeName);

    int minorVersion = metaInfo.minorVersion();
    int majorVersion = metaInfo.majorVersion();

    auto gradientNode = view()->createModelNode(fullTypeName, majorVersion, minorVersion);

    setupGradientProperties(gradientNode);

    return gradientNode;
}

QmlDesigner::ModelNode GradientModel::createGradientStopNode()
{
    QByteArray fullTypeName = "QtQuick.GradientStop";
    auto metaInfo = model()->metaInfo(fullTypeName);

    int minorVersion = metaInfo.minorVersion();
    int majorVersion = metaInfo.majorVersion();

    return view()->createModelNode(fullTypeName, majorVersion, minorVersion);
}

void GradientModel::deleteGradientNode(bool saveTransaction)
{
    QmlDesigner::ModelNode modelNode = m_itemNode.modelNode();

    if (m_itemNode.isInBaseState()) {
        if (modelNode.hasProperty(gradientPropertyName().toUtf8())) {
            QmlDesigner::RewriterTransaction transaction;
            if (saveTransaction)
                transaction = view()->beginRewriterTransaction(
                    QByteArrayLiteral("GradientModel::deleteGradient"));

            QmlDesigner::QmlObjectNode gradientNode = modelNode
                                                          .nodeProperty(gradientPropertyName().toUtf8())
                                                          .modelNode();
            if (gradientNode.isValid())
                gradientNode.destroy();
        }
    }
}

bool GradientModel::hasPercentageGradientProperty(const QString &propertyName) const
{
    bool result = false;
    getPercentageGradientProperty(propertyName.toUtf8(), &result);
    return result;
}

qreal GradientModel::getPercentageGradientProperty(const QmlDesigner::PropertyNameView propertyName,
                                                   bool *ok) const
{
    if (ok != nullptr)
        *ok = false;

    if (!m_itemNode.isValid())
        return 0.;

    //valid format is itemName1.property * 0.1
    //we are interested in parent. or parentName. items
    //and in width and height properties as of now
    //looking for something that starts with "itemName1.property"
    //looking for something like "* 0.223"
    const QmlDesigner::TypeName gradientPropertyTypeName = gradientPropertyName().toUtf8();

    const QmlDesigner::ModelNode gradientModel = m_itemNode.modelNode()
                                                     .nodeProperty(gradientPropertyTypeName)
                                                     .modelNode();

    if (!gradientModel)
        return 0.;

    if (const auto bindingProperty = gradientModel.bindingProperty(propertyName.toByteArray())) {
        const auto defaultGradient = getDefaultGradientPropertyData(propertyName, m_gradientTypeName);
        const auto expectedParentProperty = defaultGradient.bindingProperty;

        const QString expression = bindingProperty.expression();
        const QStringList splitExpression = expression.split("*", Qt::SkipEmptyParts);
        if (splitExpression.length() == 2 && !expectedParentProperty.isEmpty()) {
            const QString parentStr = splitExpression.at(0).trimmed();
            const QString percentageStr = splitExpression.at(1).trimmed();

            bool validStatement = false;

            if (!parentStr.isEmpty()) {
                const QStringList splitParent = parentStr.split(".", Qt::SkipEmptyParts);
                if (splitParent.length() == 2) {
                    const QString itemId = splitParent.at(0).trimmed();
                    const QString itemProp = splitParent.at(1).trimmed();
                    const QString realParentId = m_itemNode.modelNode().hasId() ? m_itemNode.id() : "";
                    if (!itemId.isEmpty() && !itemProp.isEmpty() && (itemId == realParentId)
                        && (itemProp.toUtf8() == expectedParentProperty)) {
                        validStatement = true;
                    }
                }
            }

            if (!percentageStr.isEmpty() && validStatement) {
                const qreal percentage = percentageStr.toFloat(ok);
                return percentage;
            }
        }
    }

    return 0.;
}

QVariant GradientModel::getGradientPropertyVariant(const QString &propertyName) const
{
    if (!m_itemNode.isValid())
        return {};

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    if (!gradient.isValid())
        return {};

    return gradient.modelValue(propertyName.toUtf8());
}

ShapeGradientPropertyData GradientModel::getDefaultGradientPropertyData(
    const QmlDesigner::PropertyNameView propertyName, const QStringView &gradientType) const
{
    const auto *gradData = getDefaultGradientData(propertyName, gradientType);
    if (gradData != nullptr)
        return *gradData;

    return {};
}

void GradientModel::setGradientProperty(const QString &propertyName, qreal value)
{
    QTC_ASSERT(m_itemNode.isValid(), return);

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    QTC_ASSERT(gradient.isValid(), return);

    try {
        gradient.setVariantProperty(propertyName.toUtf8(), value);
    } catch (const QmlDesigner::Exception &e) {
        e.showException();
    }
}

void GradientModel::setGradientPropertyPercentage(const QString &propertyName, qreal value)
{
    QTC_ASSERT(m_itemNode.isValid(), return);

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    QTC_ASSERT(gradient.isValid(), return);

    const auto gradientDefaultData = getDefaultGradientPropertyData(propertyName.toUtf8(),
                                                                    m_gradientTypeName);
    QTC_ASSERT(gradientDefaultData.canUsePercentage == ShapeGradientPropertyData::UsePercents::Yes,
               return);

    const QmlDesigner::PropertyNameView parentPropertyStr = gradientDefaultData.bindingProperty;
    QTC_ASSERT(!parentPropertyStr.isEmpty(), return);

    if (parentPropertyStr.isEmpty()
        || (gradientDefaultData.canUsePercentage == ShapeGradientPropertyData::UsePercents::No)) {
        return;
    }

    const QString parentId = m_itemNode.validId();
    const QString leftBinding = parentId + "." + parentPropertyStr;
    const QString expression = leftBinding + " * " + QString::number(value);

    try {
        gradient.setBindingProperty(propertyName.toUtf8(), expression);
    } catch (const QmlDesigner::Exception &e) {
        e.showException();
    }
}

void GradientModel::setGradientOrientation(Qt::Orientation value)
{
    QTC_ASSERT(m_itemNode.isValid(), return);

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    QTC_ASSERT(gradient.isValid(), return);

    try {
        QmlDesigner::EnumerationName name = value == Qt::Horizontal ? "Gradient.Horizontal"
                                                                    : "Gradient.Vertical";
        gradient.modelNode().variantProperty("orientation").setEnumeration(name);
    } catch (const QmlDesigner::Exception &e) {
        e.showException();
    }
}

void GradientModel::setGradientPropertyUnits(const QString &propertyName,
                                             GradientModel::GradientPropertyUnits value)
{
    //translate from previous units to the new unit system
    const bool toPixels = (value == GradientPropertyUnits::Pixels);
    const bool toPercentage = (value == GradientPropertyUnits::Percentage);
    const auto defaultGradientData = getDefaultGradientPropertyData(propertyName.toUtf8(),
                                                                    m_gradientTypeName);

    const QmlDesigner::PropertyNameView parentPropertyName = defaultGradientData.bindingProperty;
    if (parentPropertyName.isEmpty())
        return;

    const qreal parentPropertyValue = m_itemNode.instanceValue(parentPropertyName.toByteArray()).toReal();

    if (toPixels) {
        bool ok = false;
        const qreal percent = getPercentageGradientProperty(propertyName.toUtf8(), &ok);
        qreal pixelValue = 0.;

        if (!ok)
            pixelValue = defaultGradientData.getDefaultValue(m_itemNode);
        else
            pixelValue = parentPropertyValue * percent;

        if (qIsNaN(pixelValue) || qIsInf(pixelValue))
            pixelValue = 0.;

        setGradientProperty(propertyName, qRound(pixelValue));
    } else if (toPercentage) {
        const QVariant gradientProp = getGradientPropertyVariant(propertyName);
        bool ok = false;
        qreal pixels = gradientProp.toReal(&ok);
        qreal percentValue = 0.;

        if (gradientProp.isNull() || !gradientProp.isValid() || !ok)
            pixels = defaultGradientData.getDefaultValue(m_itemNode);

        if (qFuzzyIsNull(pixels) || qFuzzyIsNull(parentPropertyValue))
            percentValue = 0.;
        else
            percentValue = (pixels / parentPropertyValue);

        if (qIsNaN(percentValue) || qIsInf(percentValue))
            percentValue = 0.;

        setGradientPropertyPercentage(propertyName, percentValue);
    }
}

void GradientModel::setPresetByID(int presetID)
{
    const QGradient gradient(GradientPresetItem::createGradientFromPreset(
        static_cast<GradientPresetItem::Preset>(presetID)));
    const QList<QGradientStop> gradientStops = gradient.stops().toList();

    QList<qreal> stopsPositions;
    QList<QString> stopsColors;
    for (const QGradientStop &stop : gradientStops) {
        stopsPositions.append(stop.first);
        stopsColors.append(stop.second.name());
    }

    setPresetByStops(stopsPositions, stopsColors, gradientStops.size());
}

void GradientModel::setPresetByStops(const QList<qreal> &stopsPositions,
                                     const QList<QString> &stopsColors,
                                     int stopsCount,
                                     bool saveTransaction)
{
    if (m_locked)
        return;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return;

    QmlDesigner::RewriterTransaction transaction;
    if (saveTransaction)
        transaction = view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::setCustomPreset"));

    deleteGradientNode(false);

    if (!m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
        try {

            if (m_gradientTypeName != "Gradient")
                ensureShapesImport();

            QmlDesigner::ModelNode gradientNode = createGradientNode();

            m_itemNode.modelNode()
                .nodeProperty(gradientPropertyName().toUtf8())
                .reparentHere(gradientNode);

            for (int i = 0; i < stopsCount; i++) {
                QmlDesigner::ModelNode gradientStopNode = createGradientStopNode();
                gradientStopNode.variantProperty("position").setValue(stopsPositions.at(i));
                gradientStopNode.variantProperty("color").setValue(stopsColors.at(i));
                gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);
            }

        } catch (const QmlDesigner::Exception &e) {
            e.showException();
        }
    }
    setupModel();

    if (m_gradientTypeName != "Gradient")
        resetPuppet(); /*Unfortunately required */

    emit hasGradientChanged();
    emit gradientTypeChanged();
}

void GradientModel::savePreset()
{
    //preparing qgradient:
    QGradient currentGradient;
    QGradientStops currentStops;
    QGradientStop stop; //double, color

    for (int i = 0; i < rowCount(); i++) {
        stop.first = getPosition(i);
        stop.second = getColor(i);
        currentStops.append(stop);
    }
    currentGradient.setStops(currentStops);
    const GradientPresetItem item(currentGradient, "Custom Gradient");

    //reading the custom gradient file
    //filling the file with old data + new data
    const QString filename(GradientPresetCustomListModel::getFilename());
    QList<GradientPresetItem> items = GradientPresetCustomListModel::storedPresets(filename);
    items.append(item);
    GradientPresetCustomListModel::storePresets(filename, items);
}

void GradientModel::updateGradient()
{
    beginResetModel();

    QList<qreal> stops;
    QList<QString> colors;
    int stopsCount = rowCount();
    for (int i = 0; i < stopsCount; i++) {
        stops.append(getPosition(i));
        colors.append(getColor(i).name(QColor::HexArgb));
    }

    setPresetByStops(stops, colors, stopsCount, false);

    endResetModel();
}
