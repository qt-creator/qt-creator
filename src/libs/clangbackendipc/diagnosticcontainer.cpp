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

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

DiagnosticContainer::DiagnosticContainer(const Utf8String &text,
                                         const Utf8String &category,
                                         const std::pair<Utf8String,Utf8String> &options,
                                         DiagnosticSeverity severity,
                                         const SourceLocationContainer &location,
                                         const QVector<SourceRangeContainer> &ranges,
                                         const QVector<FixItContainer> &fixIts,
                                         const QVector<DiagnosticContainer> &children)
    : location_(location),
      ranges_(ranges),
      text_(text),
      category_(category),
      enableOption_(options.first),
      disableOption_(options.second),
      children_(children),
      fixIts_(fixIts),
      severity_(severity)
{
}

const Utf8String &DiagnosticContainer::text() const
{
    return text_;
}

const Utf8String &DiagnosticContainer::category() const
{
    return category_;
}

const Utf8String &DiagnosticContainer::enableOption() const
{
    return enableOption_;
}

const Utf8String &DiagnosticContainer::disableOption() const
{
    return disableOption_;
}

const SourceLocationContainer &DiagnosticContainer::location() const
{
    return location_;
}

const QVector<SourceRangeContainer> &DiagnosticContainer::ranges() const
{
    return ranges_;
}

DiagnosticSeverity DiagnosticContainer::severity() const
{
    return severity_;
}

const QVector<FixItContainer> &DiagnosticContainer::fixIts() const
{
    return fixIts_;
}

const QVector<DiagnosticContainer> &DiagnosticContainer::children() const
{
    return children_;
}

quint32 &DiagnosticContainer::severityAsInt()
{
    return reinterpret_cast<quint32&>(severity_);
}

QDataStream &operator<<(QDataStream &out, const DiagnosticContainer &container)
{
    out << container.text_;
    out << container.category_;
    out << container.enableOption_;
    out << container.disableOption_;
    out << container.location_;
    out << quint32(container.severity_);
    out << container.ranges_;
    out << container.fixIts_;
    out << container.children_;

    return out;
}

QDataStream &operator>>(QDataStream &in, DiagnosticContainer &container)
{
    in >> container.text_;
    in >> container.category_;
    in >> container.enableOption_;
    in >> container.disableOption_;
    in >> container.location_;
    in >> container.severityAsInt();
    in >> container.ranges_;
    in >> container.fixIts_;
    in >> container.children_;

    return in;
}

bool operator==(const DiagnosticContainer &first, const DiagnosticContainer &second)
{
    return first.text_ == second.text_
        && first.location_ == second.location_;
}

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

