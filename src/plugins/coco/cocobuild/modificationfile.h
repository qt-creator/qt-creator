// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QStringList>

namespace Coco::Internal {

class ModificationFile
{
public:
    ModificationFile();

    virtual void read() = 0;
    virtual void write() const = 0;

    virtual void setProjectDirectory(const Utils::FilePath &projectDirectory) = 0;

    virtual QString fileName() const = 0;
    QString nativePath() const { return m_filePath.nativePath(); }
    bool exists() const;

    const QStringList &options() const { return m_options; }
    void setOptions(const QString &options);

    const QStringList &tweaks() const { return m_tweaks; }
    void setTweaks(const QString &tweaks);

protected:
    void clear();

    virtual QStringList defaultModificationFile() const = 0;
    QStringList contentOf(const Utils::FilePath &filePath) const;
    QStringList currentModificationFile() const;

    void setFilePath(const Utils::FilePath &path) { m_filePath = path; }

    void setOptions(const QStringList &options);
    void setTweaks(const QStringList &tweaks);

private:
    QStringList m_options;
    QStringList m_tweaks;

    Utils::FilePath m_filePath;
};

} // namespace Coco::Internal
