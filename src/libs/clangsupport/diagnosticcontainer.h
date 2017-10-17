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

#include "sourcerangecontainer.h"
#include "fixitcontainer.h"

#include <QDataStream>
#include <QVector>

#include <utility>

namespace ClangBackEnd {

class DiagnosticContainer
{
public:
    DiagnosticContainer() = default;
    DiagnosticContainer(const Utf8String &text,
                        const Utf8String &category,
                        const std::pair<Utf8String,Utf8String> &options,
                        DiagnosticSeverity severity,
                        const SourceLocationContainer &location,
                        const QVector<SourceRangeContainer> &ranges,
                        const QVector<FixItContainer> &fixIts,
                        const QVector<DiagnosticContainer> &children)
        : m_location(location),
          m_ranges(ranges),
          m_text(text),
          m_category(category),
          m_enableOption(options.first),
          m_disableOption(options.second),
          m_children(children),
          m_fixIts(fixIts),
          m_severity(severity)
    {
    }

    const Utf8String &text() const
    {
        return m_text;
    }

    const Utf8String &category() const
    {
        return m_category;
    }

    const Utf8String &enableOption() const
    {
        return m_enableOption;
    }

    const Utf8String &disableOption() const
    {
        return m_disableOption;
    }

    const SourceLocationContainer &location() const
    {
        return m_location;
    }

    const QVector<SourceRangeContainer> &ranges() const
    {
        return m_ranges;
    }

    DiagnosticSeverity severity() const
    {
        return m_severity;
    }

    const QVector<FixItContainer> &fixIts() const
    {
        return m_fixIts;
    }

    const QVector<DiagnosticContainer> &children() const
    {
        return m_children;
    }

    friend QDataStream &operator<<(QDataStream &out, const DiagnosticContainer &container)
    {
        out << container.m_text;
        out << container.m_category;
        out << container.m_enableOption;
        out << container.m_disableOption;
        out << container.m_location;
        out << static_cast<quint32>(container.m_severity);
        out << container.m_ranges;
        out << container.m_fixIts;
        out << container.m_children;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DiagnosticContainer &container)
    {
        quint32 severity;

        in >> container.m_text;
        in >> container.m_category;
        in >> container.m_enableOption;
        in >> container.m_disableOption;
        in >> container.m_location;
        in >> severity;
        in >> container.m_ranges;
        in >> container.m_fixIts;
        in >> container.m_children;

        container.m_severity = static_cast<DiagnosticSeverity>(severity);

        return in;
    }

    friend bool operator==(const DiagnosticContainer &first, const DiagnosticContainer &second)
    {
        return first.m_text == second.m_text
            && first.m_location == second.m_location;
    }

    friend bool operator!=(const DiagnosticContainer &first, const DiagnosticContainer &second)
    {
        return !(first == second);
    }

private:
    SourceLocationContainer m_location;
    QVector<SourceRangeContainer> m_ranges;
    Utf8String m_text;
    Utf8String m_category;
    Utf8String m_enableOption;
    Utf8String m_disableOption;
    QVector<DiagnosticContainer> m_children;
    QVector<FixItContainer> m_fixIts;
    DiagnosticSeverity m_severity = DiagnosticSeverity::Ignored;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const DiagnosticContainer &container);
std::ostream &operator<<(std::ostream &os, const DiagnosticContainer &container);

} // namespace ClangBackEnd
