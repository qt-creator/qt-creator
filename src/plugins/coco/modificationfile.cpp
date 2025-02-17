// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modificationfile.h"
#include "projectexplorer/project.h"

#include <projectexplorer/buildconfiguration.h>

namespace Coco::Internal {

static void cutTail(QStringList &list)
{
    while (!list.isEmpty() && list.last().trimmed().isEmpty())
        list.removeLast();
}

ModificationFile::ModificationFile(const QString &fileName, const Utils::FilePath &defaultModificationFile)
    : m_fileName{fileName}
    , m_defaultModificationFile{defaultModificationFile}
{}

void ModificationFile::setFilePath(ProjectExplorer::BuildConfiguration *buildConfig)
{
    Utils::FilePath projectDirectory = buildConfig->project()->projectDirectory();
    m_filePath = projectDirectory.pathAppended(fileName());
}

QString ModificationFile::fileName() const
{
    return m_fileName;
}

bool ModificationFile::exists() const
{
    return m_filePath.exists();
}

void ModificationFile::clear()
{
    m_options.clear();
    m_tweaks.clear();
}

QStringList ModificationFile::defaultModificationFile() const
{
    return contentOf(m_defaultModificationFile);
}

QStringList ModificationFile::contentOf(const Utils::FilePath &filePath) const
{
    QFile resource(filePath.nativePath());
    QTC_CHECK(resource.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream inStream(&resource);

    QStringList result;
    QString line;
    while (inStream.readLineInto(&line))
        result << line + '\n';
    return result;
}

QStringList ModificationFile::currentModificationFile() const
{
    QStringList lines;
    if (m_filePath.exists())
        lines = contentOf(m_filePath);
    else
        lines = defaultModificationFile();

    return lines;
}

void ModificationFile::setOptions(const QString &options)
{
    m_options = options.split('\n', Qt::SkipEmptyParts);
}

void ModificationFile::setOptions(const QStringList &options)
{
    m_options = options;
}

void ModificationFile::setTweaks(const QString &tweaks)
{
    m_tweaks = tweaks.split('\n', Qt::KeepEmptyParts);
    cutTail(m_tweaks);
}

void ModificationFile::setTweaks(const QStringList &tweaks)
{
    m_tweaks = tweaks;
    cutTail(m_tweaks);
}

} // namespace Coco::Internal
