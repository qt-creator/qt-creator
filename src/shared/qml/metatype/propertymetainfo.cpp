#include "propertymetainfo.h"

#include <QSharedData>

#include "invalidmetainfoexception.h"
#include "metainfo.h"
#include <private/qmlvaluetype_p.h>

namespace Qml {
namespace Internal {

class PropertyMetaInfoData : public QSharedData
{
public:
    PropertyMetaInfoData()
        : QSharedData(),
          isValid(false),
          readable(false),
          writeable(false),
          resettable(false),
          enumType(false),
          flagType(false),
          isVisible(false)
    {}

    QString name;
    QString type;
    int majorVersion;
    int minorVersion;
    bool isValid;

    bool readable;
    bool writeable;
    bool resettable;

    bool enumType;
    bool flagType;
    bool isVisible;

    QHash<QString, QVariant> defaultValueHash;
};

} // namespace Internal

/*!
\class QKineticDesigner::PropertyMetaInfo
\ingroup CoreModel
\brief The PropertyMetaInfo class provides meta information about a qml type property.

A PropertyMetaInfo object can be NodeMetaInfo, or AbstractProperty::metaInfo.

The object can be invalid - you can check this by calling isValid().
The object is invalid if you ask for meta information for
an non-existing qml type. Also the node meta info can become invalid
if the type is deregistered from the meta type system (e.g.
a sub component qml file is deleted). Trying to call any accessor methods on an invalid
PropertyMetaInfo object will result in an InvalidMetaInfoException being thrown.


\see QKineticDesigner::MetaInfo, QKineticDesigner::NodeMetaInfo, QKineticDesigner::EnumeratorMetaInfo
*/

PropertyMetaInfo::PropertyMetaInfo()
    : m_data(new Internal::PropertyMetaInfoData)
{
}

PropertyMetaInfo::~PropertyMetaInfo()
{
}

/*!
  \brief Creates a copy of the handle.
  */
PropertyMetaInfo::PropertyMetaInfo(const PropertyMetaInfo &other)
    :  m_data(other.m_data)
{
}

/*!
  \brief Copies the handle.
  */
PropertyMetaInfo &PropertyMetaInfo::operator=(const PropertyMetaInfo &other)
{
    if (this != &other)
        m_data = other.m_data;

    return *this;
}

/*!
  \brief Returns whether the meta information system knows about this property.
  */
bool PropertyMetaInfo::isValid() const
{
    return m_data->isValid;
}

/*!
  \brief Returns the name of the property.
  */
QString PropertyMetaInfo::name() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->name;
}

/*!
  \brief Returns the type name of the property.
  */
QString PropertyMetaInfo::type() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->type;
}

int PropertyMetaInfo::typeMajorVersion() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->majorVersion;
}

int PropertyMetaInfo::typeMinorVersion() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->minorVersion;
}

bool PropertyMetaInfo::isVisibleToPropertyEditor() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->isVisible;
}

void PropertyMetaInfo::setIsVisibleToPropertyEditor(bool isVisible)
{
    m_data->isVisible = isVisible;
}

/*!
  \brief Returns the QVariant type of the property.
  */
QVariant::Type PropertyMetaInfo::variantTypeId() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    Q_ASSERT(!m_data->type.isEmpty());
    return QVariant::nameToType(m_data->type.toLatin1().data());
}

/*!
  \brief Returns whether the propery is readable.
  */
bool PropertyMetaInfo::isReadable() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->readable;
}

/*!
  \brief Returns whether the propery is writeable.
  */
bool PropertyMetaInfo::isWriteable() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->writeable;
}

/*!
  \brief Returns whether the propery is resettable.
  */
bool PropertyMetaInfo::isResettable() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->resettable;
}

/*!
  \brief Returns whether the propery is complex value type.
  */
bool PropertyMetaInfo::isValueType() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }

    QmlValueType *valueType(QmlValueTypeFactory::valueType(variantTypeId()));
    return valueType;
}

/*!
  \brief Returns whether the propery is a QmlList.
  */
bool PropertyMetaInfo::isListProperty() const
{
      if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }

    return type().contains("QmlList");
}

/*!
  \brief Returns whether the propery has sub properties with "." syntax e.g. font
  */
bool PropertyMetaInfo::hasDotSubProperties() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }

    return isValueType() || !isWriteable();
}

/*!
  \brief Returns whether the propery stores an enum value.
  */
bool PropertyMetaInfo::isEnumType() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->enumType;
}

/*!
  \brief Returns whether the propery stores a flag value.
  */
bool PropertyMetaInfo::isFlagType() const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }
    return m_data->flagType;
}

/*!
  \brief Returns a default value if there is one specified, an invalid QVariant otherwise.
  */
QVariant PropertyMetaInfo::defaultValue(const NodeMetaInfo &nodeMetaInfoArg) const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }

    QList<NodeMetaInfo> nodeMetaInfoList(nodeMetaInfoArg.superClasses());
    nodeMetaInfoList.prepend(nodeMetaInfoArg);
    foreach (const NodeMetaInfo &nodeMetaInfo, nodeMetaInfoList) {
        if (m_data->defaultValueHash.contains(nodeMetaInfo.typeName()))
            return m_data->defaultValueHash.value(nodeMetaInfo.typeName());
    }

    return QVariant();
}

void PropertyMetaInfo::setName(const QString &name)
{
    m_data->name = name;
}

void PropertyMetaInfo::setType(const QString &type, int majorVersion, int minorVersion)
{
    m_data->type = type;
    m_data->majorVersion = majorVersion;
    m_data->minorVersion = minorVersion;
}

void PropertyMetaInfo::setValid(bool isValid)
{
    m_data->isValid = isValid;
}

void PropertyMetaInfo::setReadable(bool isReadable)
{
    m_data->readable = isReadable;
}

void PropertyMetaInfo::setWritable(bool isWritable)
{
    m_data->writeable = isWritable;
}

void PropertyMetaInfo::setResettable(bool isRessetable)
{
    m_data->resettable = isRessetable;
}

void PropertyMetaInfo::setEnumType(bool isEnumType)
{
    m_data->enumType = isEnumType;
}

void PropertyMetaInfo::setFlagType(bool isFlagType)
{
    m_data->flagType = isFlagType;
}

void  PropertyMetaInfo::setDefaultValue(const NodeMetaInfo &nodeMetaInfo, const QVariant &value)
{
    m_data->defaultValueHash.insert(nodeMetaInfo.typeName(), value);
}

/*!
  \brief cast value type of QVariant parameter

  If the type of the passed variant does not correspond to type(), the method tries to convert
  the value according to QVariant::convert(). Returns a new QVariant with casted value type
  if successful, an invalid QVariant otherwise.

  \param variant the QVariant to take the value from
  \returns QVariant with aligned value type, or invalid QVariant
  */
QVariant PropertyMetaInfo::castedValue(const QVariant &originalVariant) const
{
    if (!isValid()) {
        Q_ASSERT_X(isValid(), Q_FUNC_INFO, "");
        throw InvalidMetaInfoException(__LINE__, Q_FUNC_INFO, __FILE__);
    }

    QVariant variant = originalVariant;
    if (m_data->enumType) {
        return variant;
    }

    QVariant::Type typeId = variantTypeId();

    if (typeId == QVariant::UserType && m_data->type == QLatin1String("QVariant")) {
        return variant;
    } else if (variant.type() == QVariant::List && variant.type() == QVariant::List) {
        // TODO: check the contents of the list
        return variant;
    } else if (type() == "var" || type() == "variant") {
        return variant;
    } else if (type() == "alias") {
        // TODO: The Qml compiler resolves the alias type. We probably should do the same.
        return variant;
    } else if (variant.convert(typeId)) {
        return variant;
    } else {
        return QVariant();
    }
}

} // namespace Qml
