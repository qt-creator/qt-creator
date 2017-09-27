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

static QString creatorResourcePath()
{
#ifndef UNIT_TESTS
    return Core::ICore::instance()->resourcePath();
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
            add(includeDirOption() + QDir::toNativeSeparators(wrappedQtHeadersPath));
            add(includeDirOption() + QDir::toNativeSeparators(wrappedQtCoreHeaderPath));
        }
    }

    void addDummyUiHeaderOnDiskIncludePath()
    {
        const QString path = ModelManagerSupportClang::instance()->dummyUiHeaderOnDiskDirPath();
        if (!path.isEmpty())
            add(includeDirOption() + QDir::toNativeSeparators(path));
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

// CLANG-UPGRADE-CHECK: Workaround still needed?
// Remove once clang reports correct columns for lines with multi-byte utf8.
int extraUtf8CharsShift(const QString &str, int column)
{
    int shift = 0;
    const QByteArray byteArray = str.toUtf8();
    for (int i = 0; i < qMin(str.length(), column); ++i) {
        const uchar firstByte = static_cast<uchar>(byteArray.at(i));
        // Skip different amount of bytes depending on value
        if (firstByte < 0xC0) {
            continue;
        } else if (firstByte < 0xE0) {
            ++shift;
            ++i;
        } else if (firstByte < 0xF0) {
            shift += 2;
            i += 2;
        } else {
            shift += 3;
            i += 3;
        }
    }
    return shift;
}


} // namespace Utils
} // namespace Clang
