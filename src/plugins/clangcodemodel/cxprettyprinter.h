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

#ifndef CXPRETTYPRINTER_H
#define CXPRETTYPRINTER_H

#include <clang-c/Index.h>
#include <QString>

namespace ClangCodeModel {
namespace Internal {

class CXPrettyPrinter
{
public:
    CXPrettyPrinter();

    QString toString(CXCompletionChunkKind kind) const;
    QString toString(CXAvailabilityKind kind) const;
    QString toString(CXCursorKind kind) const;
    QString toString(CXDiagnosticSeverity severity) const;

    QString jsonForCompletionMeta(CXCodeCompleteResults *results);
    QString jsonForCompletionString(const CXCompletionString &string);
    QString jsonForCompletion(const CXCompletionResult &result);
    QString jsonForDiagnsotic(const CXDiagnostic &diagnostic);

private:
    int m_indent;
    QString m_printed;

    void writeCompletionContexts(CXCodeCompleteResults *results);
    void writeCompletionStringJson(const CXCompletionString &string);
    void writeCompletionChunkJson(const CXCompletionString &string, unsigned i);
    void writeCompletionAnnotationJson(const CXCompletionString &string, unsigned i);

    void writeDiagnosticJson(const CXDiagnostic &diag);
    void writeFixItJson(const CXDiagnostic &diag, unsigned i);

    void writeRangeJson(const CXSourceRange &range);
    void writeLocationJson(const CXSourceLocation &location);

    void writeLineEnd();
};

} // namespace Internal
} // namespace ClangCodeModel

#endif // CXPRETTYPRINTER_H
