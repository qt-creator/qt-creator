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

#include "diagnosticcontainer.h"

#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

static const char *severityToText(DiagnosticSeverity severity)
{
    switch (severity) {
        case DiagnosticSeverity::Ignored: return "Ignored";
        case DiagnosticSeverity::Note: return "Note";
        case DiagnosticSeverity::Warning: return "Warning";
        case DiagnosticSeverity::Error: return "Error";
        case DiagnosticSeverity::Fatal: return "Fatal";
    }

    Q_UNREACHABLE();
}

QDebug operator<<(QDebug debug, const DiagnosticContainer &container)
{
    debug.nospace() << "DiagnosticContainer("
                    << container.text() << ", "
                    << container.category() << ", "
                    << container.enableOption() << ", "
                    << container.disableOption() << ", "
                    << container.location() << ", "
                    << container.ranges() << ", "
                    << container.fixIts() << ", "
                    << container.children()
                    << ")";

    return debug;
}

void PrintTo(const DiagnosticContainer &container, ::std::ostream* os)
{
    *os << severityToText(container.severity()) << ": "
        << container.text().constData() << ", "
        << container.category().constData() << ", "
        << container.enableOption().constData() << ", ";
    PrintTo(container.location(), os);
    *os   << "[";
    for (const auto &range : container.ranges())
        PrintTo(range, os);
    *os   << "], ";
    *os   << "[";
    for (const auto &fixIt : container.fixIts())
        PrintTo(fixIt, os);
    *os   << "], ";
    *os   << "[";
    for (const auto &child : container.children())
        PrintTo(child, os);
    *os   << "], ";
    *os   << ")";
}

} // namespace ClangBackEnd

