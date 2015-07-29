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

#include "semanticmarker.h"
#include "unit.h"
#include "utils_p.h"
#include "cxraii.h"

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

static const unsigned ATTACHED_NOTES_LIMIT = 10;

SemanticMarker::SemanticMarker()
{
}

SemanticMarker::~SemanticMarker()
{
}

QString SemanticMarker::fileName() const
{
    if (!m_unit)
        return QString();

    return m_unit->fileName();
}

void SemanticMarker::setFileName(const QString &fileName)
{
    if (this->fileName() == fileName)
        return;

    QStringList oldOptions;
    if (m_unit)
        oldOptions = m_unit->compilationOptions();
    m_unit = Unit::create(fileName);
    if (!oldOptions.isEmpty())
        m_unit->setCompilationOptions(oldOptions);

    unsigned clangOpts = clang_defaultEditingTranslationUnitOptions();
    clangOpts |= CXTranslationUnit_DetailedPreprocessingRecord;
    clangOpts |= CXTranslationUnit_Incomplete;
    clangOpts &= ~CXTranslationUnit_CacheCompletionResults;
    m_unit->setManagementOptions(clangOpts);
}

void SemanticMarker::setCompilationOptions(const QStringList &options)
{
    QTC_ASSERT(m_unit, return);

    if (m_unit->compilationOptions() == options)
        return;

    m_unit->setCompilationOptions(options);
    m_unit->unload();
}

void SemanticMarker::reparse(const UnsavedFiles &unsavedFiles)
{
    QTC_ASSERT(m_unit, return);

    m_unit->setUnsavedFiles(unsavedFiles);
    if (m_unit->isLoaded())
        m_unit->reparse();
    else
        m_unit->parse();
}

/**
 * \brief Calculate one or several ranges and append diagnostic for each range
 * Extracted from SemanticMarker::diagnostics() to reuse code
 */
static void appendDiagnostic(const CXDiagnostic &diag,
                             const CXSourceLocation &cxLocation,
                             Diagnostic::Severity severity,
                             const QString &spelling,
                             QList<Diagnostic> &diagnostics)
{
    const unsigned rangeCount = clang_getDiagnosticNumRanges(diag);
    bool expandLocation = true;

    for (unsigned i = 0; i < rangeCount; ++i) {
        CXSourceRange r = clang_getDiagnosticRange(diag, i);
        const SourceLocation &spellBegin = Internal::getSpellingLocation(clang_getRangeStart(r));
        const SourceLocation &spellEnd = Internal::getSpellingLocation(clang_getRangeEnd(r));
        unsigned length = spellEnd.offset() - spellBegin.offset();

        // File name can be empty due clang bug
        if (!spellBegin.fileName().isEmpty()) {
            Diagnostic d(severity, spellBegin, length, spelling);
            diagnostics.append(d);
            expandLocation = false;
        }
    }

    if (expandLocation) {
        const SourceLocation &location = Internal::getExpansionLocation(cxLocation);
        Diagnostic d(severity, location, 0, spelling);
        diagnostics.append(d);
    }
}

static bool isBlackListedDiagnostic(const Utils::MimeType &mimeType, const QString &diagnostic)
{
    static QString pragmaOnceInMainFile = QLatin1String("#pragma once in main file");

    return diagnostic == pragmaOnceInMainFile
        && mimeType.inherits(QLatin1String("text/x-chdr"));
}

QList<Diagnostic> SemanticMarker::diagnostics() const
{
    QList<Diagnostic> diagnostics;
    if (!m_unit || !m_unit->isLoaded())
        return diagnostics;

    Utils::MimeDatabase mimeDatabase;
    const Utils::MimeType mimeType = mimeDatabase.mimeTypeForFile(fileName());

    const unsigned diagCount = m_unit->getNumDiagnostics();
    for (unsigned i = 0; i < diagCount; ++i) {
        ScopedCXDiagnostic diag(m_unit->getDiagnostic(i));

        Diagnostic::Severity severity = static_cast<Diagnostic::Severity>(clang_getDiagnosticSeverity(diag));
        if (severity == Diagnostic::Ignored || severity == Diagnostic::Note)
            continue;

        CXSourceLocation cxLocation = clang_getDiagnosticLocation(diag);
        QString spelling = Internal::getQString(clang_getDiagnosticSpelling(diag));

        if (isBlackListedDiagnostic(mimeType, spelling))
            continue;

        // Attach messages with Diagnostic::Note severity
        ScopedCXDiagnosticSet cxChildren(clang_getChildDiagnostics(diag));
        const unsigned numChildren = clang_getNumDiagnosticsInSet(cxChildren);
        const unsigned size = qMin(ATTACHED_NOTES_LIMIT, numChildren);
        for (unsigned di = 0; di < size; ++di) {
            ScopedCXDiagnostic child(clang_getDiagnosticInSet(cxChildren, di));

            const Diagnostic::Severity severity
                = static_cast<Diagnostic::Severity>(clang_getDiagnosticSeverity(child));
            if (severity == Diagnostic::Ignored || severity == Diagnostic::Note)
                continue;

            spelling.append(QLatin1String("\n  "));
            spelling.append(Internal::getQString(clang_getDiagnosticSpelling(child)));
        }

        // Fatal error may occur in another file, but it breaks whole parsing
        // Typical fatal error is unresolved #include
        if (severity == Diagnostic::Fatal) {
            for (unsigned di = 0; di < numChildren; ++di) {
                ScopedCXDiagnostic child(clang_getDiagnosticInSet(cxChildren, di));
                appendDiagnostic(child, clang_getDiagnosticLocation(child), Diagnostic::Warning, spelling, diagnostics);
            }
        }

        appendDiagnostic(diag, cxLocation, severity, spelling, diagnostics);
    }

    return diagnostics;
}

QList<SemanticMarker::Range> SemanticMarker::ifdefedOutBlocks() const
{
    QList<Range> blocks;

    if (!m_unit || !m_unit->isLoaded())
        return blocks;

#if CINDEX_VERSION_MINOR >= 21
    CXSourceRangeList *skippedRanges = clang_getSkippedRanges(m_unit->clangTranslationUnit(),
                                                              m_unit->getFile());
    blocks.reserve(skippedRanges->count);
    for (unsigned i = 0; i < skippedRanges->count; ++i) {
        const CXSourceRange &r = skippedRanges->ranges[i];
        const SourceLocation &spellBegin = Internal::getSpellingLocation(clang_getRangeStart(r));
        if (spellBegin.fileName() != fileName())
            continue;
        const SourceLocation &spellEnd = Internal::getSpellingLocation(clang_getRangeEnd(r));
        const int begin = spellBegin.offset() + 1;
        const int end = spellEnd.offset() - spellEnd.column();
        blocks.append(Range(begin, end));
    }
    clang_disposeSourceRangeList(skippedRanges);
#endif

    return blocks;
}

namespace {
static void add(QList<SourceMarker> &markers,
                const CXSourceRange &extent,
                SourceMarker::Kind kind)
{
    CXSourceLocation start = clang_getRangeStart(extent);
    CXSourceLocation end = clang_getRangeEnd(extent);
    const SourceLocation &location = Internal::getExpansionLocation(start);
    const SourceLocation &locationEnd = Internal::getExpansionLocation(end);

    if (location.offset() < locationEnd.offset()) {
        const unsigned length = locationEnd.offset() - location.offset();
        markers.append(SourceMarker(location, length, kind));
    }
}

/**
 * @brief Selects correct highlighting for cursor that is reference
 * @return SourceMarker::Unknown if cannot select highlighting
 */
static SourceMarker::Kind getKindByReferencedCursor(const CXCursor &cursor)
{
    const CXCursor referenced = clang_getCursorReferenced(cursor);
    switch (clang_getCursorKind(referenced)) {
    case CXCursor_EnumConstantDecl:
        return SourceMarker::Enumeration;

    case CXCursor_FieldDecl:
    case CXCursor_ObjCIvarDecl:
    case CXCursor_ObjCPropertyDecl:
        return SourceMarker::Field;

    case CXCursor_FunctionDecl:
    case CXCursor_FunctionTemplate:
    case CXCursor_Constructor:
        return SourceMarker::Function;

    case CXCursor_VarDecl:
    case CXCursor_ParmDecl:
    case CXCursor_NonTypeTemplateParameter:
        return SourceMarker::Local;

    case CXCursor_CXXMethod:
        if (clang_CXXMethod_isVirtual(referenced))
            return SourceMarker::VirtualMethod;
        else
            return SourceMarker::Function;

    case CXCursor_ObjCClassMethodDecl:
    case CXCursor_ObjCInstanceMethodDecl:
        // calling method as property, e.h. "layer.shouldRasterize = YES"
        return SourceMarker::Field;

    case CXCursor_UnexposedDecl:
        // NSObject "self" method which is a pseudo keyword
        if (clang_getCursorLanguage(referenced) == CXLanguage_ObjC)
            return SourceMarker::PseudoKeyword;
        break;

    default:
        break;
    }
    return SourceMarker::Unknown;
}

static const QSet<QString> ObjcPseudoKeywords = QSet<QString>()
        << QLatin1String("end")
        << QLatin1String("try")
        << QLatin1String("defs")
        << QLatin1String("throw")
        << QLatin1String("class")
        << QLatin1String("catch")
        << QLatin1String("encode")
        << QLatin1String("public")
        << QLatin1String("dynamic")
        << QLatin1String("finally")
        << QLatin1String("package")
        << QLatin1String("private")
        << QLatin1String("optional")
        << QLatin1String("property")
        << QLatin1String("protocol")
        << QLatin1String("required")
        << QLatin1String("selector")
        << QLatin1String("interface")
        << QLatin1String("protected")
        << QLatin1String("synthesize")
        << QLatin1String("not_keyword")
        << QLatin1String("synchronized")
        << QLatin1String("implementation")
        << QLatin1String("compatibility_alias")
           ;

} // Anonymous namespace

/**
 * @brief SemanticMarker::sourceMarkersInRange
 * @param firstLine - first line where to generate highlighting markers
 * @param lastLine - last line where to generate highlighting markers
 *
 * There still two kinds of problems:
 *    - clang_annotateTokens() can return wrong cursor, and it's normal behavior
 *    - some cases no handled
 *
 * Problems caused by wrong cursors:
 *    - range-based for from C++ 2011
 *    - identifiers in some compound statements have type DeclStmt
 *      or CompoundStmt which refers to top-level construction.
 *    - CXCursor_ObjCIvarDecl mapped to field, but instance variable have
 *      incorrect cursor kind if it declared in private interface
 *          @interface MyApplication() {
 *              NSArray* _items;
 *          }
 *
 * Missed cases:
 *    - global variables highlighted as locals
 *    - appropriate marker had not been selected for listed cursors:
 *          CXCursor_ObjCProtocolExpr, CXCursor_ObjCEncodeExpr,
 *          CXCursor_ObjCDynamicDecl, CXCursor_ObjCBridgedCastExpr,
 *          CXCursor_ObjCSuperClassRef
 *    - template members of template classes&functions always highlighted
 *      as members, even if they are functions - no way to differ found.
 *    - @1, @{}, @[]
 */
QList<SourceMarker> SemanticMarker::sourceMarkersInRange(unsigned firstLine,
                                                         unsigned lastLine)
{
    QList<SourceMarker> result;

    if (!m_unit || !m_unit->isLoaded())
        return result;

    // Highlighting called asynchronously, and a few lines at the end can be deleted for this time.
    CXSourceRange unitRange = clang_getCursorExtent(m_unit->getTranslationUnitCursor());
    SourceLocation unitEnd = getExpansionLocation(clang_getRangeEnd(unitRange));
    if (lastLine > unitEnd.line())
        lastLine = unitEnd.line();

    if (firstLine > lastLine)
        return result;

    IdentifierTokens idTokens(*m_unit, firstLine, lastLine);

    const CXSourceRange *atTokenExtent = 0;
    for (unsigned i = 0; i < idTokens.count(); ++i) {
        const CXToken &tok = idTokens.token(i);
        CXTokenKind kind = clang_getTokenKind(tok);
        if (atTokenExtent) {
            if (CXToken_Literal == kind) {
                if (m_unit->getTokenSpelling(tok).startsWith(QLatin1Char('"')))
                    add(result, *atTokenExtent, SourceMarker::ObjCString);
                atTokenExtent = 0;
                continue;
            } else {
                add(result, *atTokenExtent, SourceMarker::PseudoKeyword);
                atTokenExtent = 0;
            }
        }

        const CXSourceRange &tokenExtent = idTokens.extent(i);

        if (CXToken_Keyword == kind) {
            QString spell = m_unit->getTokenSpelling(tok);
            if (ObjcPseudoKeywords.contains(spell))
                add(result, tokenExtent, SourceMarker::PseudoKeyword);
            continue;
        }

        if (CXToken_Punctuation == kind) {
            static const QLatin1String at("@");
            if (m_unit->getTokenSpelling(tok) == at)
                atTokenExtent = &tokenExtent;
            continue;
        }

        if (CXToken_Identifier != kind)
            continue;

        const CXCursor &cursor = idTokens.cursor(i);
        const CXCursorKind cursorKind = clang_getCursorKind(cursor);
        if (clang_isInvalid(cursorKind))
            continue;

        switch (cursorKind) {
        case CXCursor_EnumConstantDecl:
            add(result, tokenExtent, SourceMarker::Enumeration);
            break;

        case CXCursor_ClassDecl:
        case CXCursor_UnionDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_EnumDecl:
        case CXCursor_Namespace:
        case CXCursor_NamespaceRef:
        case CXCursor_NamespaceAlias:
        case CXCursor_StructDecl:
        case CXCursor_TemplateRef:
        case CXCursor_TypeRef:
        case CXCursor_TypedefDecl:
        case CXCursor_Constructor:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_UnexposedDecl: /* friend class MyClass; */
            add(result, tokenExtent, SourceMarker::Type);
            break;

        case CXCursor_ParmDecl:
        case CXCursor_VariableRef:
        case CXCursor_VarDecl:
        case CXCursor_NonTypeTemplateParameter:
            add(result, tokenExtent, SourceMarker::Local);
            break;

        case CXCursor_MemberRefExpr:
        case CXCursor_MemberRef:
        case CXCursor_DeclRefExpr:
        case CXCursor_CallExpr: {
            SourceMarker::Kind kind = getKindByReferencedCursor(cursor);
            if (kind == SourceMarker::Unknown && cursorKind == CXCursor_MemberRefExpr) {
                /* template class member in template function */
                kind = SourceMarker::Field;
            }
            if (kind != SourceMarker::Unknown)
                add(result, tokenExtent, kind);
        } break;

        case CXCursor_FieldDecl:
            add(result, tokenExtent, SourceMarker::Field);
            break;

        case CXCursor_Destructor:
        case CXCursor_CXXMethod: {
            if (clang_CXXMethod_isVirtual(cursor))
                add(result, tokenExtent, SourceMarker::VirtualMethod);
            else
                add(result, tokenExtent, SourceMarker::Function);
        } break;

        case CXCursor_CXXOverrideAttr:
        case CXCursor_CXXFinalAttr:
        case CXCursor_AnnotateAttr: // 'annotate' in '__attribute__((annotate("AnyComment")))'
        case CXCursor_UnexposedAttr: // 'align' in '__declspec(align(8))'
            add(result, tokenExtent, SourceMarker::PseudoKeyword);
            break;

        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
        case CXCursor_OverloadedDeclRef:
            add(result, tokenExtent, SourceMarker::Function);
            break;

        case CXCursor_ObjCInstanceMethodDecl:
        case CXCursor_ObjCClassMethodDecl:
        case CXCursor_ObjCSelectorExpr:
            add(result, tokenExtent, SourceMarker::ObjectiveCMessage);
            break;

        case CXCursor_ObjCMessageExpr: {
            static const QLatin1String super("super");
            if (m_unit->getTokenSpelling(tok) == super)
                add(result, tokenExtent, SourceMarker::PseudoKeyword);
            else
                add(result, tokenExtent, SourceMarker::ObjectiveCMessage);
        } break;

        case CXCursor_ObjCCategoryDecl:
        case CXCursor_ObjCCategoryImplDecl:
        case CXCursor_ObjCImplementationDecl:
        case CXCursor_ObjCInterfaceDecl:
        case CXCursor_ObjCProtocolDecl:
        case CXCursor_ObjCProtocolRef:
        case CXCursor_ObjCClassRef:
        case CXCursor_ObjCSuperClassRef:
        case CXCursor_TypeAliasDecl: // C++11 type alias: 'using value_t = T'
            add(result, tokenExtent, SourceMarker::Type);
            break;

        case CXCursor_ObjCSynthesizeDecl:
        case CXCursor_ObjCDynamicDecl:
        case CXCursor_ObjCPropertyDecl:
        case CXCursor_ObjCIvarDecl:
            add(result, tokenExtent, SourceMarker::Field);
            break;

        case CXCursor_MacroDefinition:
        case CXCursor_MacroExpansion:
            add(result, tokenExtent, SourceMarker::Macro);
            break;

        case CXCursor_LabelRef:
        case CXCursor_LabelStmt:
            add(result, tokenExtent, SourceMarker::Label);
            break;

        default:
            break;
        }
    }

    return result;
}

Unit::Ptr SemanticMarker::unit() const
{
    return m_unit;
}
