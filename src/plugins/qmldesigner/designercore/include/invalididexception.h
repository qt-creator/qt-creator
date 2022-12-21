// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "invalidargumentexception.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT InvalidIdException : public InvalidArgumentException
{
public:
    enum Reason { InvalidCharacters, DuplicateId };

    InvalidIdException(int line,
                       const QByteArray &function,
                       const QByteArray &file,
                       const QByteArray &id,
                       Reason reason);

    InvalidIdException(int line,
                       const QByteArray &function,
                       const QByteArray &file,
                       const QByteArray &id,
                       const QByteArray &description);

    QString type() const override;
};

}
