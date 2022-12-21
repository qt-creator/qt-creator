// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <exception.h>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT RemoveBaseStateException : public Exception
{
public:
    RemoveBaseStateException(int line,
                              const QByteArray &function,
                              const QByteArray &file);

    QString type() const override;

};
} // namespace QmlDesigner
