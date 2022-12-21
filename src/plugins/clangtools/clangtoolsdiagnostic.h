// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/analyzer/diagnosticlocation.h>

#include <QMetaType>
#include <QString>
#include <QVector>

namespace ClangTools {
namespace Internal {

class ExplainingStep
{
public:
    bool isValid() const;
    bool operator<(const ExplainingStep &other) const {
        return std::tie(location, ranges, message)
               < std::tie(other.location, other.ranges, other.message);
    }

    QString message;
    Debugger::DiagnosticLocation location;
    QVector<Debugger::DiagnosticLocation> ranges;
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
    Debugger::DiagnosticLocation location;
    QVector<ExplainingStep> explainingSteps;
    bool hasFixits = false;
};

using Diagnostics = QList<Diagnostic>;

} // namespace Internal
} // namespace ClangTools

Q_DECLARE_METATYPE(ClangTools::Internal::Diagnostic)
