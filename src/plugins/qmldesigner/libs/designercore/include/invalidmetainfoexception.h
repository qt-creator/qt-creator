// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exception.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT InvalidMetaInfoException : public Exception
{
public:
    InvalidMetaInfoException(
        const Sqlite::source_location &location = Sqlite::source_location::current());

    QString description() const override;
    QString type() const override;

};

}
