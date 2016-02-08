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

#include "highlightingchangedmessage.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

HighlightingChangedMessage::HighlightingChangedMessage(const FileContainer &file,
        const QVector<HighlightingMarkContainer> &highlightingMarks,
        const QVector<SourceRangeContainer> &skippedPreprocessorRanges)
    : file_(file),
      highlightingMarks_(highlightingMarks),
      skippedPreprocessorRanges_(skippedPreprocessorRanges)
{
}

const FileContainer &HighlightingChangedMessage::file() const
{
    return file_;
}

const QVector<HighlightingMarkContainer> &HighlightingChangedMessage::highlightingMarks() const
{
    return highlightingMarks_;
}

const QVector<SourceRangeContainer> &HighlightingChangedMessage::skippedPreprocessorRanges() const
{
    return skippedPreprocessorRanges_;
}

QDataStream &operator<<(QDataStream &out, const HighlightingChangedMessage &message)
{
    out << message.file_;
    out << message.highlightingMarks_;
    out << message.skippedPreprocessorRanges_;

    return out;
}

QDataStream &operator>>(QDataStream &in, HighlightingChangedMessage &message)
{
    in >> message.file_;
    in >> message.highlightingMarks_;
    in >> message.skippedPreprocessorRanges_;

    return in;
}

bool operator==(const HighlightingChangedMessage &first, const HighlightingChangedMessage &second)
{
    return first.file_ == second.file_
        && first.highlightingMarks_ == second.highlightingMarks_
        && first.skippedPreprocessorRanges_ == second.skippedPreprocessorRanges_;
}

QDebug operator<<(QDebug debug, const HighlightingChangedMessage &message)
{
    debug.nospace() << "HighlightingChangedMessage("
                    << message.file_
                    << ", " << message.highlightingMarks_.size()
                    << ", " << message.skippedPreprocessorRanges_.size()
                    << ")";

    return debug;
}

void PrintTo(const HighlightingChangedMessage &message, ::std::ostream* os)
{
    *os << "HighlightingChangedMessage(";
    PrintTo(message.file(), os);
    *os << "," << message.highlightingMarks().size();
    *os << "," << message.skippedPreprocessorRanges().size();
    *os << ")";
}

} // namespace ClangBackEnd

