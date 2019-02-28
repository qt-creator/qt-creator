/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangtooltipinfocollector.h"

#include "clangbackend_global.h"
#include "clangstring.h"
#include "cursor.h"
#include "sourcerange.h"
#include "token.h"
#include "unsavedfiles.h"
#include "unsavedfile.h"

#include <clangsupport/sourcerangecontainer.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>
#include <utils/qtcassert.h>

#include <utf8string.h>

#include <QDebug>
#include <QDir>
#include <QTextCodec>

namespace ClangBackEnd {

Utf8String qualificationPrefix(const Cursor &cursor)
{
    // TODO: Implement with qualificationPrefixAsVector()
    Utf8String qualifiedName;

    for (Cursor parent = cursor.semanticParent();
         parent.isValid() && (parent.kind() == CXCursor_Namespace);
         parent = parent.semanticParent()) {
        qualifiedName = parent.spelling() + Utf8StringLiteral("::") + qualifiedName;
    }

    return qualifiedName;
}

namespace {

Utf8StringVector qualificationPrefixAsVector(const Cursor &cursor)
{
    Utf8StringVector result;

    for (Cursor parent = cursor.semanticParent();
         parent.isValid() && (parent.kind() == CXCursor_Namespace || parent.isCompoundType());
         parent = parent.semanticParent()) {
        result.prepend(parent.spelling());
    }

    return result;
}

Utf8String displayName(const Cursor &cursor)
{
    if (cursor.kind() == CXCursor_ClassTemplate) {
        // TODO: The qualification should be part of the display name. Fix this in libclang.
        return qualificationPrefix(cursor) + cursor.displayName();
    }

    return cursor.displayName();
}

Utf8String textForFunctionLike(const Cursor &cursor)
{
#ifdef IS_PRETTY_DECL_SUPPORTED
    CXPrintingPolicy policy = clang_getCursorPrintingPolicy(cursor.cx());
    clang_PrintingPolicy_setProperty(policy, CXPrintingPolicy_FullyQualifiedName, 1);
    clang_PrintingPolicy_setProperty(policy, CXPrintingPolicy_TerseOutput, 1);
    // Avoid printing attributes/pragmas
    clang_PrintingPolicy_setProperty(policy, CXPrintingPolicy_PolishForDeclaration, 1);
    clang_PrintingPolicy_setProperty(policy, CXPrintingPolicy_SuppressInitializers, 1);
    const Utf8String prettyPrinted = ClangString(
        clang_getCursorPrettyPrinted(cursor.cx(), policy));
    clang_PrintingPolicy_dispose(policy);
    return prettyPrinted;
#else
    // Printing function declarations with displayName() is quite limited:
    //   * result type is not included
    //   * parameter names are not included
    //   * templates in the result type are not included
    //   * no full qualification of the function name
    return Utf8String(cursor.resultType().spelling())
         + Utf8StringLiteral(" ")
         + qualificationPrefix(cursor)
         + Utf8String(cursor.displayName());
#endif
}

Utf8String textForEnumConstantDecl(const Cursor &cursor)
{
    const Cursor semanticParent = cursor.semanticParent();
    QTC_ASSERT(semanticParent.kind() == CXCursor_EnumDecl, return Utf8String());

    const Type enumType = semanticParent.enumType();
    if (enumType.isUnsigned())
        return Utf8String::number(cursor.enumConstantUnsignedValue());
    return Utf8String::number(cursor.enumConstantValue());
}

Utf8String textForInclusionDirective(const Cursor &cursor)
{
    const CXFile includedFile = cursor.includedFile();
    const Utf8String fileName = ClangString(clang_getFileName(includedFile));

    return QDir::toNativeSeparators(fileName.toString());
}

Utf8String textForAnyTypeAlias(const Cursor &cursor)
{
    // For a CXCursor_TypeAliasTemplateDecl the type of cursor/referenced
    // is invalid, so we do not get the underlying type. This here solely
    // reports the unresolved name instead of the empty string.
    if (cursor.kind() == CXCursor_TypeAliasTemplateDecl)
        return cursor.displayName();

    return cursor.type().alias().utf8Spelling();
}

bool includeSizeForCursor(const Cursor &cursor)
{
    return cursor.isCompoundType()
        || cursor.kind() == CXCursor_EnumDecl
        || cursor.kind() == CXCursor_UnionDecl
        || cursor.kind() == CXCursor_FieldDecl;
}

Utf8String sizeInBytes(const Cursor &cursor)
{
    if (includeSizeForCursor(cursor)) {
        bool ok = false;
        const long long size = cursor.type().sizeOf(&ok);
        if (ok)
            return Utf8String::number(size);
    }

    return Utf8String();
}

Cursor referencedCursor(const Cursor &cursor)
{
    // Query the referenced cursor directly instead of first testing with cursor.isReference().
    // cursor.isReference() reports false for e.g. CXCursor_DeclRefExpr or CXCursor_CallExpr
    // although it returns a valid cursor.
    const Cursor referenced = cursor.referenced();
    if (referenced.isValid())
        return referenced;

    const Cursor definition = cursor.definition();
    if (definition.isValid())
        return definition;

    return cursor;
}

class ToolTipInfoCollector
{
public:
    ToolTipInfoCollector(UnsavedFiles &unsavedFiles,
                         const Utf8String &textCodecName,
                         const Utf8String &mainFilePath,
                         CXTranslationUnit cxTranslationUnit);

    ToolTipInfo collect(uint line, uint column) const;

private:
    Utf8String text(const Cursor &cursor, const Cursor &referenced) const;
    Utf8String textForMacroExpansion(const Cursor &cursor) const;
    Utf8String textForNamespaceAlias(const Cursor &cursor) const;

    ToolTipInfo qDocInfo(const Cursor &cursor) const;

    CXSourceLocation toCXSourceLocation(uint line, uint column) const;

    UnsavedFile unsavedFile(const Utf8String &filePath) const;
    Utf8String lineRange(const Utf8String &filePath, unsigned fromLine, unsigned toLine) const;

private:
    UnsavedFiles &m_unsavedFiles;
    const Utf8String m_textCodecName;

    const Utf8String m_mainFilePath;
    CXTranslationUnit m_cxTranslationUnit = nullptr;

};

ToolTipInfoCollector::ToolTipInfoCollector(UnsavedFiles &unsavedFiles,
                                           const Utf8String &textCodecName,
                                           const Utf8String &mainFilePath,
                                           CXTranslationUnit cxTranslationUnit)
    : m_unsavedFiles(unsavedFiles)
    , m_textCodecName(textCodecName)
    , m_mainFilePath(mainFilePath)
    , m_cxTranslationUnit(cxTranslationUnit)
{
}

Utf8String ToolTipInfoCollector::text(const Cursor &cursor, const Cursor &referenced) const
{
    if (cursor.kind() == CXCursor_MacroExpansion)
        return textForMacroExpansion(referenced);

    if (referenced.kind() == CXCursor_EnumConstantDecl)
        return textForEnumConstantDecl(referenced);

    if (referenced.kind() == CXCursor_InclusionDirective)
        return textForInclusionDirective(referenced);

    if (referenced.kind() == CXCursor_Namespace)
        return qualificationPrefix(referenced) + referenced.spelling();

    if (referenced.kind() == CXCursor_NamespaceAlias)
        return textForNamespaceAlias(referenced);

    if (referenced.isAnyTypeAlias())
        return textForAnyTypeAlias(referenced);

    if (referenced.isFunctionLike() || referenced.kind() == CXCursor_Constructor)
        return textForFunctionLike(referenced);

    if (referenced.type().canonical().isBuiltinType())
        return referenced.type().canonical().builtinTypeToString();

    if (referenced.kind() == CXCursor_VarDecl)
        return referenced.type().spelling(); // e.g. "Zii<int>"

    const Type referencedType = referenced.type();
    if (referencedType.isValid()) {
        // Generally, the type includes the qualification but has this limitations:
        //  * namespace aliases are not resolved
        //  * outer class of a inner template class is not included
        // The type includes the qualification, but not resolved namespace aliases.

        // For a CXType_Record, this also includes e.g. "const " as prefix.
        return referencedType.canonical().utf8Spelling();
    }

    return displayName(referenced);
}

Utf8String ToolTipInfoCollector::textForMacroExpansion(const Cursor &cursor) const
{
    QTC_ASSERT(cursor.kind() == CXCursor_MacroDefinition, return Utf8String());

    const SourceRange sourceRange = cursor.sourceRange();
    const SourceLocation start = sourceRange.start();
    const SourceLocation end = sourceRange.end();

    return lineRange(start.filePath(), start.line(), end.line());
}

Utf8String ToolTipInfoCollector::textForNamespaceAlias(const Cursor &cursor) const
{
    // TODO: Add some libclang API to get the aliased name straight away.

    const Tokens tokens(cursor.sourceRange());

    Utf8String aliasedName;
    // Start at 3 in order to skip these tokens: namespace X =
    for (uint i = 3; i < tokens.size(); ++i)
        aliasedName += tokens[i].spelling();

    return aliasedName;
}

static Utf8String typeName(const Type &type)
{
    return type.declaration().spelling();
}

static Utf8String qdocMark(const Cursor &cursor)
{
    if (cursor.kind() == CXCursor_ClassTemplate)
        return cursor.spelling();

    if (cursor.type().kind() == CXType_Enum
            || cursor.type().kind() == CXType_Typedef
            || cursor.type().kind() == CXType_Record)
        return typeName(cursor.type());

    Utf8String text = cursor.displayName();
    if (cursor.kind() == CXCursor_FunctionDecl) {
        // TODO: Remove this workaround by fixing this in
        // libclang with the help of CXPrintingPolicy.
        text.replace(Utf8StringLiteral("<>"), Utf8String());
    }

    return text;
}

static ToolTipInfo::QdocCategory qdocCategory(const Cursor &cursor)
{
    if (cursor.isFunctionLike())
        return ToolTipInfo::Function;

    if (cursor.kind() == CXCursor_MacroDefinition)
        return ToolTipInfo::Macro;

    if (cursor.kind() == CXCursor_EnumConstantDecl)
        return ToolTipInfo::Enum;

    if (cursor.type().kind() == CXType_Enum)
        return ToolTipInfo::Enum;

    if (cursor.kind() == CXCursor_InclusionDirective)
        return ToolTipInfo::Brief;

    // TODO: Handle CXCursor_NamespaceAlias, too?!
    if (cursor.kind() == CXCursor_Namespace)
        return ToolTipInfo::ClassOrNamespace;

    if (cursor.isCompoundType())
        return ToolTipInfo::ClassOrNamespace;

    if (cursor.kind() == CXCursor_NamespaceAlias)
        return ToolTipInfo::ClassOrNamespace;

    if (cursor.type().kind() == CXType_Typedef)
        return ToolTipInfo::Typedef;

    if (cursor.type().kind() == CXType_Record)
        return ToolTipInfo::ClassOrNamespace;

    if (cursor.kind() == CXCursor_TypeAliasTemplateDecl)
        return ToolTipInfo::Typedef;

    if (cursor.kind() == CXCursor_ClassTemplate)
        return ToolTipInfo::ClassOrNamespace;

    return ToolTipInfo::Unknown;
}

static Utf8String name(const Cursor &cursor)
{
    if (cursor.type().kind() == CXType_Record || cursor.kind() == CXCursor_EnumDecl)
        return typeName(cursor.type());

    return cursor.spelling();
}

static Utf8StringVector qDocIdCandidates(const Cursor &cursor)
{
    Utf8StringVector components = qualificationPrefixAsVector(cursor);
    if (components.isEmpty())
        return { name(cursor) };

    components << name(cursor);
    Utf8StringVector result;
    Utf8String name;
    for (auto it = components.rbegin(); it != components.rend(); ++it) {
        if (name.isEmpty())
            name = *it;
        else
            name = *it + (Utf8StringLiteral("::") + name);

        result.prepend(name);
    }

    return result;
}

// TODO: Add libclang API for this?!
static bool isBuiltinOrPointerToBuiltin(const Type &type)
{
    Type theType = type;

    if (theType.isBuiltinType())
        return true;

    // TODO: Simplify
    // TODO: Test with **
    while (theType.pointeeType().isValid() && theType != theType.pointeeType()) {
        theType = theType.pointeeType();
        if (theType.isBuiltinType())
            return true;
    }

    return false;
}

ToolTipInfo ToolTipInfoCollector::qDocInfo(const Cursor &cursor) const
{
    ToolTipInfo result;

    if (isBuiltinOrPointerToBuiltin(cursor.type()))
        return result;

    if (cursor.kind() == CXCursor_Constructor) {
        const ToolTipInfo parentInfo = qDocInfo(cursor.semanticParent());
        result.qdocIdCandidates = parentInfo.qdocIdCandidates;
        result.qdocMark = parentInfo.qdocMark;
        result.qdocCategory = ToolTipInfo::Unknown;
        return result;
    }

    result.qdocIdCandidates = qDocIdCandidates(cursor);
    result.qdocMark = qdocMark(cursor);
    result.qdocCategory = qdocCategory(cursor);

    if (cursor.type().kind() == CXType_Record) {
        result.qdocIdCandidates = qDocIdCandidates(cursor.type().declaration());
        return result;
    }

    if (cursor.kind() == CXCursor_VarDecl || cursor.kind() == CXCursor_ParmDecl
        || cursor.kind() == CXCursor_FieldDecl) {
        // maybe template instantiation
        if (cursor.type().kind() == CXType_Unexposed && cursor.type().canonical().kind() == CXType_Record) {
            result.qdocIdCandidates = qDocIdCandidates(cursor.type().canonical().declaration());
            result.qdocMark = typeName(cursor.type());
            result.qdocCategory = ToolTipInfo::ClassOrNamespace;
            return result;
        }

        Type type = cursor.type();
        while (type.pointeeType().isValid() && type != type.pointeeType())
            type = type.pointeeType();

        const Cursor typeCursor = type.declaration();
        result.qdocIdCandidates = qDocIdCandidates(typeCursor);
        result.qdocCategory = qdocCategory(typeCursor);
        result.qdocMark = typeName(type);
    }

    // TODO: Handle also RValueReference()
    if (cursor.type().isLValueReference()) {
        const Cursor pointeeTypeDeclaration = cursor.type().pointeeType().declaration();
        result.qdocIdCandidates = qDocIdCandidates(pointeeTypeDeclaration);
        result.qdocMark = pointeeTypeDeclaration.spelling();
        result.qdocCategory = qdocCategory(pointeeTypeDeclaration);
        return result;
    }

    return result;
}

CXSourceLocation ToolTipInfoCollector::toCXSourceLocation(uint line, uint column) const
{
    return clang_getLocation(m_cxTranslationUnit,
                             clang_getFile(m_cxTranslationUnit,
                                           m_mainFilePath.constData()),
                             line,
                             column);
}

UnsavedFile ToolTipInfoCollector::unsavedFile(const Utf8String &filePath) const
{
    const UnsavedFile &unsavedFile = m_unsavedFiles.unsavedFile(filePath);
    if (!unsavedFile.filePath().isEmpty())
        return unsavedFile;

    // Create an unsaved file with the file content from disk.
    // TODO: Make use of clang_getFileContents() instead of reading from disk.
    QTextCodec *codec = QTextCodec::codecForName(m_textCodecName);
    QByteArray fileContent;
    QString errorString;
    using namespace Utils;
    const TextFileFormat::ReadResult readResult
            = TextFileFormat::readFileUTF8(filePath.toString(), codec, &fileContent, &errorString);
    if (readResult != TextFileFormat::ReadSuccess) {
        qWarning() << "Failed to read file" << filePath << ":" << errorString;
        return UnsavedFile();
    }

    return UnsavedFile(filePath, Utf8String::fromByteArray(fileContent));
}

Utf8String ToolTipInfoCollector::lineRange(const Utf8String &filePath,
                                           unsigned fromLine,
                                           unsigned toLine) const
{
    if (toLine < fromLine)
        return Utf8String();

    const UnsavedFile file = unsavedFile(filePath);
    if (file.fileContent().isEmpty())
        return Utf8String();

    return file.lineRange(fromLine, toLine);
}

ToolTipInfo ToolTipInfoCollector::collect(uint line, uint column) const
{
    const Cursor cursor = clang_getCursor(m_cxTranslationUnit, toCXSourceLocation(line, column));
    if (!cursor.isValid())
        return ToolTipInfo(); // E.g. cursor on ifdeffed out range

    const Cursor referenced = referencedCursor(cursor);
    QTC_CHECK(referenced.isValid());

    ToolTipInfo info;
    info.text = text(cursor, referenced);
    info.briefComment = referenced.briefComment();

    {
        ToolTipInfo qDocToolTipInfo = qDocInfo(referenced);
        info.qdocIdCandidates = qDocToolTipInfo.qdocIdCandidates;
        info.qdocMark = qDocToolTipInfo.qdocMark;
        info.qdocCategory = qDocToolTipInfo.qdocCategory;
    }

    info.sizeInBytes = sizeInBytes(cursor);

    return info;
}

} // anonymous namespace

ToolTipInfo collectToolTipInfo(UnsavedFiles &unsavedFiles,
                               const Utf8String &textCodecName,
                               const Utf8String &mainFilePath,
                               CXTranslationUnit cxTranslationUnit,
                               uint line,
                               uint column)
{
    ToolTipInfoCollector collector(unsavedFiles, textCodecName, mainFilePath, cxTranslationUnit);
    const ToolTipInfo info = collector.collect(line, column);

    return info;
}

} // namespace ClangBackEnd
