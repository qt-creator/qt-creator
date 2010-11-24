#ifndef PROPERTYBINDINGCONTAINER_H
#define PROPERTYBINDINGCONTAINER_H

#include <QDataStream>
#include <qmetatype.h>
#include <QString>


namespace QmlDesigner {

class PropertyBindingContainer
{
    friend QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container);

public:
    PropertyBindingContainer();
    PropertyBindingContainer(qint32 instanceId, const QString &name, const QString &expression, const QString &dynamicTypeName);

    qint32 instanceId() const;
    QString name() const;
    QString expression() const;
    bool isDynamic() const;
    QString dynamicTypeName() const;

private:
    qint32 m_instanceId;
    QString m_name;
    QString m_expression;
    QString m_dynamicTypeName;
};

QDataStream &operator<<(QDataStream &out, const PropertyBindingContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyBindingContainer);
#endif // PROPERTYBINDINGCONTAINER_H
