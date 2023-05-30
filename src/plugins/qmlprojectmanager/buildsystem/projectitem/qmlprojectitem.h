// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "filefilteritems.h"

#include <utils/environment.h>

#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>

#include <memory>
#include <vector>

namespace QmlJS {
class SimpleReaderNode;
}

namespace QmlProjectManager {

class QmlProjectItem : public QObject
{
    Q_OBJECT
public:
    explicit QmlProjectItem(const Utils::FilePath &filePath, const bool skipRewrite = false);

    bool isQt4McuProject() const;

    QString versionQt() const;
    void setVersionQt(const QString &version);

    QString versionQtQuick() const;
    void setVersionQtQuick(const QString &version);

    QString versionDesignStudio() const;
    void setVersionDesignStudio(const QString &version);

    Utils::FilePath sourceDirectory() const;
    QString targetDirectory() const;

    QStringList importPaths() const;
    void setImportPaths(const QStringList &paths);
    void addImportPath(const QString &importPath);

    QStringList fileSelectors() const;
    void setFileSelectors(const QStringList &selectors);
    void addFileSelector(const QString &selector);

    bool multilanguageSupport() const;
    void setMultilanguageSupport(const bool &isEnabled);

    QStringList supportedLanguages() const;
    void setSupportedLanguages(const QStringList &languages);
    void addSupportedLanguage(const QString &language);

    QString primaryLanguage() const;
    void setPrimaryLanguage(const QString &language);

    Utils::FilePaths files() const;
    bool matchesFile(const QString &filePath) const;

    bool forceFreeType() const;
    void setForceFreeType(const bool &isForced);

    void setMainFile(const QString &mainFile);
    QString mainFile() const;

    void setMainUiFile(const QString &mainUiFile);
    QString mainUiFile() const;

    bool widgetApp() const;
    void setWidgetApp(const bool &widgetApp);

    QStringList shaderToolArgs() const;
    void setShaderToolArgs(const QStringList &args);
    void addShaderToolArg(const QString &arg);

    QStringList shaderToolFiles() const;
    void setShaderToolFiles(const QStringList &files);
    void addShaderToolFile(const QString &file);

    Utils::EnvironmentItems environment() const;
    void addToEnviroment(const QString &key, const QString &value);

    QJsonObject project() const;

signals:
    void qmlFilesChanged(const QSet<QString> &, const QSet<QString> &);

private:
    typedef QSharedPointer<QmlProjectItem> ShrdPtrQPI;
    typedef std::unique_ptr<FileFilterItem> UnqPtrFFBI;
    typedef std::unique_ptr<FileFilterItem> UnqPtrFFI;

    // files & props
    std::vector<std::unique_ptr<FileFilterItem>> m_content; // content property

    // runtime variables
    Utils::FilePath m_projectFile; // design studio project file
    QJsonObject m_project;         // root project object
    const bool m_skipRewrite;

    // initializing functions
    bool initProjectObject();
    void setupFileFilters();

    // file update functions
    void insertAndUpdateProjectFile(const QString &key, const QJsonValue &value);
};

} // namespace QmlProjectManager
