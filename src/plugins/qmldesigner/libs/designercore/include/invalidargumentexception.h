// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exception.h"


namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT InvalidArgumentException : public Exception
{
public:
    InvalidArgumentException(
        const QString &argument,
        const Sqlite::source_location &location = Sqlite::source_location::current());

    const char *what() const noexcept override;

    QString description() const override;
    QString type() const override;
    QString argument() const;

protected:
    static QString invalidArgumentDescription(const Sqlite::source_location &location,
                                              const QString &argument);

private:
    const QString m_argument;
};

}
