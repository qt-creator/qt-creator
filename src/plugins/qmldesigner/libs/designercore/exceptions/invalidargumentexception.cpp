// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidargumentexception.h"

#include <designercoretr.h>

#include <QString>
#include <QCoreApplication>

namespace QmlDesigner {

using namespace Qt::StringLiterals;

QString InvalidArgumentException::invalidArgumentDescription(const Sqlite::source_location &location,
                                                             const QString &argument)
{
    if (QLatin1StringView{location.file_name()} == "createNode"_L1)
        return DesignerCore::Tr::tr("Failed to create item of type %1.").arg(argument);

    return Exception::defaultDescription(location);
}

InvalidArgumentException::InvalidArgumentException(const QString &argument,
                                                   const Sqlite::source_location &location)
    : Exception(location)
    , m_argument{argument}
{
    createWarning();
}

const char *InvalidArgumentException::what() const noexcept
{
    return "InvalidArgumentException";
}

QString InvalidArgumentException::description() const
{
    if (QLatin1StringView{location().file_name()} == "createNode"_L1)
        return DesignerCore::Tr::tr("Failed to create item of type %1.").arg(m_argument);

    return Exception::defaultDescription(location());
}

/*!
    Returns the type of the exception as a string.
*/
QString InvalidArgumentException::type() const
{
    return "InvalidArgumentException"_L1;
}

/*!
    Returns the argument of the exception as a string.
*/
QString InvalidArgumentException::argument() const
{
    return m_argument;
}

}
