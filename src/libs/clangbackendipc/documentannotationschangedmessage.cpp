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

#include "documentannotationschangedmessage.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

DocumentAnnotationsChangedMessage::DocumentAnnotationsChangedMessage(
        const FileContainer &file,
        const QVector<DiagnosticContainer> &diagnostics,
        const QVector<HighlightingMarkContainer> &highlightingMarks,
        const QVector<SourceRangeContainer> &skippedPreprocessorRanges)
    : fileContainer_(file),
      diagnostics_(diagnostics),
      highlightingMarks_(highlightingMarks),
      skippedPreprocessorRanges_(skippedPreprocessorRanges)
{
}

const FileContainer &DocumentAnnotationsChangedMessage::fileContainer() const
{
    return fileContainer_;
}

const QVector<DiagnosticContainer> &DocumentAnnotationsChangedMessage::diagnostics() const
{
    return diagnostics_;
}

const QVector<HighlightingMarkContainer> &DocumentAnnotationsChangedMessage::highlightingMarks() const
{
    return highlightingMarks_;
}

const QVector<SourceRangeContainer> &DocumentAnnotationsChangedMessage::skippedPreprocessorRanges() const
{
    return skippedPreprocessorRanges_;
}

QDataStream &operator<<(QDataStream &out, const DocumentAnnotationsChangedMessage &message)
{
    out << message.fileContainer_;
    out << message.diagnostics_;
    out << message.highlightingMarks_;
    out << message.skippedPreprocessorRanges_;

    return out;
}

QDataStream &operator>>(QDataStream &in, DocumentAnnotationsChangedMessage &message)
{
    in >> message.fileContainer_;
    in >> message.diagnostics_;
    in >> message.highlightingMarks_;
    in >> message.skippedPreprocessorRanges_;

    return in;
}

bool operator==(const DocumentAnnotationsChangedMessage &first,
                const DocumentAnnotationsChangedMessage &second)
{
    return first.fileContainer_ == second.fileContainer_
        && first.diagnostics_ == second.diagnostics_
        && first.highlightingMarks_ == second.highlightingMarks_
        && first.skippedPreprocessorRanges_ == second.skippedPreprocessorRanges_;
}

QDebug operator<<(QDebug debug, const DocumentAnnotationsChangedMessage &message)
{
    debug.nospace() << "DocumentAnnotationsChangedMessage("
                    << message.fileContainer_
                    << ", " << message.diagnostics_.size()
                    << ", " << message.highlightingMarks_.size()
                    << ", " << message.skippedPreprocessorRanges_.size()
                    << ")";

    return debug;
}

void PrintTo(const DocumentAnnotationsChangedMessage &message, ::std::ostream* os)
{
    *os << "DocumentAnnotationsChangedMessage(";
    PrintTo(message.fileContainer(), os);
    *os << "," << message.diagnostics().size();
    *os << "," << message.highlightingMarks().size();
    *os << "," << message.skippedPreprocessorRanges().size();
    *os << ")";
}

} // namespace ClangBackEnd

