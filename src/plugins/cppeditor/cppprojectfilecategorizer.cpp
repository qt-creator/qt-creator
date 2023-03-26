// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppprojectfilecategorizer.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>

using namespace Utils;

namespace CppEditor {

ProjectFileCategorizer::ProjectFileCategorizer(const QString &projectPartName,
                                               const QStringList &filePaths,
                                               const FileIsActive &fileIsActive,
                                               const GetMimeType &getMimeType)
    : m_partName(projectPartName)
{
    const ProjectFiles ambiguousHeaders = classifyFiles(filePaths, fileIsActive, getMimeType);
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
                                                   const FileIsActive &fileIsActive,
                                                   const GetMimeType &getMimeType)
{
    ProjectFiles ambiguousHeaders;

    for (const QString &filePath : filePaths) {
        const ProjectFile projectFile(FilePath::fromString(filePath),
                                      getMimeType
                                          ? ProjectFile::classifyByMimeType(getMimeType(filePath))
                                          : ProjectFile::classify(filePath),
                                      fileIsActive ? fileIsActive(filePath) : true);

        switch (projectFile.kind) {
        case ProjectFile::AmbiguousHeader:
            ambiguousHeaders += projectFile;
            break;
        case ProjectFile::CXXSource:
        case ProjectFile::CXXHeader:
        case ProjectFile::CudaSource:
        case ProjectFile::OpenCLSource:
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
        case ProjectFile::Unclassified:
        case ProjectFile::Unsupported:
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

} // namespace CppEditor
