/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    QString name;
    QString description;
    QString category;
    QString type;
    Debugger::DiagnosticLocation location;
    QVector<ExplainingStep> explainingSteps;
    bool hasFixits = false;
};

bool operator==(const Diagnostic &lhs, const Diagnostic &rhs);

using Diagnostics = QList<Diagnostic>;

quint32 qHash(const Diagnostic &diagnostic);

} // namespace Internal
} // namespace ClangTools

Q_DECLARE_METATYPE(ClangTools::Internal::Diagnostic)
