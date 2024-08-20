// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "notimplementedexception.h"

namespace QmlDesigner {

NotImplementedException::NotImplementedException(int line,
                                       const QByteArray &function,
                                       const QByteArray &file):
    Exception(line, function, file)
{
}

QString NotImplementedException::type() const
{
    return QLatin1String("NotImplementedException");
}
}
