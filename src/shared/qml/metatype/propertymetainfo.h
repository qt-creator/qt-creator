#ifndef PROPERTYMETAINFO_H
#define PROPERTYMETAINFO_H

#include <qml/qml_global.h>

#include <QString>
#include <QExplicitlySharedDataPointer>
#include <QVariant>

namespace Qml {

class MetaInfo;
class NodeMetaInfo;

namespace Internal {

class MetaInfoPrivate;
class MetaInfoParser;
class PropertyMetaInfoData;

}

class QML_EXPORT PropertyMetaInfo
{
    friend class Qml::Internal::MetaInfoPrivate;
    friend class Qml::Internal::MetaInfoParser;
    friend class Qml::MetaInfo;
    friend class Qml::NodeMetaInfo;
public:
    PropertyMetaInfo();
    ~PropertyMetaInfo();

    PropertyMetaInfo(const PropertyMetaInfo &other);
    PropertyMetaInfo& operator=(const PropertyMetaInfo &other);

    bool isValid() const;

    QString name() const;
    QString type() const;
    int typeMajorVersion() const;
    int typeMinorVersion() const;

    QVariant::Type variantTypeId() const;

    bool isReadable() const;
    bool isWriteable() const;
    bool isResettable() const;
    bool isValueType() const;
    bool isListProperty() const;

    bool isEnumType() const;
    bool isFlagType() const;

    QVariant defaultValue(const NodeMetaInfo &nodeMetaInfo) const;
    bool isVisibleToPropertyEditor() const;

    QVariant castedValue(const QVariant &variant) const;

private:
    void setName(const QString &name);
    void setType(const QString &type, int majorVersion, int minorVersion);
    void setValid(bool isValid);

    void setReadable(bool isReadable);
    void setWritable(bool isWritable);
    void setResettable(bool isRessetable);

    void setEnumType(bool isEnumType);
    void setFlagType(bool isFlagType);

    void setDefaultValue(const NodeMetaInfo &nodeMetaInfo, const QVariant &value);
    void setIsVisibleToPropertyEditor(bool isVisible);

    bool hasDotSubProperties() const;


private:
    QExplicitlySharedDataPointer<Internal::PropertyMetaInfoData> m_data;
};

} // namespace Qml


#endif // PROPERTYMETAINFO_H
