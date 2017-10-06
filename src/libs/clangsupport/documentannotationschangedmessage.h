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

#include "clangsupport_global.h"
#include "diagnosticcontainer.h"
#include "filecontainer.h"
#include "highlightingmarkcontainer.h"
#include "sourcerangecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT DocumentAnnotationsChangedMessage
{
public:
    DocumentAnnotationsChangedMessage() = default;
    DocumentAnnotationsChangedMessage(const FileContainer &fileContainer,
                                      const QVector<DiagnosticContainer> &diagnostics,
                                      const DiagnosticContainer &firstHeaderErrorDiagnostic,
                                      const QVector<HighlightingMarkContainer> &highlightingMarks,
                                      const QVector<SourceRangeContainer> &skippedPreprocessorRanges)
        : m_fileContainer(fileContainer),
          m_diagnostics(diagnostics),
          m_firstHeaderErrorDiagnostic(firstHeaderErrorDiagnostic),
          m_highlightingMarks(highlightingMarks),
          m_skippedPreprocessorRanges(skippedPreprocessorRanges)
    {
    }

    const FileContainer &fileContainer() const
    {
        return m_fileContainer;
    }

    const QVector<DiagnosticContainer> &diagnostics() const
    {
        return m_diagnostics;
    }

    const DiagnosticContainer &firstHeaderErrorDiagnostic() const
    {
        return m_firstHeaderErrorDiagnostic;
    }

    const QVector<HighlightingMarkContainer> &highlightingMarks() const
    {
        return m_highlightingMarks;
    }

    const QVector<SourceRangeContainer> &skippedPreprocessorRanges() const
    {
        return m_skippedPreprocessorRanges;
    }

    friend QDataStream &operator<<(QDataStream &out, const DocumentAnnotationsChangedMessage &message)
    {
        out << message.m_fileContainer;
        out << message.m_diagnostics;
        out << message.m_firstHeaderErrorDiagnostic;
        out << message.m_highlightingMarks;
        out << message.m_skippedPreprocessorRanges;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DocumentAnnotationsChangedMessage &message)
    {
        in >> message.m_fileContainer;
        in >> message.m_diagnostics;
        in >> message.m_firstHeaderErrorDiagnostic;
        in >> message.m_highlightingMarks;
        in >> message.m_skippedPreprocessorRanges;

        return in;
    }

    friend bool operator==(const DocumentAnnotationsChangedMessage &first,
                    const DocumentAnnotationsChangedMessage &second)
    {
        return first.m_fileContainer == second.m_fileContainer
            && first.m_diagnostics == second.m_diagnostics
            && first.m_firstHeaderErrorDiagnostic == second.m_firstHeaderErrorDiagnostic
            && first.m_highlightingMarks == second.m_highlightingMarks
            && first.m_skippedPreprocessorRanges == second.m_skippedPreprocessorRanges;
    }

private:
    FileContainer m_fileContainer;
    QVector<DiagnosticContainer> m_diagnostics;
    DiagnosticContainer m_firstHeaderErrorDiagnostic;
    QVector<HighlightingMarkContainer> m_highlightingMarks;
    QVector<SourceRangeContainer> m_skippedPreprocessorRanges;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const DocumentAnnotationsChangedMessage &message);
std::ostream &operator<<(std::ostream &os, const DocumentAnnotationsChangedMessage &message);

DECLARE_MESSAGE(DocumentAnnotationsChangedMessage)
} // namespace ClangBackEnd
