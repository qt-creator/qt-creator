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
#include "tokeninfocontainer.h"
#include "sourcerangecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class CLANGSUPPORT_EXPORT AnnotationsMessage
{
public:
    AnnotationsMessage() = default;
    // For pure token infos update
    AnnotationsMessage(const FileContainer &fileContainer,
                                      const QVector<TokenInfoContainer> &tokenInfos)
        : fileContainer(fileContainer),
          tokenInfos(tokenInfos),
          onlyTokenInfos(true)
    {
    }
    AnnotationsMessage(const FileContainer &fileContainer,
                       const QVector<DiagnosticContainer> &diagnostics,
                       const DiagnosticContainer &firstHeaderErrorDiagnostic,
                       const QVector<TokenInfoContainer> &tokenInfos,
                       const QVector<SourceRangeContainer> &skippedPreprocessorRanges)
        : fileContainer(fileContainer)
        , tokenInfos(tokenInfos)
        , diagnostics(diagnostics)
        , firstHeaderErrorDiagnostic(firstHeaderErrorDiagnostic)
        , skippedPreprocessorRanges(skippedPreprocessorRanges)
    {
    }

    friend QDataStream &operator<<(QDataStream &out, const AnnotationsMessage &message)
    {
        out << message.onlyTokenInfos;
        out << message.fileContainer;
        out << message.tokenInfos;
        if (message.onlyTokenInfos)
            return out;
        out << message.diagnostics;
        out << message.firstHeaderErrorDiagnostic;
        out << message.skippedPreprocessorRanges;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, AnnotationsMessage &message)
    {
        in >> message.onlyTokenInfos;
        in >> message.fileContainer;
        in >> message.tokenInfos;
        if (message.onlyTokenInfos)
            return in;
        in >> message.diagnostics;
        in >> message.firstHeaderErrorDiagnostic;
        in >> message.skippedPreprocessorRanges;

        return in;
    }

    friend bool operator==(const AnnotationsMessage &first, const AnnotationsMessage &second)
    {
        return first.fileContainer == second.fileContainer
            && first.diagnostics == second.diagnostics
            && first.firstHeaderErrorDiagnostic == second.firstHeaderErrorDiagnostic
            && first.tokenInfos == second.tokenInfos
            && first.skippedPreprocessorRanges == second.skippedPreprocessorRanges;
    }

public:
    FileContainer fileContainer;
    QVector<TokenInfoContainer> tokenInfos;
    QVector<DiagnosticContainer> diagnostics;
    DiagnosticContainer firstHeaderErrorDiagnostic;
    QVector<SourceRangeContainer> skippedPreprocessorRanges;
    bool onlyTokenInfos = false;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const AnnotationsMessage &message);

DECLARE_MESSAGE(AnnotationsMessage)
} // namespace ClangBackEnd
