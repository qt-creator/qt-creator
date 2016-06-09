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
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const DocumentAnnotationsChangedMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, DocumentAnnotationsChangedMessage &message);
    friend CMBIPC_EXPORT bool operator==(const DocumentAnnotationsChangedMessage &first, const DocumentAnnotationsChangedMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DocumentAnnotationsChangedMessage &message);
    friend void PrintTo(const DocumentAnnotationsChangedMessage &message, ::std::ostream* os);

public:
    DocumentAnnotationsChangedMessage() = default;
    DocumentAnnotationsChangedMessage(const FileContainer &fileContainer,
                                      const QVector<DiagnosticContainer> &diagnostics,
                                      const QVector<HighlightingMarkContainer> &highlightingMarks,
                                      const QVector<SourceRangeContainer> &skippedPreprocessorRanges);

    const FileContainer &fileContainer() const;
    const QVector<DiagnosticContainer> &diagnostics() const;
    const QVector<HighlightingMarkContainer> &highlightingMarks() const;
    const QVector<SourceRangeContainer> &skippedPreprocessorRanges() const;

private:
    FileContainer fileContainer_;
    QVector<DiagnosticContainer> diagnostics_;
    QVector<HighlightingMarkContainer> highlightingMarks_;
    QVector<SourceRangeContainer> skippedPreprocessorRanges_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const DocumentAnnotationsChangedMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, DocumentAnnotationsChangedMessage &message);
CMBIPC_EXPORT bool operator==(const DocumentAnnotationsChangedMessage &first, const DocumentAnnotationsChangedMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DocumentAnnotationsChangedMessage &message);
void PrintTo(const DocumentAnnotationsChangedMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(DocumentAnnotationsChangedMessage)
} // namespace ClangBackEnd
