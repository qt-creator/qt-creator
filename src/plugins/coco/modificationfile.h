// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QStringList>

namespace ProjectExplorer {
class BuildConfiguration;
}

namespace Coco::Internal {

class ModificationFile
{
public:
    ModificationFile(const QString &fileName, const Utils::FilePath &defaultModificationFile);

    void setFilePath(ProjectExplorer::BuildConfiguration *buildConfig);

    QString fileName() const;
    QString nativePath() const { return m_filePath.nativePath(); }
    bool exists() const;

    const QStringList &options() const { return m_options; }
    void setOptions(const QString &options);
    void setOptions(const QStringList &options);

    const QStringList &tweaks() const { return m_tweaks; }
    void setTweaks(const QString &tweaks);
    void setTweaks(const QStringList &tweaks);

    void clear();

    QStringList defaultModificationFile() const;
    QStringList currentModificationFile() const;

private:
    QStringList contentOf(const Utils::FilePath &filePath) const;

    const QString m_fileName;
    const Utils::FilePath m_defaultModificationFile;

    QStringList m_options;
    QStringList m_tweaks;

    Utils::FilePath m_filePath;
};

} // namespace Coco::Internal
