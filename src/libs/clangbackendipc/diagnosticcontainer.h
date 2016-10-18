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

    const Utf8String &text() const
    {
        return text_;
    }

    const Utf8String &category() const
    {
        return category_;
    }

    const Utf8String &enableOption() const
    {
        return enableOption_;
    }

    const Utf8String &disableOption() const
    {
        return disableOption_;
    }

    const SourceLocationContainer &location() const
    {
        return location_;
    }

    const QVector<SourceRangeContainer> &ranges() const
    {
        return ranges_;
    }

    DiagnosticSeverity severity() const
    {
        return severity_;
    }

    const QVector<FixItContainer> &fixIts() const
    {
        return fixIts_;
    }

    const QVector<DiagnosticContainer> &children() const
    {
        return children_;
    }

    friend QDataStream &operator<<(QDataStream &out, const DiagnosticContainer &container)
    {
        out << container.text_;
        out << container.category_;
        out << container.enableOption_;
        out << container.disableOption_;
        out << container.location_;
        out << static_cast<quint32>(container.severity_);
        out << container.ranges_;
        out << container.fixIts_;
        out << container.children_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DiagnosticContainer &container)
    {
        quint32 severity;

        in >> container.text_;
        in >> container.category_;
        in >> container.enableOption_;
        in >> container.disableOption_;
        in >> container.location_;
        in >> severity;
        in >> container.ranges_;
        in >> container.fixIts_;
        in >> container.children_;

        container.severity_ = static_cast<DiagnosticSeverity>(severity);

        return in;
    }

    friend bool operator==(const DiagnosticContainer &first, const DiagnosticContainer &second)
    {
        return first.text_ == second.text_
            && first.location_ == second.location_;
    }

    friend bool operator!=(const DiagnosticContainer &first, const DiagnosticContainer &second)
    {
        return !(first == second);
    }

private:
    SourceLocationContainer location_;
    QVector<SourceRangeContainer> ranges_;
    Utf8String text_;
    Utf8String category_;
    Utf8String enableOption_;
    Utf8String disableOption_;
    QVector<DiagnosticContainer> children_;
    QVector<FixItContainer> fixIts_;
    DiagnosticSeverity severity_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DiagnosticContainer &container);
void PrintTo(const DiagnosticContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd
