// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exception.h"


namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT InvalidArgumentException : public Exception
{
public:
    InvalidArgumentException(int line,
                             const QByteArray &function,
                             const QByteArray &file,
                             const QByteArray &argument);

    QString type() const override;
    QString argument() const;

protected:
    InvalidArgumentException(int line,
                             const QByteArray &function,
                             const QByteArray &file,
                             const QByteArray &argument,
                             const QString &description);
    static QString invalidArgumentDescription(int line,
                                              const QByteArray &function,
                                              const QByteArray &file,
                                              const QByteArray &argument);
private:
    const QString m_argument;
};

}
