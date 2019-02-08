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

#include "cppprojectfilecategorizer.h"

#include <utils/algorithm.h>

namespace CppTools {

ProjectFileCategorizer::ProjectFileCategorizer(const QString &projectPartName,
                                               const QStringList &filePaths,
                                               const FileClassifier &fileClassifier)
    : m_partName(projectPartName)
{
    const ProjectFiles ambiguousHeaders = classifyFiles(filePaths, fileClassifier);
    expandSourcesWithAmbiguousHeaders(ambiguousHeaders);

    m_partCount = (m_cSources.isEmpty() ? 0 : 1)
                + (m_cxxSources.isEmpty() ? 0 : 1)
                + (m_objcSources.isEmpty() ? 0 : 1)
                + (m_objcxxSources.isEmpty() ? 0 : 1);
}

// TODO: Always tell the language version?
QString ProjectFileCategorizer::partName(const QString &languageName) const
{
    if (hasMultipleParts())
        return QString::fromLatin1("%1 (%2)").arg(m_partName, languageName);

    return m_partName;
}

ProjectFiles ProjectFileCategorizer::classifyFiles(const QStringList &filePaths,
                                                   const FileClassifier &fileClassifier)
{
    ProjectFiles ambiguousHeaders;

    foreach (const QString &filePath, filePaths) {
        const ProjectFile projectFile = fileClassifier
                ? fileClassifier(filePath)
                : ProjectFile(filePath, ProjectFile::classify(filePath));

        switch (projectFile.kind) {
        case ProjectFile::AmbiguousHeader:
            ambiguousHeaders += projectFile;
            break;
        case ProjectFile::CXXSource:
        case ProjectFile::CXXHeader:
            m_cxxSources += projectFile;
            break;
        case ProjectFile::ObjCXXSource:
        case ProjectFile::ObjCXXHeader:
            m_objcxxSources += projectFile;
            break;
        case ProjectFile::CSource:
        case ProjectFile::CHeader:
            m_cSources += projectFile;
            break;
        case ProjectFile::ObjCSource:
        case ProjectFile::ObjCHeader:
            m_objcSources += projectFile;
            break;
        default:
            continue;
        }
    }

    return ambiguousHeaders;
}

static ProjectFiles toProjectFilesWithKind(const ProjectFiles &ambiguousHeaders,
                                           const ProjectFile::Kind overriddenKind)
{
    return Utils::transform(ambiguousHeaders, [overriddenKind](const ProjectFile &projectFile) {
        return ProjectFile(projectFile.path, overriddenKind, projectFile.active);
    });
}

void ProjectFileCategorizer::expandSourcesWithAmbiguousHeaders(const ProjectFiles &ambiguousHeaders)
{
    const bool hasC = !m_cSources.isEmpty();
    const bool hasCxx = !m_cxxSources.isEmpty();
    const bool hasObjc = !m_objcSources.isEmpty();
    const bool hasObjcxx = !m_objcxxSources.isEmpty();
    const bool hasOnlyAmbiguousHeaders
             = !hasC
            && !hasCxx
            && !hasObjc
            && !hasObjcxx
            && !ambiguousHeaders.isEmpty();

    if (hasC || hasOnlyAmbiguousHeaders)
        m_cSources += toProjectFilesWithKind(ambiguousHeaders, ProjectFile::CHeader);

    if (hasCxx || hasOnlyAmbiguousHeaders)
        m_cxxSources += toProjectFilesWithKind(ambiguousHeaders, ProjectFile::CXXHeader);

    if (hasObjc || hasOnlyAmbiguousHeaders)
        m_objcSources += toProjectFilesWithKind(ambiguousHeaders, ProjectFile::ObjCHeader);

    if (hasObjcxx || hasOnlyAmbiguousHeaders)
        m_objcxxSources += toProjectFilesWithKind(ambiguousHeaders, ProjectFile::ObjCXXHeader);
}

} // namespace CppTools
