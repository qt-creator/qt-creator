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
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTextBlock>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace Core;
using namespace CppTools;

namespace ClangCodeModel {
namespace Utils {

class LibClangOptionsBuilder final : public CompilerOptionsBuilder
{
public:
    LibClangOptionsBuilder(const ProjectPart &projectPart)
        : CompilerOptionsBuilder(projectPart,
                                 UseSystemHeader::No,
                                 CppTools::SkipBuiltIn::No,
                                 CppTools::SkipLanguageDefines::Yes,
                                 QString(CLANG_VERSION),
                                 QString(CLANG_RESOURCE_DIR))
    {
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
    void addDummyUiHeaderOnDiskIncludePath()
    {
        const QString path = ClangModelManagerSupport::instance()->dummyUiHeaderOnDiskDirPath();
        if (!path.isEmpty()) {
            add("-I");
            add(QDir::toNativeSeparators(path));
        }
    }
};

QStringList createClangOptions(const ProjectPart &projectPart, ProjectFile::Kind fileKind)
{
    return LibClangOptionsBuilder(projectPart)
        .build(fileKind, CompilerOptionsBuilder::PchUsage::None);
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
    // '- 1' and '+ 1' are because of 1-based columns
    return line.text().left(cppEditorColumn - 1).toUtf8().size() + 1;
}

int cppEditorColumn(const QTextBlock &line, int clangColumn)
{
    // (1) clangColumn is the column in Clang which is the utf8 byte offset from the beginning
    //     of the line.
    // (2) The return value is the actual column shown by CppEditor.
    // Here we convert column from (1) to (2).
    // '- 1' and '+ 1' are because of 1-based columns
    return QString::fromUtf8(line.text().toUtf8().left(clangColumn - 1)).size() + 1;
}

::Utils::CodeModelIcon::Type iconTypeForToken(const ClangBackEnd::TokenInfoContainer &token)
{
    const ClangBackEnd::ExtraInfo &extraInfo = token.extraInfo;
    if (extraInfo.signal)
        return ::Utils::CodeModelIcon::Signal;

    ClangBackEnd::AccessSpecifier access = extraInfo.accessSpecifier;
    if (extraInfo.slot) {
        switch (access) {
        case ClangBackEnd::AccessSpecifier::Public:
        case ClangBackEnd::AccessSpecifier::Invalid:
            return ::Utils::CodeModelIcon::SlotPublic;
        case ClangBackEnd::AccessSpecifier::Protected:
            return ::Utils::CodeModelIcon::SlotProtected;
        case ClangBackEnd::AccessSpecifier::Private:
            return ::Utils::CodeModelIcon::SlotPrivate;
        }
    }

    ClangBackEnd::HighlightingType mainType = token.types.mainHighlightingType;

    if (mainType == ClangBackEnd::HighlightingType::QtProperty)
        return ::Utils::CodeModelIcon::Property;

    if (mainType == ClangBackEnd::HighlightingType::PreprocessorExpansion
            || mainType == ClangBackEnd::HighlightingType::PreprocessorDefinition) {
        return ::Utils::CodeModelIcon::Macro;
    }

    if (mainType == ClangBackEnd::HighlightingType::Enumeration)
        return ::Utils::CodeModelIcon::Enumerator;

    if (mainType == ClangBackEnd::HighlightingType::Type
            || mainType == ClangBackEnd::HighlightingType::Keyword) {
        const ClangBackEnd::MixinHighlightingTypes &types = token.types.mixinHighlightingTypes;
        if (types.contains(ClangBackEnd::HighlightingType::Enum))
            return ::Utils::CodeModelIcon::Enum;
        if (types.contains(ClangBackEnd::HighlightingType::Struct))
            return ::Utils::CodeModelIcon::Struct;
        if (types.contains(ClangBackEnd::HighlightingType::Namespace))
            return ::Utils::CodeModelIcon::Namespace;
        if (types.contains(ClangBackEnd::HighlightingType::Class))
            return ::Utils::CodeModelIcon::Class;
        if (mainType == ClangBackEnd::HighlightingType::Keyword)
            return ::Utils::CodeModelIcon::Keyword;
        return ::Utils::CodeModelIcon::Class;
    }

    ClangBackEnd::StorageClass storageClass = extraInfo.storageClass;
    if (mainType == ClangBackEnd::HighlightingType::VirtualFunction
            || mainType == ClangBackEnd::HighlightingType::Function
            || token.types.mixinHighlightingTypes.contains(
                ClangBackEnd::HighlightingType::Operator)) {
        if (storageClass != ClangBackEnd::StorageClass::Static) {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return ::Utils::CodeModelIcon::FuncPublic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return ::Utils::CodeModelIcon::FuncProtected;
            case ClangBackEnd::AccessSpecifier::Private:
                return ::Utils::CodeModelIcon::FuncPrivate;
            }
        } else {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return ::Utils::CodeModelIcon::FuncPublicStatic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return ::Utils::CodeModelIcon::FuncProtectedStatic;
            case ClangBackEnd::AccessSpecifier::Private:
                return ::Utils::CodeModelIcon::FuncPrivateStatic;
            }
        }
    }
    if (mainType == ClangBackEnd::HighlightingType::GlobalVariable
            || mainType == ClangBackEnd::HighlightingType::Field) {
        if (storageClass != ClangBackEnd::StorageClass::Static) {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return ::Utils::CodeModelIcon::VarPublic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return ::Utils::CodeModelIcon::VarProtected;
            case ClangBackEnd::AccessSpecifier::Private:
                return ::Utils::CodeModelIcon::VarPrivate;
            }
        } else {
            switch (access) {
            case ClangBackEnd::AccessSpecifier::Public:
            case ClangBackEnd::AccessSpecifier::Invalid:
                return ::Utils::CodeModelIcon::VarPublicStatic;
            case ClangBackEnd::AccessSpecifier::Protected:
                return ::Utils::CodeModelIcon::VarProtectedStatic;
            case ClangBackEnd::AccessSpecifier::Private:
                return ::Utils::CodeModelIcon::VarPrivateStatic;
            }
        }
    }

    return ::Utils::CodeModelIcon::Unknown;
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

static ::Utils::FileName buildDirectory(const CppTools::ProjectPart &projectPart)
{
    ProjectExplorer::Target *target = projectPart.project->activeTarget();
    if (!target)
        return ::Utils::FileName();

    ProjectExplorer::BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    if (!buildConfig)
        return ::Utils::FileName();

    return buildConfig->buildDirectory();
}

static QJsonObject createFileObject(CompilerOptionsBuilder &optionsBuilder,
                                    const ProjectFile &projFile,
                                    const ::Utils::FileName &buildDir)
{
    const ProjectFile::Kind kind = ProjectFile::classify(projFile.path);
    optionsBuilder.updateLanguageOption(kind);

    QJsonObject fileObject;
    fileObject["file"] = projFile.path;
    QJsonArray args = QJsonArray::fromStringList(optionsBuilder.options());
    args.prepend(kind == ProjectFile::CXXSource ? "clang++" : "clang");
    args.append(QDir::toNativeSeparators(projFile.path));
    fileObject["arguments"] = args;
    fileObject["directory"] = buildDir.toString();
    return fileObject;
}

void generateCompilationDB(::Utils::FileName projectDir, CppTools::ProjectInfo projectInfo)
{
    QFile compileCommandsFile(projectDir.toString() + "/compile_commands.json");

    compileCommandsFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    compileCommandsFile.write("[");
    for (ProjectPart::Ptr projectPart : projectInfo.projectParts()) {
        const ::Utils::FileName buildDir = buildDirectory(*projectPart);

        CompilerOptionsBuilder optionsBuilder(*projectPart,
                                              CppTools::UseSystemHeader::No,
                                              CppTools::SkipBuiltIn::Yes);
        optionsBuilder.build(CppTools::ProjectFile::Unclassified,
                             CppTools::CompilerOptionsBuilder::PchUsage::None);

        for (const ProjectFile &projFile : projectPart->files) {
            const QJsonObject json = createFileObject(optionsBuilder, projFile, buildDir);
            if (compileCommandsFile.size() > 1)
                compileCommandsFile.write(",");
            compileCommandsFile.write('\n' + QJsonDocument(json).toJson().trimmed());
        }
    }

    compileCommandsFile.write("\n]");
    compileCommandsFile.close();
}

} // namespace Utils
} // namespace Clang
