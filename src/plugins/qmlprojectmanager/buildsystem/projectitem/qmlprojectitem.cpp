// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprojectitem.h"

#include <QDir>
#include <QJsonDocument>
#include <QLoggingCategory>

#include "converters.h"

#include "../../qmlproject.h"
#include "../../qmlprojectconstants.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <qmljs/qmljssimplereader.h>

#include <QJsonArray>

namespace QmlProjectManager {

//#define REWRITE_PROJECT_FILE_IN_JSON_FORMAT

Q_LOGGING_CATEGORY(log, "QmlProjectManager.QmlProjectItem", QtCriticalMsg)

QmlProjectItem::QmlProjectItem(const Utils::FilePath &filePath, const bool skipRewrite)
    : m_projectFile(filePath)
    , m_skipRewrite(skipRewrite)
{
    if (initProjectObject())
        setupFileFilters();
}

bool QmlProjectItem::initProjectObject()
{
    if (m_projectFile.endsWith(Constants::fakeProjectName)) {
        auto uiFile = m_projectFile.toUrlishString();
        uiFile.remove(Constants::fakeProjectName);

        auto parentDir = Utils::FilePath::fromString(uiFile).parentDir();
        m_projectFile = parentDir.pathAppended(Constants::fakeProjectName);
        m_project = Converters::qmlProjectTojson({});

        return true;
    }

    auto contents = m_projectFile.fileContents();
    if (!contents) {
        qCWarning(log) << "Cannot open project file. Path:" << m_projectFile.fileName();
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
            qCWarning(log) << "Cannot parse the json formatted project file. Error:"
                           << parseError.errorString();
        } else {
            qCWarning(log) << "Cannot convert QmlProject to Json.";
        }
        return false;
    }

    m_project = rootObj;
    return true;
}

void QmlProjectItem::setupFileFilters()
{
    auto setupFileFilterItem = [this](const QJsonObject &fileGroup) {
        // first we need to add all directories as a 'resource' path for the project and set them as
        // recursive.
        // if there're any files with the explicit absolute paths, we need to add them afterwards.
        for (const QString &directory : fileGroup["directory"].toVariant().toStringList()) {
            std::unique_ptr<FileFilterItem> fileFilterItem{new FileFilterItem};
            fileFilterItem->setDirectory(directory);
            fileFilterItem->setFilters(fileGroup["filters"].toVariant().toStringList());
            fileFilterItem->setRecursive(true);

            fileFilterItem->setDefaultDirectory(m_projectFile.parentDir().toFSPathString());
#ifndef TESTS_ENABLED_QMLPROJECTITEM
            connect(fileFilterItem.get(),
                    &FileFilterItem::filesChanged,
                    this,
                    &QmlProjectItem::filesChanged);

            connect(fileFilterItem.get(),
                    &FileFilterItem::fileModified,
                    this,
                    &QmlProjectItem::fileModified);
#endif
            m_content.push_back(std::move(fileFilterItem));
        };

        // here we begin to add files with the explicit absolute paths
        QJsonArray files = fileGroup["files"].toArray();
        if (files.isEmpty())
            return;

        QStringList filesArr;
        std::transform(files.begin(),
                       files.end(),
                       std::back_inserter(filesArr),
                       [](const QJsonValue &value) { return value.toString(); });

        const QString directory = fileGroup["directory"].toString() == ""
                                      ? m_projectFile.parentDir().toUrlishString()
                                      : fileGroup["directory"].toString();
        Utils::FilePath groupDir = Utils::FilePath::fromString(directory);
        std::unique_ptr<FileFilterItem> fileFilterItem{new FileFilterItem};
        fileFilterItem->setRecursive(false);
        fileFilterItem->setPathsProperty(filesArr);
        fileFilterItem->setDefaultDirectory(m_projectFile.parentDir().toUrlishString());
        fileFilterItem->setDirectory(groupDir.toUrlishString());
#ifndef TESTS_ENABLED_QMLPROJECTITEM
        connect(fileFilterItem.get(), &FileFilterItem::filesChanged, this, &QmlProjectItem::filesChanged);
        connect(fileFilterItem.get(), &FileFilterItem::fileModified, this, &QmlProjectItem::fileModified);
#endif
        m_content.push_back(std::move(fileFilterItem));
    };

    for (const QJsonValue &fileGroup : m_project["fileGroups"].toArray()) {
        setupFileFilterItem(fileGroup.toObject());
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
    return m_project["mcu"].toObject()["enabled"].toBool();
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

QStringList QmlProjectItem::mockImports() const
{
    return m_project["mockImports"].toVariant().toStringList();
}

void QmlProjectItem::setMockImports(const QStringList &paths)
{
    insertAndUpdateProjectFile("mockImports", QJsonArray::fromStringList(paths));
}

void QmlProjectItem::addImportPath(const QString &importPath)
{
    QJsonArray importPaths = m_project["importPaths"].toArray();

    if (importPaths.contains(importPath))
        return;

    importPaths.append(importPath);
    insertAndUpdateProjectFile("importPaths", importPaths);
}

QStringList QmlProjectItem::qmlProjectModules() const
{
    return m_project["qmlprojectDependencies"].toVariant().toStringList();
}

void QmlProjectItem::setQmlProjectModules(const QStringList &paths)
{
    if (qmlProjectModules() == paths)
        return;

    auto jsonArray = QJsonArray::fromStringList(paths);
    updateFileGroup("Module", "files", jsonArray);
    insertAndUpdateProjectFile("qmlprojectDependencies", jsonArray);
}

void QmlProjectItem::addQmlProjectModule(const QString &modulePath)
{
    QJsonArray qmlModules = m_project["qmlprojectDependencies"].toArray();

    if (qmlModules.contains(modulePath))
        return;

    qmlModules.append(modulePath);
    updateFileGroup("Module", "files", qmlModules);
    insertAndUpdateProjectFile("qmlprojectDependencies", qmlModules);
}

void QmlProjectItem::addFileFilter(const Utils::FilePath &path)
{
    QJsonArray filters = m_project["fileGroups"].toArray();
    auto pathString = path.path();
    auto iter = std::ranges::find_if(filters, [pathString](const QJsonValue &elem) {
        return elem["directory"].toString() == pathString;
    });

    if (iter == filters.end()) {
        QJsonObject newFilter;
        newFilter["directory"] = pathString;
        newFilter["type"] = "Qml";
        filters.prepend(newFilter);
        insertAndUpdateProjectFile("fileGroups", filters);
    }
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

QStringList QmlProjectItem::effectComposerQsbArgs() const
{
    return m_project["effectComposer"].toObject()["qsbArgs"].toVariant().toStringList();
}

void QmlProjectItem::setEffectComposerQsbArgs(const QStringList &args)
{
    QJsonObject effectComposer = m_project["effectComposer"].toObject();
    effectComposer["qsbArgs"] = QJsonArray::fromStringList(args);
    insertAndUpdateProjectFile("effectComposer", effectComposer);
}

void QmlProjectItem::addEffectComposerQsbArgs(const QString &arg)
{
    QJsonObject targetObj = m_project["effectComposer"].toObject();
    QJsonArray toolArgs = targetObj["qsbArgs"].toArray();

    toolArgs.append(arg);
    targetObj["qsbArgs"] = toolArgs;
    insertAndUpdateProjectFile("effectComposer", targetObj);
}

void QmlProjectItem::insertAndUpdateProjectFile(const QString &key, const QJsonValue &value)
{
    m_project[key] = value;

    if (!m_skipRewrite)
        m_projectFile.writeFileContents(Converters::jsonToQmlProject(m_project).toUtf8());
}

void QmlProjectItem::updateFileGroup(const QString &groupType,
                                     const QString &property,
                                     const QJsonValue &value)
{
    auto arr = m_project["fileGroups"].toArray();
    auto found = std::find_if(arr.begin(), arr.end(), [groupType](const QJsonValue &elem) {
        return elem["type"].toString() == groupType;
    });
    if (found == arr.end()) {
        qCWarning(log) << "fileGroups - unable to find group:" << groupType;
        return;
    }

    auto obj = found->toObject();
    obj[property] = value;

    arr.removeAt(std::distance(arr.begin(), found));
    arr.append(obj);
    m_project["fileGroups"] = arr;

    m_content.clear();
    setupFileFilters();
}

bool QmlProjectItem::enableCMakeGeneration() const
{
    return m_project["deployment"].toObject()["enableCMakeGeneration"].toBool();
}

void QmlProjectItem::setEnableCMakeGeneration(bool enable)
{
    QJsonObject obj = m_project["deployment"].toObject();
    obj["enableCMakeGeneration"] = enable;
    insertAndUpdateProjectFile("deployment", obj);
}

bool QmlProjectItem::enablePythonGeneration() const
{
    return m_project["deployment"].toObject()["enablePythonGeneration"].toBool();
}

void QmlProjectItem::setEnablePythonGeneration(bool enable)
{
    QJsonObject obj = m_project["deployment"].toObject();
    obj["enablePythonGeneration"] = enable;
    insertAndUpdateProjectFile("deployment", obj);
}

bool QmlProjectItem::standaloneApp() const
{
    return m_project["deployment"].toObject()["standaloneApp"].toBool(true);
}

void QmlProjectItem::setStandaloneApp(bool value)
{
    QJsonObject obj = m_project["deployment"].toObject();
    obj["standaloneApp"] = value;
    insertAndUpdateProjectFile("deployment", obj);
}

} // namespace QmlProjectManager
