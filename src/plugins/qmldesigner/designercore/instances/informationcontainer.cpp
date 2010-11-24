#include "informationcontainer.h"

namespace QmlDesigner {

InformationContainer::InformationContainer()
    : m_instanceId(-1),
      m_name(NoName)
{
}

InformationContainer::InformationContainer(qint32 instanceId,
                                           InformationName name,
                                           const QVariant &information,
                                           const QVariant &secondInformation,
                                           const QVariant &thirdInformation)
    : m_instanceId(instanceId),
    m_name(name),
    m_information(information),
    m_secondInformation(secondInformation),
    m_thirdInformation(thirdInformation)
{
}

qint32 InformationContainer::instanceId() const
{
    return m_instanceId;
}

InformationName InformationContainer::name() const
{
    return InformationName(m_name);
}

QVariant InformationContainer::information() const
{
    return m_information;
}

QVariant InformationContainer::secondInformation() const
{
    return m_secondInformation;
}

QVariant InformationContainer::thirdInformation() const
{
    return m_thirdInformation;
}

QDataStream &operator<<(QDataStream &out, const InformationContainer &container)
{
    out << container.instanceId();//
    out << container.m_name;
    out << container.information();
    out << container.secondInformation();
    out << container.thirdInformation();

    return out;
}

QDataStream &operator>>(QDataStream &in, InformationContainer &container)
{

    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_information;
    in >> container.m_secondInformation;
    in >> container.m_thirdInformation;

    return in;
}

} // namespace QmlDesigner
