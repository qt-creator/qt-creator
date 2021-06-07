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

#include "itestparser.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanager.h>
#include <utils/textfileformat.h>
#include <utils/algorithm.h>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

namespace Autotest {

using LookupInfo = QPair<QString, QString>;
static QHash<LookupInfo, bool> s_pchLookupCache;

CppParser::CppParser(ITestFramework *framework)
    : ITestParser(framework)
{
}

void CppParser::init(const Utils::FilePaths &filesToParse, bool fullParse)
{
    Q_UNUSED(filesToParse)
    Q_UNUSED(fullParse)
    m_cppSnapshot = CppTools::CppModelManager::instance()->snapshot();
    m_workingCopy = CppTools::CppModelManager::instance()->workingCopy();
}

bool CppParser::selectedForBuilding(const Utils::FilePath &fileName)
{
    QList<CppTools::ProjectPart::Ptr> projParts =
            CppTools::CppModelManager::instance()->projectPart(fileName);

    return !projParts.isEmpty() && projParts.at(0)->selectedForBuilding;
}

QByteArray CppParser::getFileContent(const Utils::FilePath &filePath) const
{
    QByteArray fileContent;
    if (m_workingCopy.contains(filePath)) {
        fileContent = m_workingCopy.source(filePath);
    } else {
        QString error;
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        if (Utils::TextFileFormat::readFileUTF8(filePath, codec, &fileContent, &error)
                != Utils::TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << error;
        }
    }
    fileContent.replace("\r\n", "\n");
    return fileContent;
}

bool precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                               const Utils::FilePath &filePath,
                               const QString &cacheString,
                               const std::function<bool(const QString &)> &checker)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QList<CppTools::ProjectPart::Ptr> projectParts = modelManager->projectPart(filePath);
    if (projectParts.isEmpty())
        return false;
    const QStringList precompiledHeaders = projectParts.first()->precompiledHeaders;
    auto headerContains = [&](const QString &header){
        LookupInfo info{header, cacheString};
        auto it = s_pchLookupCache.find(info);
        if (it == s_pchLookupCache.end()) {
            it = s_pchLookupCache.insert(info,
                                         Utils::anyOf(snapshot.allIncludesForDocument(header),
                                                      checker));
        }
        return it.value();
    };
    return Utils::anyOf(precompiledHeaders, headerContains);
}

bool CppParser::precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath,
                                          const QString &headerFilePath)
{
    return Autotest::precompiledHeaderContains(snapshot,
                                               filePath,
                                               headerFilePath,
                                               [&](const QString &include) {
                                                   return include.endsWith(headerFilePath);
                                               });
}

bool CppParser::precompiledHeaderContains(const CPlusPlus::Snapshot &snapshot,
                                          const Utils::FilePath &filePath,
                                          const QRegularExpression &headerFileRegex)
{
    return Autotest::precompiledHeaderContains(snapshot,
                                               filePath,
                                               headerFileRegex.pattern(),
                                               [&](const QString &include) {
                                                   return headerFileRegex.match(include).hasMatch();
                                               });
}

void CppParser::release()
{
    m_cppSnapshot = CPlusPlus::Snapshot();
    m_workingCopy = CppTools::WorkingCopy();
    s_pchLookupCache.clear();
}

CPlusPlus::Document::Ptr CppParser::document(const Utils::FilePath &fileName)
{
    return selectedForBuilding(fileName) ? m_cppSnapshot.document(fileName) : nullptr;
}

} // namespace Autotest
