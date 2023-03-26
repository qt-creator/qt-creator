// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <utils/algorithm.h>

#include <QDir>

using namespace Utils;

namespace QmlProjectManager {

// kind of initialization
void QmlProjectItem::setSourceDirectory(const FilePath &directoryPath)
{
    if (m_sourceDirectory == directoryPath)
        return;

    m_sourceDirectory = directoryPath;

    for (auto &fileFilter : m_content) {
        fileFilter->setDefaultDirectory(directoryPath.toFSPathString());
        connect(fileFilter.get(),
                &FileFilterBaseItem::filesChanged,
                this,
                &QmlProjectItem::qmlFilesChanged);
    }
}

void QmlProjectItem::setTargetDirectory(const FilePath &directoryPath)
{
    m_targetDirectory = directoryPath;
}

void QmlProjectItem::setQtForMCUs(bool b)
{
    m_qtForMCUs = b;
}

void QmlProjectItem::setQt6Project(bool qt6Project)
{
    m_qt6Project = qt6Project;
}

void QmlProjectItem::setImportPaths(const QStringList &importPaths)
{
    if (m_importPaths != importPaths)
        m_importPaths = importPaths;
}

void QmlProjectItem::setFileSelectors(const QStringList &selectors)
{
    if (m_fileSelectors != selectors)
        m_fileSelectors = selectors;
}

void QmlProjectItem::setMultilanguageSupport(const bool isEnabled)
{
    m_multilanguageSupport = isEnabled;
}

void QmlProjectItem::setSupportedLanguages(const QStringList &languages)
{
    if (m_supportedLanguages != languages)
        m_supportedLanguages = languages;
}

void QmlProjectItem::setPrimaryLanguage(const QString &language)
{
    if (m_primaryLanguage != language)
        m_primaryLanguage = language;
}

/* Returns list of absolute paths */
QStringList QmlProjectItem::files() const
{
    QSet<QString> files;

    for (const auto &fileFilter : m_content) {
        const QStringList fileList = fileFilter->files();
        for (const QString &file : fileList) {
            files.insert(file);
        }
    }
    return Utils::toList(files);
}

/**
  Check whether the project would include a file path
  - regardless whether the file already exists or not.

  @param filePath: absolute file path to check
  */
bool QmlProjectItem::matchesFile(const QString &filePath) const
{
    return Utils::contains(m_content, [&filePath](const auto &fileFilter) {
        return fileFilter->matchesFile(filePath);
    });
}

void QmlProjectItem::setForceFreeType(bool b)
{
    m_forceFreeType = b;
}

Utils::EnvironmentItems QmlProjectItem::environment() const
{
    return m_environment;
}

void QmlProjectItem::addToEnviroment(const QString &key, const QString &value)
{
    m_environment.append(Utils::EnvironmentItem(key, value));
}

} // namespace QmlProjectManager
