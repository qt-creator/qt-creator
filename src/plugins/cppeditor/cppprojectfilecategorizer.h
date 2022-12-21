// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppprojectfile.h"

#include <projectexplorer/rawprojectpart.h>

#include <QString>
#include <QVector>

namespace CppEditor {

class CPPEDITOR_EXPORT ProjectFileCategorizer
{
public:
    using FileIsActive = ProjectExplorer::RawProjectPart::FileIsActive;
    using GetMimeType = ProjectExplorer::RawProjectPart::GetMimeType;

public:
    ProjectFileCategorizer(const QString &projectPartName,
                           const QStringList &filePaths,
                           const FileIsActive &fileIsActive = {},
                           const GetMimeType &getMimeType = {});

    bool hasCSources() const { return !m_cSources.isEmpty(); }
    bool hasCxxSources() const { return !m_cxxSources.isEmpty(); }
    bool hasObjcSources() const { return !m_objcSources.isEmpty(); }
    bool hasObjcxxSources() const { return !m_objcxxSources.isEmpty(); }

    ProjectFiles cSources() const { return m_cSources; }
    ProjectFiles cxxSources() const { return m_cxxSources; }
    ProjectFiles objcSources() const { return m_objcSources; }
    ProjectFiles objcxxSources() const { return m_objcxxSources; }

    bool hasMultipleParts() const { return m_partCount > 1; }
    bool hasParts() const { return m_partCount > 0; }

    QString partName(const QString &languageName) const;

private:
    ProjectFiles classifyFiles(const QStringList &filePaths,
                               const FileIsActive &fileIsActive,
                               const GetMimeType &getMimeType);
    void expandSourcesWithAmbiguousHeaders(const ProjectFiles &ambiguousHeaders);

private:
    QString m_partName;
    ProjectFiles m_cSources;
    ProjectFiles m_cxxSources;
    ProjectFiles m_objcSources;
    ProjectFiles m_objcxxSources;
    int m_partCount;
};

} // namespace CppEditor
