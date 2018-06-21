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

#include "clangutils.h"

#include "clangeditordocumentprocessor.h"
#include "clangmodelmanagersupport.h"

#include <clangsupport/tokeninfocontainer.h>

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectpart.h>
#include <cpptools/cppcodemodelsettings.h>
#include <cpptools/cpptoolsreuse.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QStringList>
#include <QTextBlock>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace Core;
using namespace CppTools;

namespace ClangCodeModel {
namespace Utils {

/**
 * @brief Creates list of message-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileName Path to file, non-empty
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, const QString &fileName)
{
    ProjectFile::Kind fileKind = ProjectFile::Unclassified;
    if (!pPart.isNull())
        foreach (const ProjectFile &file, pPart->files)
            if (file.path == fileName) {
                fileKind = file.kind;
                break;
            }
    if (fileKind == ProjectFile::Unclassified)
        fileKind = ProjectFile::classify(fileName);

    return createClangOptions(pPart, fileKind);
}

static QString creatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::resourcePath();
#else
    return QString();
#endif
}

class LibClangOptionsBuilder final : public CompilerOptionsBuilder
{
public:
    LibClangOptionsBuilder(const ProjectPart &projectPart)
        : CompilerOptionsBuilder(projectPart, CLANG_VERSION, CLANG_RESOURCE_DIR)
    {
    }

    void addPredefinedHeaderPathsOptions() final
    {
        CompilerOptionsBuilder::addPredefinedHeaderPathsOptions();
        addWrappedQtHeadersIncludePath();
    }

    void addToolchainAndProjectMacros() final
    {
        addMacros({ProjectExplorer::Macro("Q_CREATOR_RUN", "1")});
        CompilerOptionsBuilder::addToolchainAndProjectMacros();
    }

    void addExtraOptions() final
    {
        addDummyUiHeaderOnDiskIncludePath();
        add("-fmessage-length=0");
        add("-fdiagnostics-show-note-include-stack");
        add("-fmacro-backtrace-limit=0");
        add("-fretain-comments-from-system-headers");
        add("-ferror-limit=1000");
    }

private:
    void addWrappedQtHeadersIncludePath()
    {
        static const QString resourcePath = creatorResourcePath();
        static QString wrappedQtHeadersPath = resourcePath + "/cplusplus/wrappedQtHeaders";
        QTC_ASSERT(QDir(wrappedQtHeadersPath).exists(), return;);

        if (m_projectPart.qtVersion != CppTools::ProjectPart::NoQt) {
            const QString wrappedQtCoreHeaderPath = wrappedQtHeadersPath + "/QtCore";
            add(includeDirOption());
            add(QDir::toNativeSeparators(wrappedQtHeadersPath));
            add(includeDirOption());
            add(QDir::toNativeSeparators(wrappedQtCoreHeaderPath));
        }
    }

    void addDummyUiHeaderOnDiskIncludePath()
    {
        const QString path = ModelManagerSupportClang::instance()->dummyUiHeaderOnDiskDirPath();
        if (!path.isEmpty()) {
            add(includeDirOption());
            add(QDir::toNativeSeparators(path));
        }
    }
};

/**
 * @brief Creates list of message-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileKind Determines language and source/header state
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, ProjectFile::Kind fileKind)
{
    if (!pPart)
        return QStringList();
    return LibClangOptionsBuilder(*pPart).build(fileKind, CompilerOptionsBuilder::PchUsage::None);
}

ProjectPart::Ptr projectPartForFile(const QString &filePath)
{
    if (const auto parser = CppTools::BaseEditorDocumentParser::get(filePath))
        return parser->projectPartInfo().projectPart;
    return ProjectPart::Ptr();
}

ProjectPart::Ptr projectPartForFileBasedOnProcessor(const QString &filePath)
{
    if (const auto processor = ClangEditorDocumentProcessor::get(filePath))
        return processor->projectPart();
    return ProjectPart::Ptr();
}

bool isProjectPartLoaded(const ProjectPart::Ptr projectPart)
{
    if (projectPart)
        return CppModelManager::instance()->projectPartForId(projectPart->id());
    return false;
}

QString projectPartIdForFile(const QString &filePath)
{
    const ProjectPart::Ptr projectPart = projectPartForFile(filePath);

    if (isProjectPartLoaded(projectPart))
        return projectPart->id(); // OK, Project Part is still loaded
    return QString();
}

CppEditorDocumentHandle *cppDocument(const QString &filePath)
{
    return CppTools::CppModelManager::instance()->cppEditorDocument(filePath);
}

void setLastSentDocumentRevision(const QString &filePath, uint revision)
{
    if (CppEditorDocumentHandle *document = cppDocument(filePath))
        document->sendTracker().setLastSentRevision(int(revision));
}

int clangColumn(const QTextBlock &line, int cppEditorColumn)
{
    // (1) cppEditorColumn is the actual column shown by CppEditor.
    // (2) The return value is the column in Clang which is the utf8 byte offset from the beginning
    //     of the line.
    // Here we convert column from (1) to (2).
    // '+ 1' is for 1-based columns
    return line.text().left(cppEditorColumn).toUtf8().size() + 1;
}

CPlusPlus::Icons::IconType iconTypeForToken(const ClangBackEnd::TokenInfoContainer &token)
{
    const ClangBackEnd::ExtraInfo &extraInfo = token.extraInfo;
    if (extraInfo.signal)
        return CPlusPlus::Icons::SignalIconType;

    ClangBackEnd::AccessSpecifier access = extraInfo.accessSpecifier;
    if (extraInfo.slot) {
        switch (access) {
        case ClangBackEnd::AccessSpecifier::Public:
        case ClangBackEnd::AccessSpecifier::Invalid:
            return CPlusPlus::Icons::SlotPublicIconType;
        case ClangBackEnd::AccessSpecifier::Protected:
            return CPlusPlus::Icons::SlotProtectedIconType;
        case ClangBackEnd::AccessSpecifier::Private:
            return CPlusPlus::Icons::SlotPrivateIconType;
        }
    }

    ClangBackEnd::HighlightingType mainType = token.types.mainHighlightingType;

    if (mainType == ClangBackEnd::HighlightingType::QtProperty)
        return CPlusPlus::Icons::PropertyIconType;

    if (mainType == ClangBackEnd::HighlightingType::PreprocessorExpansion
            || mainType == ClangBackEnd::HighlightingType::PreprocessorDefinition) {
        return CPlusPlus::Icons::MacroIconType;
    }

    if (mainType == ClangBackEnd::HighlightingType::Enumeration)
        return CPlusPlus::Icons::EnumeratorIconType;

    if (mainType == ClangBackEnd::HighlightingType::Type
            || mainType == ClangBackEnd::HighlightingType::Keyword) {
        const ClangBackEnd::MixinHighlightingTypes &types = token.types.mixinHighlightingTypes;
        if (types.contains(ClangBackEnd::HighlightingType::Enum))
            return CPlusPlus::Icons::EnumIconType;
        if (types.contains(ClangBackEnd::HighlightingType::Struct))
            return CPlusPlus::Icons::StructIconType;
        if (types.contains(ClangBackEnd::HighlightingType::Namespace))
            return CPlusPlus::Icons::NamespaceIconType;
        if (types.contains(ClangBackEnd::HighlightingType::Class))
            return CPlusPlus::Icons::ClassIconType;
        if (mainType == ClangBackEnd::HighlightingType::Keyword)
            return CPlusPlus::Icons::KeywordIconType;
        return CPlusPlus::Icons::ClassIconType;
    }

    ClangBackEnd::StorageClass storageClass = extraInfo.storageClass;
    if (mainType == ClangBackEnd::HighlightingType::VirtualFunction
            || mainType == ClangBackEnd::HighlightingType::Function
            || mainType == ClangBackEnd::HighlightingType::Operator) {
        if (storageClass != ClangBackEnd::StorageClass::Static) {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CPlusPlus::Icons::FuncPublicIconType;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CPlusPlus::Icons::FuncProtectedIconType;
            case ClangBackEnd::AccessSpecifier::Private:
                return CPlusPlus::Icons::FuncPrivateIconType;
            }
        } else {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CPlusPlus::Icons::FuncPublicStaticIconType;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CPlusPlus::Icons::FuncProtectedStaticIconType;
            case ClangBackEnd::AccessSpecifier::Private:
                return CPlusPlus::Icons::FuncPrivateStaticIconType;
            }
        }
    }
    if (mainType == ClangBackEnd::HighlightingType::GlobalVariable
            || mainType == ClangBackEnd::HighlightingType::Field) {
        if (storageClass != ClangBackEnd::StorageClass::Static) {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CPlusPlus::Icons::VarPublicIconType;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CPlusPlus::Icons::VarProtectedIconType;
            case ClangBackEnd::AccessSpecifier::Private:
                return CPlusPlus::Icons::VarPrivateIconType;
            }
        } else {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return CPlusPlus::Icons::VarPublicStaticIconType;
            case ClangBackEnd::AccessSpecifier::Protected:
                return CPlusPlus::Icons::VarProtectedStaticIconType;
            case ClangBackEnd::AccessSpecifier::Private:
                return CPlusPlus::Icons::VarPrivateStaticIconType;
            }
        }
    }

    return CPlusPlus::Icons::UnknownIconType;
}

QString diagnosticCategoryPrefixRemoved(const QString &text)
{
    QString theText = text;

    // Prefixes are taken from $LLVM_SOURCE_DIR/tools/clang/lib/Frontend/TextDiagnostic.cpp,
    // function TextDiagnostic::printDiagnosticLevel (llvm-3.6.2).
    static const QStringList categoryPrefixes = {
        QStringLiteral("note"),
        QStringLiteral("remark"),
        QStringLiteral("warning"),
        QStringLiteral("error"),
        QStringLiteral("fatal error")
    };

    for (const QString &prefix : categoryPrefixes) {
        const QString fullPrefix = prefix + QStringLiteral(": ");
        if (theText.startsWith(fullPrefix)) {
            theText.remove(0, fullPrefix.length());
            return theText;
        }
    }

    return text;
}

} // namespace Utils
} // namespace Clang
