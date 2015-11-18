/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "highlightingchangedmessage.h"

#include "container_common.h"

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

bool operator<(const HighlightingChangedMessage &first, const HighlightingChangedMessage &second)
{
    return first.file_ < second.file_
        && compareContainer(first.highlightingMarks_, second.highlightingMarks_)
        && compareContainer(first.skippedPreprocessorRanges_, second.skippedPreprocessorRanges_);
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

