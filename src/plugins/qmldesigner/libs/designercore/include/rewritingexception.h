// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "exception.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT RewritingException: public Exception
{
public:
    RewritingException(const QString &description,
                       const QString &documentTextContent,
                       const Sqlite::source_location &location = Sqlite::source_location::current());

    const char *what() const noexcept override;

    QString description() const override;
    QString type() const override;
    QString documentTextContent() const;

private:
    QString m_description;
    QString m_documentTextContent;
};

} // namespace QmlDesigner
