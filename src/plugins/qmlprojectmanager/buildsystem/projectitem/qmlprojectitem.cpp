// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprojectitem.h"

#include <QDir>
#include <QJsonDocument>

#include "converters.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <qmljs/qmljssimplereader.h>

#include <QJsonArray>

namespace QmlProjectManager {

//#define REWRITE_PROJECT_FILE_IN_JSON_FORMAT

QmlProjectItem::QmlProjectItem(const Utils::FilePath &filePath, const bool skipRewrite)
    : m_projectFile(filePath)
    , m_skipRewrite(skipRewrite)
{
    if (initProjectObject())
        setupFileFilters();
}

bool QmlProjectItem::initProjectObject()
{
    auto contents = m_projectFile.fileContents();
    if (!contents) {
        qWarning() << "Cannot open project file. Path:" << m_projectFile.fileName();
        return false;
    }

    QString fileContent{QString::fromUtf8(contents.value())};
    QJsonObject rootObj;
    QJsonParseError parseError;

    if (fileContent.contains("import qmlproject", Qt::CaseInsensitive)) {
        rootObj = Converters::qmlProjectTojson(m_projectFile);
#ifdef REWRITE_PROJECT_FILE_IN_JSON_FORMAT
        m_projectFile.writeFileContents(QJsonDocument(rootObj).toJson());
#endif
    } else {
        rootObj
                = QJsonDocument::fromJson(m_projectFile.fileContents()->data(), &parseError).object();
    }

    if (rootObj.isEmpty()) {
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Cannot parse the json formatted project file. Error:"
                       << parseError.errorString();
        } else {
            qWarning() << "Cannot convert QmlProject to Json.";
        }
        return false;
    }

    m_project = rootObj;
    return true;
}

void QmlProjectItem::setupFileFilters()
{
    auto setupFileFilterItem = [this](const QJsonObject &fileGroup) {
        for (const QString &directory : fileGroup["directories"].toVariant().toStringList()) {
            std::unique_ptr<FileFilterItem> fileFilterItem{new FileFilterItem};

            QStringList filesArr;
            for (const QJsonValue &file : fileGroup["files"].toArray()) {
                filesArr.append(file["name"].toString());
            }

            fileFilterItem->setDirectory(directory);
            fileFilterItem->setFilters(fileGroup["filters"].toVariant().toStringList());
            fileFilterItem->setRecursive(fileGroup["recursive"].toBool(true));
            fileFilterItem->setPathsProperty(fileGroup["directories"].toVariant().toStringList());
            fileFilterItem->setPathsProperty(filesArr);
            fileFilterItem->setDefaultDirectory(m_projectFile.parentDir().toString());
#ifndef TESTS_ENABLED_QMLPROJECTITEM
            connect(fileFilterItem.get(),
                    &FileFilterItem::filesChanged,
                    this,
                    &QmlProjectItem::qmlFilesChanged);
#endif
            m_content.push_back(std::move(fileFilterItem));
        };
    };

    QJsonObject fileGroups = m_project["fileGroups"].toObject();
    for (const QString &groupName : fileGroups.keys()) {
        setupFileFilterItem(fileGroups[groupName].toObject());
    }
}

Utils::FilePath QmlProjectItem::sourceDirectory() const
{
    return m_projectFile.parentDir();
}

QString QmlProjectItem::targetDirectory() const
{
    return m_project["deployment"].toObject()["targetDirectory"].toString();
}

bool QmlProjectItem::isQt4McuProject() const
{
    return m_project["mcuConfig"].toObject()["mcuEnabled"].toBool();
}

Utils::EnvironmentItems QmlProjectItem::environment() const
{
    Utils::EnvironmentItems envItems;
    QJsonObject envVariables = m_project["environment"].toObject();
    const QStringList variableNames = envVariables.keys();
    for (const QString &variableName : variableNames)
        envItems.append(Utils::EnvironmentItem(variableName, envVariables[variableName].toString()));
    return envItems;
}

void QmlProjectItem::addToEnviroment(const QString &key, const QString &value)
{
    QJsonObject envVariables = m_project["environment"].toObject();
    envVariables.insert(key, value);
    insertAndUpdateProjectFile("environment", envVariables);
}

QJsonObject QmlProjectItem::project() const
{
    return m_project;
}

QString QmlProjectItem::versionQt() const
{
    return m_project["versions"].toObject()["qt"].toString();
}

void QmlProjectItem::setVersionQt(const QString &version)
{
    QJsonObject targetObj = m_project["versions"].toObject();
    targetObj["qt"] = version;
    insertAndUpdateProjectFile("versions", targetObj);
}

QString QmlProjectItem::versionQtQuick() const
{
    return m_project["versions"].toObject()["qtQuick"].toString();
}

void QmlProjectItem::setVersionQtQuick(const QString &version)
{
    QJsonObject targetObj = m_project["versions"].toObject();
    targetObj["qtQuick"] = version;
    insertAndUpdateProjectFile("versions", targetObj);
}

QString QmlProjectItem::versionDesignStudio() const
{
    return m_project["versions"].toObject()["designStudio"].toString();
}

void QmlProjectItem::setVersionDesignStudio(const QString &version)
{
    QJsonObject targetObj = m_project["versions"].toObject();
    targetObj["designStudio"] = version;
    insertAndUpdateProjectFile("versions", targetObj);
}

QStringList QmlProjectItem::importPaths() const
{
    return m_project["importPaths"].toVariant().toStringList();
}

void QmlProjectItem::setImportPaths(const QStringList &importPaths)
{
    insertAndUpdateProjectFile("importPaths", QJsonArray::fromStringList(importPaths));
}

void QmlProjectItem::addImportPath(const QString &importPath)
{
    QJsonArray importPaths = m_project["importPaths"].toArray();

    if (importPaths.contains(importPath))
        return;

    importPaths.append(importPath);
    insertAndUpdateProjectFile("importPaths", importPaths);
}

QStringList QmlProjectItem::fileSelectors() const
{
    return m_project["runConfig"].toObject()["fileSelectors"].toVariant().toStringList();
}

void QmlProjectItem::setFileSelectors(const QStringList &selectors)
{
    QJsonObject targetObj = m_project["runConfig"].toObject();
    targetObj["fileSelectors"] = QJsonArray::fromStringList(selectors);
    insertAndUpdateProjectFile("runConfig", targetObj);
}

void QmlProjectItem::addFileSelector(const QString &selector)
{
    QJsonObject targetObj = m_project["runConfig"].toObject();
    QJsonArray fileSelectors = targetObj["fileSelectors"].toArray();

    if (fileSelectors.contains(selector))
        return;

    fileSelectors.append(selector);
    targetObj["fileSelectors"] = fileSelectors;
    insertAndUpdateProjectFile("runConfig", targetObj);
}

bool QmlProjectItem::multilanguageSupport() const
{
    return m_project["language"].toObject()["multiLanguageSupport"].toBool();
}

void QmlProjectItem::setMultilanguageSupport(const bool &isEnabled)
{
    QJsonObject targetObj = m_project["language"].toObject();
    targetObj["multiLanguageSupport"] = isEnabled;
    insertAndUpdateProjectFile("language", targetObj);
}

QStringList QmlProjectItem::supportedLanguages() const
{
    return m_project["language"].toObject()["supportedLanguages"].toVariant().toStringList();
}

void QmlProjectItem::setSupportedLanguages(const QStringList &languages)
{
    QJsonObject targetObj = m_project["language"].toObject();
    targetObj["supportedLanguages"] = QJsonArray::fromStringList(languages);
    insertAndUpdateProjectFile("language", targetObj);
}

void QmlProjectItem::addSupportedLanguage(const QString &language)
{
    QJsonObject targetObj = m_project["language"].toObject();
    QJsonArray suppLangs = targetObj["supportedLanguages"].toArray();

    if (suppLangs.contains(language))
        return;

    suppLangs.append(language);
    targetObj["supportedLanguages"] = suppLangs;
    insertAndUpdateProjectFile("language", targetObj);
}

QString QmlProjectItem::primaryLanguage() const
{
    return m_project["language"].toObject()["primaryLanguage"].toString();
}

void QmlProjectItem::setPrimaryLanguage(const QString &language)
{
    QJsonObject targetObj = m_project["language"].toObject();
    targetObj["primaryLanguage"] = language;
    insertAndUpdateProjectFile("language", targetObj);
}

Utils::FilePaths QmlProjectItem::files() const
{
    QSet<QString> filesSet;
    for (const auto &fileFilter : m_content) {
        const QStringList fileList = fileFilter->files();
        for (const QString &file : fileList) {
            filesSet.insert(file);
        }
    }

    Utils::FilePaths files;
    for (const auto &fileName : filesSet) {
        files.append(Utils::FilePath::fromString(fileName));
    }

    return files;
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

bool QmlProjectItem::forceFreeType() const
{
    return m_project["runConfig"].toObject()["forceFreeType"].toBool();
}

void QmlProjectItem::setForceFreeType(const bool &isForced)
{
    QJsonObject runConfig = m_project["runConfig"].toObject();
    runConfig["forceFreeType"] = isForced;
    insertAndUpdateProjectFile("runConfig", runConfig);
}

void QmlProjectItem::setMainFile(const QString &mainFile)
{
    QJsonObject runConfig = m_project["runConfig"].toObject();
    runConfig["mainFile"] = mainFile;
    insertAndUpdateProjectFile("runConfig", runConfig);
}

QString QmlProjectItem::mainFile() const
{
    return m_project["runConfig"].toObject()["mainFile"].toString();
}

void QmlProjectItem::setMainUiFile(const QString &mainUiFile)
{
    QJsonObject runConfig = m_project["runConfig"].toObject();
    runConfig["mainUiFile"] = mainUiFile;
    insertAndUpdateProjectFile("runConfig", runConfig);
}

QString QmlProjectItem::mainUiFile() const
{
    return m_project["runConfig"].toObject()["mainUiFile"].toString();
}

bool QmlProjectItem::widgetApp() const
{
    return m_project["runConfig"].toObject()["widgetApp"].toBool();
}

void QmlProjectItem::setWidgetApp(const bool &widgetApp)
{
    QJsonObject runConfig = m_project["runConfig"].toObject();
    runConfig["widgetApp"] = widgetApp;
    insertAndUpdateProjectFile("runConfig", runConfig);
}

QStringList QmlProjectItem::shaderToolArgs() const
{
    return m_project["shaderTool"].toObject()["args"].toVariant().toStringList();
}

void QmlProjectItem::setShaderToolArgs(const QStringList &args)
{
    QJsonObject shaderTool = m_project["shaderTool"].toObject();
    shaderTool["args"] = QJsonArray::fromStringList(args);
    insertAndUpdateProjectFile("shaderTool", shaderTool);
}

void QmlProjectItem::addShaderToolArg(const QString &arg)
{
    QJsonObject targetObj = m_project["shaderTool"].toObject();
    QJsonArray toolArgs = targetObj["args"].toArray();

    if (toolArgs.contains(arg))
        return;

    toolArgs.append(arg);
    targetObj["args"] = toolArgs;
    insertAndUpdateProjectFile("shaderTool", targetObj);
}

QStringList QmlProjectItem::shaderToolFiles() const
{
    return m_project.value("shaderTool").toObject().value("files").toVariant().toStringList();
}

void QmlProjectItem::setShaderToolFiles(const QStringList &files)
{
    QJsonObject shaderTool = m_project["shaderTool"].toObject();
    shaderTool["files"] = QJsonArray::fromStringList(files);
    insertAndUpdateProjectFile("shaderTool", shaderTool);
}

void QmlProjectItem::addShaderToolFile(const QString &file)
{
    QJsonObject targetObj = m_project["shaderTool"].toObject();
    QJsonArray toolArgs = targetObj["files"].toArray();

    if (toolArgs.contains(file))
        return;

    toolArgs.append(file);
    targetObj["files"] = toolArgs;
    insertAndUpdateProjectFile("shaderTool", targetObj);
}

void QmlProjectItem::insertAndUpdateProjectFile(const QString &key, const QJsonValue &value)
{
    m_project[key] = value;
    if (!m_skipRewrite)
        m_projectFile.writeFileContents(Converters::jsonToQmlProject(m_project).toUtf8());
}

} // namespace QmlProjectManager
