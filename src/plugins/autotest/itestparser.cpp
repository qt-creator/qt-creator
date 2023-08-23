// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itestparser.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppmodelmanager.h>
#include <projectexplorer/projectmanager.h>
#include <utils/textfileformat.h>
#include <utils/algorithm.h>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

using namespace Utils;

namespace Autotest {

using LookupInfo = QPair<QString, QString>;
static QHash<LookupInfo, bool> s_pchLookupCache;
Q_GLOBAL_STATIC(QMutex, s_cacheMutex);

CppParser::CppParser(ITestFramework *framework)
    : ITestParser(framework)
{
}

void CppParser::init(const QSet<FilePath> &filesToParse, bool fullParse)
{
    Q_UNUSED(filesToParse)
    Q_UNUSED(fullParse)
    m_cppSnapshot = CppEditor::CppModelManager::snapshot();
    m_workingCopy = CppEditor::CppModelManager::workingCopy();
}

bool CppParser::selectedForBuilding(const FilePath &fileName)
{
    QList<CppEditor::ProjectPart::ConstPtr> projParts =
            CppEditor::CppModelManager::projectPart(fileName);

    return !projParts.isEmpty() && projParts.at(0)->selectedForBuilding;
}

QByteArray CppParser::getFileContent(const FilePath &filePath) const
{
    QByteArray fileContent;
    if (const auto source = m_workingCopy.source(filePath)) {
        fileContent = *source;
    } else {
        QString error;
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        if (TextFileFormat::readFileUTF8(filePath, codec, &fileContent, &error)
                != TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << error;
        }
    }
    fileContent.replace("\r\n", "\n");
    return fileContent;
}

bool precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                               const FilePath &filePath,
                               const QString &cacheString,
                               const std::function<bool(const FilePath &)> &checker)
{
    const QList<CppEditor::ProjectPart::ConstPtr> projectParts
        = CppEditor::CppModelManager::projectPart(filePath);
    if (projectParts.isEmpty())
        return false;
    const QStringList precompiledHeaders = projectParts.first()->precompiledHeaders;
    auto headerContains = [&](const QString &header){
        LookupInfo info{header, cacheString};
        auto it = s_pchLookupCache.find(info);
        if (it == s_pchLookupCache.end()) {
            it = s_pchLookupCache.insert(info,
                         Utils::anyOf(snapshot.allIncludesForDocument(FilePath::fromString(header)),
                                      checker));
        }
        return it.value();
    };
    QMutexLocker l(s_cacheMutex());
    return Utils::anyOf(precompiledHeaders, headerContains);
}

bool CppParser::precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const FilePath &filePath,
                                          const QString &headerFilePath)
{
    return Autotest::precompiledHeaderContains(snapshot,
                                               filePath,
                                               headerFilePath,
                                               [&](const FilePath &include) {
                                                   return include.path().endsWith(headerFilePath);
                                               });
}

bool CppParser::precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const FilePath &filePath,
                                          const QRegularExpression &headerFileRegex)
{
    return Autotest::precompiledHeaderContains(snapshot,
                                               filePath,
                                               headerFileRegex.pattern(),
                                               [&](const FilePath &include) {
                                                   return headerFileRegex.match(include.path()).hasMatch();
                                               });
}

std::optional<QSet<FilePath>> CppParser::filesContainingMacro(const QByteArray &macroName)
{
    // safety net to avoid adding some option
    static const bool noPrefilter = qtcEnvironmentVariableIsSet("QTC_AUTOTEST_DISABLE_PREFILTER");
    if (noPrefilter)
        return std::nullopt;

    QSet<FilePath> result;
    CppEditor::ProjectInfo::ConstPtr infos = CppEditor::CppModelManager::projectInfo(
                ProjectExplorer::ProjectManager::startupProject());
    if (!infos)
        return std::nullopt;

    const auto projectParts = infos->projectParts();
    for (const auto &pp : projectParts) {
        if (!pp->selectedForBuilding)
            continue;

        const ProjectExplorer::Macros macros = pp->projectMacros;
        if (Utils::anyOf(pp->projectMacros, Utils::equal(&ProjectExplorer::Macro::key, macroName)))
            result.unite(Utils::transform<QSet>(pp->files, &CppEditor::ProjectFile::path));
    }
    return std::make_optional(result);
}

void CppParser::release()
{
    m_cppSnapshot = CPlusPlus::Snapshot();
    m_workingCopy = CppEditor::WorkingCopy();
    QMutexLocker l(s_cacheMutex());
    s_pchLookupCache.clear();
}

CPlusPlus::Document::Ptr CppParser::document(const FilePath &fileName)
{
    return selectedForBuilding(fileName) ? m_cppSnapshot.document(fileName) : nullptr;
}

} // namespace Autotest
