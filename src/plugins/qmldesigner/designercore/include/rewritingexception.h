// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exception.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT RewritingException: public Exception
{
public:
    RewritingException(int line,
                       const QByteArray &function,
                       const QByteArray &file,
                       const QByteArray &description,
                       const QString &documentTextContent);

    QString type() const override;
    QString documentTextContent() const;

private:
    const QString m_documentTextContent;
};

} // namespace QmlDesigner
