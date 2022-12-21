// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filefilteritems.h"

#include <utils/environment.h>
#include <utils/filepath.h>

#include <QObject>
#include <QSet>
#include <QStringList>

#include <memory>
#include <vector>

namespace QmlProjectManager {

class QmlProjectItem : public QObject
{
    Q_OBJECT

public:
    const Utils::FilePath &sourceDirectory() const { return m_sourceDirectory; }
    void setSourceDirectory(const Utils::FilePath &directoryPath);
    const Utils::FilePath &targetDirectory() const { return m_targetDirectory; }
    void setTargetDirectory(const Utils::FilePath &directoryPath);

    bool qtForMCUs() const { return m_qtForMCUs; }
    void setQtForMCUs(bool qtForMCUs);

    bool qt6Project() const { return m_qt6Project; }
    void setQt6Project(bool qt6Project);

    QStringList importPaths() const { return m_importPaths; }
    void setImportPaths(const QStringList &paths);

    QStringList fileSelectors() const { return m_fileSelectors; }
    void setFileSelectors(const QStringList &selectors);

    bool multilanguageSupport() const { return m_multilanguageSupport; }
    void setMultilanguageSupport(const bool isEnabled);

    QStringList supportedLanguages() const { return m_supportedLanguages; }
    void setSupportedLanguages(const QStringList &languages);

    QString primaryLanguage() const { return m_primaryLanguage; }
    void setPrimaryLanguage(const QString &language);

    QStringList files() const;
    bool matchesFile(const QString &filePath) const;

    bool forceFreeType() const { return m_forceFreeType; };
    void setForceFreeType(bool);

    QString mainFile() const { return m_mainFile; }
    void setMainFile(const QString &mainFilePath) { m_mainFile = mainFilePath; }

    QString mainUiFile() const { return m_mainUiFile; }
    void setMainUiFile(const QString &mainUiFilePath) { m_mainUiFile = mainUiFilePath; }

    bool widgetApp() const { return m_widgetApp; }
    void setWidgetApp(bool widgetApp) { m_widgetApp = widgetApp; }

    QStringList shaderToolArgs() const { return m_shaderToolArgs; }
    void setShaderToolArgs(const QStringList &args) {m_shaderToolArgs = args; }

    QStringList shaderToolFiles() const { return m_shaderToolFiles; }
    void setShaderToolFiles(const QStringList &files) { m_shaderToolFiles = files; }

    void appendContent(std::unique_ptr<FileFilterBaseItem> item)
    {
        m_content.push_back(std::move(item));
    }

    Utils::EnvironmentItems environment() const;
    void addToEnviroment(const QString &key, const QString &value);

signals:
    void qmlFilesChanged(const QSet<QString> &, const QSet<QString> &);

protected:
    Utils::FilePath m_sourceDirectory;
    Utils::FilePath m_targetDirectory;
    QStringList m_importPaths;
    QStringList m_fileSelectors;
    bool m_multilanguageSupport;
    QStringList m_supportedLanguages;
    QString m_primaryLanguage;
    QString m_mainFile;
    QString m_mainUiFile;
    Utils::EnvironmentItems m_environment;
    std::vector<std::unique_ptr<FileFilterBaseItem>> m_content; // content property
    bool m_forceFreeType = false;
    bool m_qtForMCUs = false;
    bool m_qt6Project = false;
    bool m_widgetApp = false;
    QStringList m_shaderToolArgs;
    QStringList m_shaderToolFiles;
};

} // namespace QmlProjectManager
