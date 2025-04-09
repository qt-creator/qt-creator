// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/link.h>

#include <QMetaType>
#include <QString>

namespace ClangTools::Internal {

class ExplainingStep
{
public:
    bool isValid() const;
    bool operator<(const ExplainingStep &other) const {
        return std::tie(location, ranges, message)
               < std::tie(other.location, other.ranges, other.message);
    }

    QString message;
    Utils::Link location;
    Utils::Links ranges;
    bool isFixIt = false;
};

class Diagnostic
{
public:
    bool isValid() const;
    QIcon icon() const;

    friend bool operator==(const Diagnostic &lhs, const Diagnostic &rhs);
    friend size_t qHash(const Diagnostic &diagnostic);

    QString name;
    QString description;
    QString category;
    QString type;
    Utils::Link location;
    QList<ExplainingStep> explainingSteps;
    bool hasFixits = false;
};

using Diagnostics = QList<Diagnostic>;

} // namespace ClangTools::Internal

Q_DECLARE_METATYPE(ClangTools::Internal::Diagnostic)
