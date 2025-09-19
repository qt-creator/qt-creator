// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gradientmodel.h"

#include "gradientpresetcustomlistmodel.h"
#include "gradientpresetitem.h"
#include "propertyeditortracing.h"
#include "propertyeditorview.h"
#include "qmlanchorbindingproxy.h"

#include <abstractview.h>
#include <exception.h>
#include <modelutils.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <rewritertransaction.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <QScopeGuard>
#include <QTimer>

namespace {

using QmlDesigner::PropertyEditorTracing::category;

constexpr auto widthBinding = [](const QStringView nodeName) -> QString {
    return QString("%1.width").arg(nodeName);
};
constexpr auto heightBinding = [](const QStringView nodeName) -> QString {
    return QString("%1.height").arg(nodeName);
};
constexpr auto minBinding = [](const QStringView nodeName) -> QString {
    return QString("Math.min(%1.width, %1.height)").arg(nodeName);
};
constexpr auto emptyBinding = [](const QStringView) -> QString {
    return {};
};

using BindingStringFunctionVariant = std::variant<std::monostate,
                                                 decltype(widthBinding),
                                                 decltype(heightBinding),
                                                 decltype(minBinding),
                                                 decltype(emptyBinding)>;

constexpr auto widthBindingValue = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return node.instanceValue("width").toReal();
};
constexpr auto heightBindingValue = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return node.instanceValue("height").toReal();
};
constexpr auto minBindingValue = [](const QmlDesigner::QmlItemNode &node) -> qreal {
    return qMin(node.instanceValue("width").toReal(), node.instanceValue("height").toReal());
};
constexpr auto emptyBindingValue = [](const QmlDesigner::QmlItemNode &) -> qreal { return 0.0; };

using BindingValueFunctionVariant = std::variant<std::monostate,
                                                  decltype(widthBindingValue),
                                                  decltype(heightBindingValue),
                                                  decltype(minBindingValue),
                                                  decltype(emptyBindingValue)>;

} // namespace

class ShapeGradientPropertyData
{
public:
    enum class UsePercents { No, Yes };

    constexpr ShapeGradientPropertyData() = default;

    constexpr ShapeGradientPropertyData(QmlDesigner::PropertyNameView name,
                                        UsePercents canPercent,
                                        qreal defPercent,
                                        BindingStringFunctionVariant binVariant,
                                        BindingValueFunctionVariant binValueVariant)
        : name(name)
        , canUsePercentage(canPercent)
        , defaultPercent(defPercent)
        , m_bindingString(binVariant)
        , m_bindingValue(binValueVariant)
    {}

    QmlDesigner::PropertyNameView name;
    UsePercents canUsePercentage = UsePercents::No;
    qreal defaultPercent = 0.0;

private:
    BindingStringFunctionVariant m_bindingString;
    BindingValueFunctionVariant m_bindingValue;

public:
    QString getBindingString(const QStringView nodeId) const
    {
        return std::visit(
            [&](auto &func) -> QString {
                using Type = std::decay_t<decltype(func)>;
                if constexpr (std::is_same_v<Type, std::monostate>)
                    return {};
                else
                    return func(nodeId);
            },
            m_bindingString);
    }

    qreal getBindingValue(const QmlDesigner::QmlItemNode &node) const
    {
        return std::visit(
            [&](auto &func) -> qreal {
                using Type = std::decay_t<decltype(func)>;
                if constexpr (std::is_same_v<Type, std::monostate>)
                    return {};
                else
                    return func(node);
            },
            m_bindingValue);
    }

    qreal getDefaultValue(const QmlDesigner::QmlItemNode &itemNode) const
    {
        return getBindingValue(itemNode) * defaultPercent;
    }

    QString getDefaultPercentString(const QStringView nodeId) const
    {
        return QString("%1 * %2").arg(getBindingString(nodeId), QString::number(defaultPercent));
    }
};

constexpr QmlDesigner::PropertyNameView linearX1Str = "x1";
constexpr QmlDesigner::PropertyNameView linearX2Str = "x2";
constexpr QmlDesigner::PropertyNameView linearY1Str = "y1";
constexpr QmlDesigner::PropertyNameView linearY2Str = "y2";

constexpr QmlDesigner::PropertyNameView radialCenterRadiusStr = "centerRadius";
constexpr QmlDesigner::PropertyNameView radialCenterXStr = "centerX";
constexpr QmlDesigner::PropertyNameView radialCenterYStr = "centerY";
constexpr QmlDesigner::PropertyNameView radialFocalRadiusStr = "focalRadius";
constexpr QmlDesigner::PropertyNameView radialFocalXStr = "focalX";
constexpr QmlDesigner::PropertyNameView radialFocalYStr = "focalY";

constexpr QmlDesigner::PropertyNameView conicalAngleStr = "angle";
constexpr QmlDesigner::PropertyNameView conicalCenterXStr = "centerX";
constexpr QmlDesigner::PropertyNameView conicalCenterYStr = "centerY";

constexpr ShapeGradientPropertyData defaultLinearShapeGradients[] = {
    {linearX1Str,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.0,
     widthBinding,
     widthBindingValue},
    {linearX2Str,
     ShapeGradientPropertyData::UsePercents::Yes,
     1.0,
     widthBinding,
     widthBindingValue},
    {linearY1Str,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.0,
     heightBinding,
     heightBindingValue},
    {linearY2Str,
     ShapeGradientPropertyData::UsePercents::Yes,
     1.0,
     heightBinding,
     heightBindingValue}};

constexpr ShapeGradientPropertyData defaultRadialShapeGradients[] = {
    {radialCenterRadiusStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     minBinding,
     minBindingValue},
    {radialCenterXStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     widthBinding,
     widthBindingValue},
    {radialCenterYStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     heightBinding,
     heightBindingValue},
    {radialFocalRadiusStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.0,
     minBinding,
     minBindingValue},
    {radialFocalXStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     widthBinding,
     widthBindingValue},
    {radialFocalYStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     heightBinding,
     heightBindingValue}};

constexpr ShapeGradientPropertyData defaultConicalShapeGradients[] = {
    {conicalAngleStr,
     ShapeGradientPropertyData::UsePercents::No,
     0.0,
     emptyBinding,
     emptyBindingValue},
    {conicalCenterXStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     widthBinding,
     widthBindingValue},
    {conicalCenterYStr,
     ShapeGradientPropertyData::UsePercents::Yes,
     0.5,
     heightBinding,
     heightBindingValue}};

template<typename GradientArrayType>
const ShapeGradientPropertyData *findGradientInArray(const GradientArrayType &array,
                                                     const QmlDesigner::PropertyNameView propName)
{
    const auto found = std::ranges::find(array, propName, &std::iter_value_t<GradientArrayType>::name);
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
                     const QmlDesigner::QmlItemNode &node,
                     ShapeGradientPropertyData::UsePercents usePercents)
{
    std::for_each(std::begin(array), std::end(array), [&](const ShapeGradientPropertyData &a) {
        if (a.canUsePercentage == ShapeGradientPropertyData::UsePercents::Yes
            && usePercents == ShapeGradientPropertyData::UsePercents::Yes) {
            gradient.bindingProperty(a.name.toByteArray())
                .setExpression(a.getDefaultPercentString(node.id()));
        } else {
            gradient.variantProperty(a.name.toByteArray()).setValue(a.getDefaultValue(node));
        }
    });
}

GradientModel::GradientModel(QObject *parent) :
    QAbstractListModel(parent)
{
    NanotraceHR::Tracer tracer{"gradient model constructor", category()};
}

int GradientModel::rowCount(const QModelIndex & /*parent*/) const
{
    NanotraceHR::Tracer tracer{"gradient model row count", category()};

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
    NanotraceHR::Tracer tracer{"gradient model role names", category()};

    static QHash<int, QByteArray> roleNames{{Qt::UserRole + 1, "position"},
                                            {Qt::UserRole + 2, "color"},
                                            {Qt::UserRole + 3, "readOnly"},
                                            {Qt::UserRole + 4, "index"}};

    return roleNames;
}

QVariant GradientModel::data(const QModelIndex &index, int role) const
{
    NanotraceHR::Tracer tracer{"gradient model data", category()};

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
    NanotraceHR::Tracer tracer{"gradient model add stop", category()};

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
    NanotraceHR::Tracer tracer{"gradient model add gradient", category()};

    if (m_locked)
        return;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return;

    if (!m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {

        if (m_gradientTypeName != "Gradient")
            QmlDesigner::ModelUtils::ensureShapesImport(model());

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
    NanotraceHR::Tracer tracer{"gradient model set color", category()};

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
    NanotraceHR::Tracer tracer{"gradient model set position", category()};

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
    NanotraceHR::Tracer tracer{"gradient model get color", category()};

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
    NanotraceHR::Tracer tracer{"gradient model get position", category()};

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
    NanotraceHR::Tracer tracer{"gradient model remove stop", category()};

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
    NanotraceHR::Tracer tracer{"gradient model delete gradient", category()};

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
    NanotraceHR::Tracer tracer{"gradient model lock", category()};

    m_locked = true;
}

void GradientModel::unlock()
{
    NanotraceHR::Tracer tracer{"gradient model unlock", category()};

    m_locked = false;
}

void GradientModel::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"gradient model register declarative type", category()};

    qmlRegisterType<GradientModel>("HelperWidgets", 2, 0, "GradientModel");
}

qreal GradientModel::readGradientProperty(const QString &propertyName) const
{
    NanotraceHR::Tracer tracer{"gradient model read gradient property", category()};

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
    NanotraceHR::Tracer tracer{"gradient model read gradient property percentage", category()};

    return getPercentageGradientProperty(propertyName.toUtf8());
}

QString GradientModel::readGradientOrientation() const
{
    NanotraceHR::Tracer tracer{"gradient model read gradient orientation", category()};

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
    NanotraceHR::Tracer tracer{"gradient model read gradient property units", category()};

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
    NanotraceHR::Tracer tracer{"gradient model setup model", category()};

    m_locked = true;
    const QScopeGuard cleanup([&] { m_locked = false; });

    beginResetModel();
    endResetModel();
}

void GradientModel::setAnchorBackend(const QVariant &anchorBackend)
{
    NanotraceHR::Tracer tracer{"gradient model set anchor backend", category()};

    auto anchorBackendObject = anchorBackend.value<QObject *>();

    const auto backendCasted =
            qobject_cast<const QmlDesigner::QmlAnchorBindingProxy *>(anchorBackendObject);

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
    NanotraceHR::Tracer tracer{"gradient model gradient property name", category()};

    return m_gradientPropertyName;
}

void GradientModel::setGradientPropertyName(const QString &name)
{
    NanotraceHR::Tracer tracer{"gradient model set gradient property name", category()};

    m_gradientPropertyName = name;
}

QString GradientModel::gradientTypeName() const
{
    NanotraceHR::Tracer tracer{"gradient model gradient type name", category()};

    return m_gradientTypeName;
}

void GradientModel::setGradientTypeName(const QString &name)
{
    NanotraceHR::Tracer tracer{"gradient model set gradient type name", category()};

    m_gradientTypeName = name;
}

bool GradientModel::hasGradient() const
{
    NanotraceHR::Tracer tracer{"gradient model has gradient", category()};

    return m_itemNode.isValid()
           && m_itemNode.modelNode().hasProperty(gradientPropertyName().toUtf8());
}

bool GradientModel::locked() const
{
    NanotraceHR::Tracer tracer{"gradient model locked", category()};

    if (m_locked)
        return true;

    auto editorView = qobject_cast<QmlDesigner::PropertyEditorView*>(view());

    return editorView && editorView->locked();
}

void GradientModel::setupGradientProperties(const QmlDesigner::ModelNode &gradient)
{
    NanotraceHR::Tracer tracer{"gradient model setup gradient properties", category()};

    QTC_ASSERT(m_itemNode.isValid(), return);

    QTC_ASSERT(gradient.isValid(), return);

    if (m_gradientTypeName == u"Gradient") {
        gradient.variantProperty("orientation").setEnumeration("Gradient.Vertical");
    } else if (m_gradientTypeName == u"LinearGradient") {
        prepareGradient(defaultLinearShapeGradients,
                        gradient,
                        m_itemNode,
                        ShapeGradientPropertyData::UsePercents::Yes);
    } else if (m_gradientTypeName == u"RadialGradient") {
        prepareGradient(defaultRadialShapeGradients,
                        gradient,
                        m_itemNode,
                        ShapeGradientPropertyData::UsePercents::Yes);
    } else if (m_gradientTypeName == u"ConicalGradient") {
        prepareGradient(defaultConicalShapeGradients,
                        gradient,
                        m_itemNode,
                        ShapeGradientPropertyData::UsePercents::Yes);
    }
}

QmlDesigner::Model *GradientModel::model() const
{
    NanotraceHR::Tracer tracer{"gradient model model", category()};

    QTC_ASSERT(m_itemNode.isValid(), return nullptr);
    return m_itemNode.view()->model();
}

QmlDesigner::AbstractView *GradientModel::view() const
{
    NanotraceHR::Tracer tracer{"gradient model view", category()};

    QTC_ASSERT(m_itemNode.isValid(), return nullptr);
    return m_itemNode.view();
}

void GradientModel::resetPuppet()
{
    NanotraceHR::Tracer tracer{"gradient model reset puppet", category()};

    QTimer::singleShot(1000, view(), &QmlDesigner::AbstractView::resetPuppet);
}

QmlDesigner::ModelNode GradientModel::createGradientNode()
{
    NanotraceHR::Tracer tracer{"gradient model create gradient node", category()};

    QmlDesigner::TypeName typeName = m_gradientTypeName.toUtf8();
    auto gradientNode = view()->createModelNode(typeName);

    setupGradientProperties(gradientNode);

    return gradientNode;
}

QmlDesigner::ModelNode GradientModel::createGradientStopNode()
{
    NanotraceHR::Tracer tracer{"gradient model create gradient stop node", category()};

    return view()->createModelNode("GradientStop");
}

void GradientModel::deleteGradientNode(bool saveTransaction)
{
    NanotraceHR::Tracer tracer{"gradient model delete gradient node", category()};

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
    NanotraceHR::Tracer tracer{"gradient model has percentage gradient property", category()};

    bool result = false;
    getPercentageGradientProperty(propertyName.toUtf8(), &result);
    return result;
}

qreal GradientModel::getPercentageGradientProperty(const QmlDesigner::PropertyNameView propertyName,
                                                   bool *ok) const
{
    NanotraceHR::Tracer tracer{"gradient model get percentage gradient property", category()};

    if (ok != nullptr)
        *ok = false;

    if (!m_itemNode.isValid() || !m_itemNode.hasModelNode())
        return 0.;

    if (!m_itemNode.modelNode().hasId())
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
        const auto expectedBindingProperty = defaultGradient.getBindingString(m_itemNode.id());

        const QString expression = bindingProperty.expression();
        const QStringList splitExpression = expression.split("*", Qt::SkipEmptyParts);
        if (splitExpression.length() == 2 && !expectedBindingProperty.isEmpty()) {
            const QString parentStr = splitExpression.at(0).trimmed();
            const QString percentageStr = splitExpression.at(1).trimmed();

            if (!parentStr.isEmpty() && !percentageStr.isEmpty()) {
                if (parentStr == expectedBindingProperty) {
                    const qreal percentage = percentageStr.toFloat(ok);
                    return percentage;
                }
            }
        }
    }

    return 0.;
}

QVariant GradientModel::getGradientPropertyVariant(const QString &propertyName) const
{
    NanotraceHR::Tracer tracer{"gradient model get gradient property variant", category()};

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
    NanotraceHR::Tracer tracer{"gradient model get default gradient property data", category()};

    const auto *gradData = getDefaultGradientData(propertyName, gradientType);
    if (gradData != nullptr)
        return *gradData;

    return {};
}

void GradientModel::setGradientProperty(const QString &propertyName, qreal value)
{
    NanotraceHR::Tracer tracer{"gradient model set gradient property", category()};

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
    NanotraceHR::Tracer tracer{"gradient model set gradient property percentage", category()};

    QTC_ASSERT(m_itemNode.isValid(), return);

    QmlDesigner::QmlObjectNode gradient = m_itemNode.modelNode()
                                              .nodeProperty(gradientPropertyName().toUtf8())
                                              .modelNode();

    QTC_ASSERT(gradient.isValid(), return);

    const auto gradientDefaultData = getDefaultGradientPropertyData(propertyName.toUtf8(),
                                                                    m_gradientTypeName);
    QTC_ASSERT(gradientDefaultData.canUsePercentage == ShapeGradientPropertyData::UsePercents::Yes,
               return);

    const QString parentId = m_itemNode.validId();
    const QString bindingPropertyStr = gradientDefaultData.getBindingString(parentId);
    QTC_ASSERT(!bindingPropertyStr.isEmpty(), return);

    if (bindingPropertyStr.isEmpty()
        || (gradientDefaultData.canUsePercentage == ShapeGradientPropertyData::UsePercents::No)) {
        return;
    }
    const QString expression = bindingPropertyStr + " * " + QString::number(value);

    try {
        gradient.setBindingProperty(propertyName.toUtf8(), expression);
    } catch (const QmlDesigner::Exception &e) {
        e.showException();
    }
}

void GradientModel::setGradientOrientation(Qt::Orientation value)
{
    NanotraceHR::Tracer tracer{"gradient model set gradient orientation", category()};

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
    NanotraceHR::Tracer tracer{"gradient model set gradient property units", category()};

    //translate from previous units to the new unit system
    const bool toPixels = (value == GradientPropertyUnits::Pixels);
    const bool toPercentage = (value == GradientPropertyUnits::Percentage);
    const auto defaultGradientData = getDefaultGradientPropertyData(propertyName.toUtf8(),
                                                                    m_gradientTypeName);

    const QString parentPropertyName = defaultGradientData.getBindingString(m_itemNode.validId());
    if (parentPropertyName.isEmpty())
        return;

    const qreal parentPropertyValue = defaultGradientData.getBindingValue(m_itemNode);

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
    NanotraceHR::Tracer tracer{"gradient model set preset by ID", category()};

    const QGradient gradient(GradientPresetItem::createGradientFromPreset(
        static_cast<GradientPresetItem::Preset>(presetID)));
    const QList<QGradientStop> gradientStops = gradient.stops();

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
    NanotraceHR::Tracer tracer{"gradient model set preset by stops", category()};

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
                QmlDesigner::ModelUtils::ensureShapesImport(model());

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
    NanotraceHR::Tracer tracer{"gradient model save preset", category()};

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
    NanotraceHR::Tracer tracer{"gradient model update gradient", category()};

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
