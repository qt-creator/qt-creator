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

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/editordocumenthandle.h>
#include <cpptools/projectpart.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QStringList>

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

static QString getResourceDir()
{
    QDir dir(ICore::libexecPath()
                + QLatin1String("/clang/lib/clang/")
                + QLatin1String(CLANG_VERSION)
                + QLatin1String("/include"));
    if (!dir.exists() || !QFileInfo(dir, QLatin1String("stdint.h")).exists())
        dir = QDir(QLatin1String(CLANG_RESOURCE_DIR));
    return dir.canonicalPath();
}

class LibClangOptionsBuilder : public CompilerOptionsBuilder
{
public:
    static QStringList build(const ProjectPart::Ptr &projectPart, ProjectFile::Kind fileKind)
    {
        if (projectPart.isNull())
            return QStringList();

        LibClangOptionsBuilder optionsBuilder(*projectPart.data());

        optionsBuilder.addWordWidth();
        optionsBuilder.addTargetTriple();
        optionsBuilder.addLanguageOption(fileKind);
        optionsBuilder.addOptionsForLanguage(/*checkForBorlandExtensions*/ true);
        optionsBuilder.enableExceptions();

        optionsBuilder.addDefineToAvoidIncludingGccOrMinGwIntrinsics();
        optionsBuilder.addDefineFloat128ForMingw();
        optionsBuilder.addToolchainAndProjectDefines();
        optionsBuilder.undefineCppLanguageFeatureMacrosForMsvc2015();

        optionsBuilder.addPredefinedMacrosAndHeaderPathsOptions();
        optionsBuilder.addWrappedQtHeadersIncludePath();
        optionsBuilder.addHeaderPathOptions();
        optionsBuilder.addDummyUiHeaderOnDiskIncludePath();
        optionsBuilder.addProjectConfigFileInclude();

        optionsBuilder.addMsvcCompatibilityVersion();

        optionsBuilder.addExtraOptions();

        return optionsBuilder.options();
    }

private:
    LibClangOptionsBuilder(const CppTools::ProjectPart &projectPart)
        : CompilerOptionsBuilder(projectPart)
    {
    }

    bool excludeHeaderPath(const QString &path) const override
    {
        if (m_projectPart.toolchainType == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
            if (path.contains(QLatin1String("lib/gcc/i686-apple-darwin")))
                return true;
        }

        return CompilerOptionsBuilder::excludeHeaderPath(path);
    }

    void addPredefinedMacrosAndHeaderPathsOptions()
    {
        if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            addPredefinedMacrosAndHeaderPathsOptionsForMsvc();
        else
            addPredefinedMacrosAndHeaderPathsOptionsForNonMsvc();
    }

    void addPredefinedMacrosAndHeaderPathsOptionsForMsvc()
    {
        add(QLatin1String("-nostdinc"));
        add(QLatin1String("-undef"));
    }

    void addPredefinedMacrosAndHeaderPathsOptionsForNonMsvc()
    {
        static const QString resourceDir = getResourceDir();
        if (QTC_GUARD(!resourceDir.isEmpty())) {
            add(QLatin1String("-nostdlibinc"));
            add(QLatin1String("-I") + QDir::toNativeSeparators(resourceDir));
            add(QLatin1String("-undef"));
        }
    }

    void addWrappedQtHeadersIncludePath()
    {
        static const QString wrappedQtHeadersPath = ICore::instance()->resourcePath()
                + QLatin1String("/cplusplus/wrappedQtHeaders");

        if (m_projectPart.qtVersion != ProjectPart::NoQt) {
            const QString wrappedQtCoreHeaderPath = wrappedQtHeadersPath + QLatin1String("/QtCore");
            add(QLatin1String("-I") + QDir::toNativeSeparators(wrappedQtHeadersPath));
            add(QLatin1String("-I") + QDir::toNativeSeparators(wrappedQtCoreHeaderPath));
        }
    }

    void addProjectConfigFileInclude()
    {
        if (!m_projectPart.projectConfigFile.isEmpty()) {
            add(QLatin1String("-include"));
            add(QDir::toNativeSeparators(m_projectPart.projectConfigFile));
        }
    }

    void addDummyUiHeaderOnDiskIncludePath()
    {
        const QString path = ModelManagerSupportClang::instance()->dummyUiHeaderOnDiskDirPath();
        if (!path.isEmpty())
            add(includeDirOption() + QDir::toNativeSeparators(path));
    }

    void addExtraOptions()
    {
        add(QLatin1String("-fmessage-length=0"));
        add(QLatin1String("-fdiagnostics-show-note-include-stack"));
        add(QLatin1String("-fmacro-backtrace-limit=0"));
        add(QLatin1String("-fretain-comments-from-system-headers"));
        add(QLatin1String("-ferror-limit=1000"));
    }
};

/**
 * @brief Creates list of message-line arguments required for correct parsing
 * @param pPart Null if file isn't part of any project
 * @param fileKind Determines language and source/header state
 */
QStringList createClangOptions(const ProjectPart::Ptr &pPart, ProjectFile::Kind fileKind)
{
    return LibClangOptionsBuilder::build(pPart, fileKind);
}

ProjectPart::Ptr projectPartForFile(const QString &filePath)
{
    if (const auto parser = CppTools::BaseEditorDocumentParser::get(filePath))
        return parser->projectPart();
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

} // namespace Utils
} // namespace Clang
