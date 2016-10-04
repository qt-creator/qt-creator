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

#include "clangbackendipc_global.h"
#include "diagnosticcontainer.h"
#include "filecontainer.h"
#include "highlightingmarkcontainer.h"
#include "sourcerangecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT DocumentAnnotationsChangedMessage
{
public:
    DocumentAnnotationsChangedMessage() = default;
    DocumentAnnotationsChangedMessage(const FileContainer &fileContainer,
                                      const QVector<DiagnosticContainer> &diagnostics,
                                      const DiagnosticContainer &firstHeaderErrorDiagnostic_,
                                      const QVector<HighlightingMarkContainer> &highlightingMarks,
                                      const QVector<SourceRangeContainer> &skippedPreprocessorRanges)
        : fileContainer_(fileContainer),
          diagnostics_(diagnostics),
          firstHeaderErrorDiagnostic_(firstHeaderErrorDiagnostic_),
          highlightingMarks_(highlightingMarks),
          skippedPreprocessorRanges_(skippedPreprocessorRanges)
    {
    }

    const FileContainer &fileContainer() const
    {
        return fileContainer_;
    }

    const QVector<DiagnosticContainer> &diagnostics() const
    {
        return diagnostics_;
    }

    const DiagnosticContainer &firstHeaderErrorDiagnostic() const
    {
        return firstHeaderErrorDiagnostic_;
    }

    const QVector<HighlightingMarkContainer> &highlightingMarks() const
    {
        return highlightingMarks_;
    }

    const QVector<SourceRangeContainer> &skippedPreprocessorRanges() const
    {
        return skippedPreprocessorRanges_;
    }

    friend QDataStream &operator<<(QDataStream &out, const DocumentAnnotationsChangedMessage &message)
    {
        out << message.fileContainer_;
        out << message.diagnostics_;
        out << message.firstHeaderErrorDiagnostic_;
        out << message.highlightingMarks_;
        out << message.skippedPreprocessorRanges_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DocumentAnnotationsChangedMessage &message)
    {
        in >> message.fileContainer_;
        in >> message.diagnostics_;
        in >> message.firstHeaderErrorDiagnostic_;
        in >> message.highlightingMarks_;
        in >> message.skippedPreprocessorRanges_;

        return in;
    }

    friend bool operator==(const DocumentAnnotationsChangedMessage &first,
                    const DocumentAnnotationsChangedMessage &second)
    {
        return first.fileContainer_ == second.fileContainer_
            && first.diagnostics_ == second.diagnostics_
            && first.firstHeaderErrorDiagnostic_ == second.firstHeaderErrorDiagnostic_
            && first.highlightingMarks_ == second.highlightingMarks_
            && first.skippedPreprocessorRanges_ == second.skippedPreprocessorRanges_;
    }

private:
    FileContainer fileContainer_;
    QVector<DiagnosticContainer> diagnostics_;
    DiagnosticContainer firstHeaderErrorDiagnostic_;
    QVector<HighlightingMarkContainer> highlightingMarks_;
    QVector<SourceRangeContainer> skippedPreprocessorRanges_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DocumentAnnotationsChangedMessage &message);
void PrintTo(const DocumentAnnotationsChangedMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(DocumentAnnotationsChangedMessage)
} // namespace ClangBackEnd
