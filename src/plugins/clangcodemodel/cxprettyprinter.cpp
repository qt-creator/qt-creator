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

#include "cxprettyprinter.h"
#include "utils_p.h"
#include "cxraii.h"
#include <QStringList>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

CXPrettyPrinter::CXPrettyPrinter()
    : m_indent(0)
{
}

QString CXPrettyPrinter::toString(CXCompletionChunkKind kind) const
{
    switch (kind) {
    case CXCompletionChunk_Optional:
        return QLatin1String("Optional");
    case CXCompletionChunk_TypedText:
        return QLatin1String("TypedText");
    case CXCompletionChunk_Text:
        return QLatin1String("Text");
    case CXCompletionChunk_Placeholder:
        return QLatin1String("Placeholder");
    case CXCompletionChunk_Informative:
        return QLatin1String("Informative");
    case CXCompletionChunk_CurrentParameter:
        return QLatin1String("CurrentParameter");
    case CXCompletionChunk_LeftParen:
        return QLatin1String("LeftParen");
    case CXCompletionChunk_RightParen:
        return QLatin1String("RightParen");
    case CXCompletionChunk_LeftBracket:
        return QLatin1String("LeftBracket");
    case CXCompletionChunk_RightBracket:
        return QLatin1String("RightBracket");
    case CXCompletionChunk_LeftBrace:
        return QLatin1String("LeftBrace");
    case CXCompletionChunk_RightBrace:
        return QLatin1String("RightBrace");
    case CXCompletionChunk_LeftAngle:
        return QLatin1String("LeftAngle");
    case CXCompletionChunk_RightAngle:
        return QLatin1String("RightAngle");
    case CXCompletionChunk_Comma:
        return QLatin1String("Comma");
    case CXCompletionChunk_ResultType:
        return QLatin1String("ResultType");
    case CXCompletionChunk_Colon:
        return QLatin1String("Colon");
    case CXCompletionChunk_SemiColon:
        return QLatin1String("SemiColon");
    case CXCompletionChunk_Equal:
        return QLatin1String("Equal");
    case CXCompletionChunk_HorizontalSpace:
        return QLatin1String("HorizontalSpace");
    case CXCompletionChunk_VerticalSpace:
        return QLatin1String("VerticalSpace");
    default:
        return QLatin1String("<UNKNOWN>");
    }
}

QString CXPrettyPrinter::toString(CXAvailabilityKind kind) const
{
    switch (kind) {
    case CXAvailability_Available:
        return QLatin1String("Available");
    case CXAvailability_Deprecated:
        return QLatin1String("Deprecated");
    case CXAvailability_NotAccessible:
        return QLatin1String("NotAccessible");
    case CXAvailability_NotAvailable:
        return QLatin1String("NotAvailable");
    default:
        return QLatin1String("<UNKNOWN>");
    }
}

QString CXPrettyPrinter::toString(CXCursorKind kind) const
{
    return getQString(clang_getCursorKindSpelling(kind));
}

QString CXPrettyPrinter::toString(CXDiagnosticSeverity severity) const
{
    switch (severity)
    {
    case CXDiagnostic_Ignored:
        return QLatin1String("Ignored");
    case CXDiagnostic_Note:
        return QLatin1String("Note");
    case CXDiagnostic_Warning:
        return QLatin1String("Warning");
    case CXDiagnostic_Error:
        return QLatin1String("Error");
    case CXDiagnostic_Fatal:
        return QLatin1String("Fatal");
    default:
        return QLatin1String("<UNKNOWN>");
    }
}

QString CXPrettyPrinter::jsonForCompletionMeta(CXCodeCompleteResults *results)
{
    QString json;
    m_printed.swap(json);
    m_indent = 0;

    m_printed += QLatin1String("CXCodeCompleteResults {");
    m_indent += 4;

    CXCursorKind containerKind = clang_codeCompleteGetContainerKind(results, NULL);
    writeLineEnd();
    m_printed += QLatin1String("'container CursorKind': ");
    m_printed += toString(containerKind);
    m_printed += QLatin1Char(',');

    QString containerUSR(Internal::getQString(clang_codeCompleteGetContainerUSR(results)));
    if (!containerUSR.isEmpty()) {
        writeLineEnd();
        m_printed += QLatin1String("'container USR': ");
        m_printed += containerUSR;
        m_printed += QLatin1Char(',');
    }

    QString objCSelector(Internal::getQString(clang_codeCompleteGetObjCSelector(results)));
    if (!objCSelector.isEmpty()) {
        writeLineEnd();
        m_printed += QLatin1String("'Objective-C selector': ");
        m_printed += objCSelector;
        m_printed += QLatin1Char(',');
    }

    writeLineEnd();
    m_printed += QLatin1String("'contexts': [");
    m_indent += 4;
    writeCompletionContexts(results);
    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char(']');

    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char('}');

    m_printed.swap(json);
    return json;
}

QString CXPrettyPrinter::jsonForCompletionString(const CXCompletionString &string)
{
    QString json;
    m_printed.swap(json);
    m_indent = 0;

    m_printed += QLatin1String("CXCompletionString: ");
    writeCompletionStringJson(string);

    m_printed.swap(json);
    return json;
}

QString CXPrettyPrinter::jsonForCompletion(const CXCompletionResult &result)
{
    QString json;
    m_printed.swap(json);
    m_indent = 4;

    m_printed += QLatin1String("CXCompletionResult: {\n"
                               "    CompletionString: ");
    writeCompletionStringJson(result.CompletionString);
    m_printed += QLatin1Char('\n');

    m_printed += QLatin1String("    CursorKind: ");
    m_printed += toString(result.CursorKind);
    m_printed += QLatin1String(";\n}");

    m_printed.swap(json);
    return json;
}

/**
 * @brief CXPrettyPrinter::jsonForDiagnsotic
 * @param diagnostic
 * @return
 *
 * List of used clang-c API calls:
 *   CXDiagnosticSet clang_getChildDiagnostics(CXDiagnostic D);
 *   CXSourceLocation clang_getDiagnosticLocation(CXDiagnostic);
 *   CXString clang_getDiagnosticOption(CXDiagnostic Diag,
 *                                      CXString *Disable);
 *   unsigned clang_getDiagnosticCategory(CXDiagnostic);
 *   CXString clang_getDiagnosticCategoryText(CXDiagnostic);
 *   unsigned clang_getDiagnosticNumRanges(CXDiagnostic);
 *   CXSourceRange clang_getDiagnosticRange(CXDiagnostic Diagnostic,
 *                                          unsigned Range);
 *   unsigned clang_getDiagnosticNumFixIts(CXDiagnostic Diagnostic);
 *   CXString clang_getDiagnosticFixIt(CXDiagnostic Diagnostic,
 *                                     unsigned FixIt,
 *                                     CXSourceRange *ReplacementRange);
 */
QString CXPrettyPrinter::jsonForDiagnsotic(const CXDiagnostic &diagnostic)
{
    QString json;
    m_printed.swap(json);
    m_indent = 0;

    m_printed += QLatin1String("CXDiagnostic: ");
    writeDiagnosticJson(diagnostic);

    m_printed.swap(json);
    return json;
}

void CXPrettyPrinter::writeCompletionContexts(CXCodeCompleteResults *results)
{
    quint64 contexts = clang_codeCompleteGetContexts(results);
    QStringList lines;

    if (contexts & CXCompletionContext_AnyType)
        lines << QLatin1String("'any type'");
    if (contexts & CXCompletionContext_AnyValue)
        lines << QLatin1String("'any value'");
    if (contexts & CXCompletionContext_ObjCObjectValue)
        lines << QLatin1String("'Objective-C object'");
    if (contexts & CXCompletionContext_ObjCSelectorValue)
        lines << QLatin1String("'Objective-C selector'");
    if (contexts & CXCompletionContext_CXXClassTypeValue)
        lines << QLatin1String("'C++ class'");
    if (contexts & CXCompletionContext_DotMemberAccess)
        lines << QLatin1String("'. member access'");
    if (contexts & CXCompletionContext_ArrowMemberAccess)
        lines << QLatin1String("'-> member access'");
    if (contexts & CXCompletionContext_ObjCPropertyAccess)
        lines << QLatin1String("'. Objective-C property access'");
    if (contexts & CXCompletionContext_EnumTag)
        lines << QLatin1String("'enum tag'");
    if (contexts & CXCompletionContext_UnionTag)
        lines << QLatin1String("'union tag'");
    if (contexts & CXCompletionContext_StructTag)
        lines << QLatin1String("'struct tag'");
    if (contexts & CXCompletionContext_ClassTag)
        lines << QLatin1String("'C++ class tag'");
    if (contexts & CXCompletionContext_Namespace)
        lines << QLatin1String("'namespace tag'");
    if (contexts & CXCompletionContext_NestedNameSpecifier)
        lines << QLatin1String("'C++ nested name specifier'");
    if (contexts & CXCompletionContext_ObjCInterface)
        lines << QLatin1String("'Objective-C interface'");
    if (contexts & CXCompletionContext_ObjCProtocol)
        lines << QLatin1String("'Objective-C protocol'");
    if (contexts & CXCompletionContext_ObjCCategory)
        lines << QLatin1String("'Objective-C category'");
    if (contexts & CXCompletionContext_ObjCInstanceMessage)
        lines << QLatin1String("'Objective-C instance message'");
    if (contexts & CXCompletionContext_ObjCClassMessage)
        lines << QLatin1String("'Objective-C class message'");
    if (contexts & CXCompletionContext_ObjCSelectorName)
        lines << QLatin1String("'Objective-C selector name'");
    if (contexts & CXCompletionContext_MacroName)
        lines << QLatin1String("'macro name'");
    if (contexts & CXCompletionContext_NaturalLanguage)
        lines << QLatin1String("'natural language'");

    foreach (const QString &line, lines) {
        writeLineEnd();
        m_printed += line + QLatin1Char(',');
    }
}

void CXPrettyPrinter::writeCompletionStringJson(const CXCompletionString &string)
{
    m_printed += QLatin1Char('{');
    writeLineEnd();

    // availability
    m_printed += QLatin1String("availability: ");
    m_printed += toString(clang_getCompletionAvailability(string));
    m_printed += QLatin1Char(';');
    writeLineEnd();

    // priority
    m_printed += QLatin1String("priority: ");
    m_printed += QString::number(clang_getCompletionPriority(string));
    m_printed += QLatin1Char(';');
    writeLineEnd();

    // parent
    m_printed += QLatin1String("parent: \'");
    m_printed += getQString(clang_getCompletionParent(string, NULL));
    m_printed += QLatin1String("\';");
    writeLineEnd();

    // chunks
    m_printed += QLatin1String("chunks: [");
    m_indent += 4;
    unsigned numChunks = clang_getNumCompletionChunks(string);
    for (unsigned i = 0; i < numChunks; ++i) {
        writeLineEnd();
        writeCompletionChunkJson(string, i);
    }
    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char(']');
    writeLineEnd();

    // annotation
    m_printed += QLatin1String("annotations: [");
    m_indent += 4;
    unsigned numAnns = clang_getCompletionNumAnnotations(string);
    for (unsigned i = 0; i < numAnns; ++i) {
        writeLineEnd();
        writeCompletionAnnotationJson(string, i);
    }
    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char(']');
    writeLineEnd();

    m_printed += QLatin1Char('}');
}

void CXPrettyPrinter::writeCompletionChunkJson(const CXCompletionString &string, unsigned i)
{
    QString text = getQString(clang_getCompletionChunkText(string, i));
    QString kind = toString(clang_getCompletionChunkKind(string, i));
    CXCompletionString optional = clang_getCompletionChunkCompletionString(string, i);

    m_printed += kind;
    m_printed += QLatin1String(": ");
    if (!text.isEmpty()) {
        m_printed += QLatin1Char('\'');
        m_printed += text;
        m_printed += QLatin1Char('\'');
    }
    if (optional != NULL) {
        if (!text.isEmpty())
            m_printed += QLatin1String(", ");
        m_indent += 4;
        writeCompletionStringJson(optional);
        m_indent -= 4;
    }
}

void CXPrettyPrinter::writeCompletionAnnotationJson(const CXCompletionString &string, unsigned i)
{
    m_printed += QLatin1Char('\'');
    m_printed += getQString(clang_getCompletionAnnotation(string, i));
    m_printed += QLatin1Char('\'');
}

void CXPrettyPrinter::writeDiagnosticJson(const CXDiagnostic &diag)
{
    m_printed += QLatin1Char('{');
    m_indent += 4;
    writeLineEnd();

    // message
    m_printed += QLatin1Char('\'');
    m_printed += getQString(clang_formatDiagnostic(diag, /*options*/ 0));
    m_printed += QLatin1Char('\'');
    writeLineEnd();

    // severity
    m_printed += QLatin1String("severity: ");
    m_printed += toString(clang_getDiagnosticSeverity(diag));
    writeLineEnd();

    // location
    m_printed += QLatin1String("location: ");
    writeLocationJson(clang_getDiagnosticLocation(diag));
    writeLineEnd();

    // fix-its
    unsigned numFixIts = clang_getDiagnosticNumFixIts(diag);
    if (numFixIts > 0) {
        m_printed += QLatin1String("FixIts: [");
        writeLineEnd();
        for (unsigned i = 0; i < numFixIts; ++i) {
            writeFixItJson(diag, i);
            writeLineEnd();
        }
        m_printed += QLatin1Char(']');
        writeLineEnd();
    }

    // clang CLI options
    CXString cxDisabler;
    QString enabler = getQString(clang_getDiagnosticOption(diag, &cxDisabler));
    QString disabler = getQString(cxDisabler);
    if (!enabler.isEmpty()) {
        m_printed += QLatin1String("enabledBy: \'");
        m_printed += enabler;
        m_printed += QLatin1String("';");
        writeLineEnd();
    }
    if (!disabler.isEmpty()) {
        m_printed += QLatin1String("disabledBy: \'");
        m_printed += disabler;
        m_printed += QLatin1String("';");
        writeLineEnd();
    }

    // diagnostic category
    m_printed += QLatin1String("category: \'");
    m_printed += getQString(clang_getDiagnosticCategoryText(diag));
    m_printed += QLatin1String("';");

    // ranges
    unsigned numRanges = clang_getDiagnosticNumRanges(diag);
    if (numRanges > 0) {
        writeLineEnd();
        m_printed += QLatin1String("ranges: [");
        m_indent += 4;
        for (unsigned i = 0; i < numRanges; ++i) {
            writeLineEnd();
            writeRangeJson(clang_getDiagnosticRange(diag, i));
        }
        m_indent -= 4;
        writeLineEnd();
        m_printed += QLatin1Char(']');
    }

    // children
    CXDiagnosticSet set(clang_getChildDiagnostics(diag));
    unsigned numChildren = clang_getNumDiagnosticsInSet(set);
    if (numChildren > 0) {
        writeLineEnd();
        m_printed += QLatin1String("children: [");
        m_indent += 4;
        for (unsigned i = 0; i < numChildren; ++i) {
            writeLineEnd();
            ScopedCXDiagnostic child(clang_getDiagnosticInSet(set, i));
            writeDiagnosticJson(child);
        }
        m_indent -= 4;
        writeLineEnd();
        m_printed += QLatin1Char(']');
    }

    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char('}');
}

void CXPrettyPrinter::writeFixItJson(const CXDiagnostic &diag, unsigned i)
{
    CXSourceRange range; // half-open range [a, b)
    QString text = getQString(clang_getDiagnosticFixIt(diag, i, &range));

    m_printed += QLatin1String("{ newText: ");
    m_printed += QLatin1String("\'");
    m_printed += text;
    m_printed += QLatin1String("\', range: ");
    writeRangeJson(range);
    m_printed += QLatin1Char('}');
}

void CXPrettyPrinter::writeRangeJson(const CXSourceRange &range)
{
    SourceLocation start = getSpellingLocation(clang_getRangeStart(range));
    SourceLocation end = getSpellingLocation(clang_getRangeEnd(range));

    m_printed += QLatin1Char('{');
    m_indent += 4;
    writeLineEnd();

    m_printed += QLatin1String("file: \'");
    m_printed += start.fileName();
    m_printed += QLatin1String("\',");
    writeLineEnd();

    m_printed += QLatin1String("from: {");
    m_printed += QString::number(start.line());
    m_printed += QLatin1String(", ");
    m_printed += QString::number(start.column());
    m_printed += QLatin1String("},");

    m_printed += QLatin1String("to: {");
    m_printed += QString::number(end.line());
    m_printed += QLatin1String(", ");
    m_printed += QString::number(end.column());
    m_printed += QLatin1Char('}');

    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char('}');
}

void CXPrettyPrinter::writeLocationJson(const CXSourceLocation &location)
{
    SourceLocation loc = getSpellingLocation(location);
    m_printed += QLatin1Char('{');
    m_indent += 4;
    writeLineEnd();

    m_printed += QLatin1String("file: \'");
    m_printed += loc.fileName();
    m_printed += QLatin1String("\',");
    writeLineEnd();

    m_printed += QLatin1String("line: ");
    m_printed += QString::number(loc.line());
    m_printed += QLatin1Char(',');
    writeLineEnd();

    m_printed += QLatin1String("column: ");
    m_printed += QString::number(loc.column());

    m_indent -= 4;
    writeLineEnd();
    m_printed += QLatin1Char('}');
}

void CXPrettyPrinter::writeLineEnd()
{
    m_printed += QLatin1Char('\n');
    for (int i = 0; i < m_indent; ++i)
        m_printed += QLatin1Char(' ');
}
