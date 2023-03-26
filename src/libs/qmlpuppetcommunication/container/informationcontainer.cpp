// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "informationcontainer.h"

#include <QtDebug>

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

bool operator ==(const InformationContainer &first, const InformationContainer &second)
{
    return first.m_instanceId == second.m_instanceId
            && first.m_name == second.m_name
            && first.m_information == second.m_information
            && first.m_secondInformation == second.m_secondInformation
            && first.m_thirdInformation == second.m_thirdInformation;
}

static bool isFirstLessThenSecond(const QVariant &first, const QVariant &second)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0) || QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (first.userType() == second.userType()) {
        if (first.canConvert<QByteArray>())
            return first.value<QByteArray>() < second.value<QByteArray>();
    }

    return true;
#else
    return first < second;
#endif
}

bool operator <(const InformationContainer &first, const InformationContainer &second)
{
    return (first.m_instanceId < second.m_instanceId)
        || (first.m_instanceId == second.m_instanceId && first.m_name < second.m_name)
        || (first.m_instanceId == second.m_instanceId && first.m_name == second.m_name
            && isFirstLessThenSecond(first.m_information, second.m_information));
}

QDebug operator <<(QDebug debug, const InformationContainer &container)
{
    debug.nospace() << "InformationContainer("
                    << "instanceId: " << container.instanceId() << ", "
                    << "name: " << container.name() << ", "
                    << "information: " << container.information();

    if (container.secondInformation().isValid())
        debug.nospace() << ", " << "secondInformation: " << container.secondInformation();

    if (container.thirdInformation().isValid())
        debug.nospace() << ", " << "thirdInformation: " << container.thirdInformation();

    return debug.nospace() << ")";
}

} // namespace QmlDesigner
