// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "invalidmetainfoexception.h"

namespace QmlDesigner {

using namespace Qt::StringLiterals;

InvalidMetaInfoException::InvalidMetaInfoException(const Sqlite::source_location &location)
    : Exception(location)
{
    createWarning();
}

QString InvalidMetaInfoException::description() const
{
    return defaultDescription(location());
}

/*!
    Returns the type of this exception as a string.
*/
QString InvalidMetaInfoException::type() const
{
    return "InvalidMetaInfoException"_L1;
}

}
