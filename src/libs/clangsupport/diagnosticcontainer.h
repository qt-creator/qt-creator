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
        : location(location),
          ranges(ranges),
          text(text),
          category(category),
          enableOption(options.first),
          disableOption(options.second),
          children(children),
          fixIts(fixIts),
          severity(severity)
    {
        for (auto it = this->children.begin(); it != this->children.end(); ++it) {
            if (it->text == "note: remove constant to silence this warning") {
                this->children.erase(it);
                break;
            }
        }
    }

    friend QDataStream &operator<<(QDataStream &out, const DiagnosticContainer &container)
    {
        out << container.text;
        out << container.category;
        out << container.enableOption;
        out << container.disableOption;
        out << container.location;
        out << static_cast<quint32>(container.severity);
        out << container.ranges;
        out << container.fixIts;
        out << container.children;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DiagnosticContainer &container)
    {
        quint32 severity;

        in >> container.text;
        in >> container.category;
        in >> container.enableOption;
        in >> container.disableOption;
        in >> container.location;
        in >> severity;
        in >> container.ranges;
        in >> container.fixIts;
        in >> container.children;

        container.severity = static_cast<DiagnosticSeverity>(severity);

        return in;
    }

    friend bool operator==(const DiagnosticContainer &first, const DiagnosticContainer &second)
    {
        return first.text == second.text
            && first.location == second.location;
    }

    friend bool operator!=(const DiagnosticContainer &first, const DiagnosticContainer &second)
    {
        return !(first == second);
    }

public:
    SourceLocationContainer location;
    QVector<SourceRangeContainer> ranges;
    Utf8String text;
    Utf8String category;
    Utf8String enableOption;
    Utf8String disableOption;
    QVector<DiagnosticContainer> children;
    QVector<FixItContainer> fixIts;
    DiagnosticSeverity severity = DiagnosticSeverity::Ignored;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const DiagnosticContainer &container);

} // namespace ClangBackEnd
