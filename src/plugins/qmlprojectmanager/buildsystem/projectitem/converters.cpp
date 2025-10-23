// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "converters.h"
#include "utils/algorithm.h"
#include "../../qmlprojectexporter/filetypes.h"
#include "qmlprojectconstants.h"

#include <QJsonDocument>

namespace QmlProjectManager::Converters {

const static QStringList qmlFilesFilter{QStringLiteral("*.qml")};
const static QStringList javaScriptFilesFilter{QStringLiteral("*.js"), QStringLiteral("*.ts")};

const QStringList imageFilesFilter() {
    return imageFiles([](const QString& suffix) { return "*." + suffix; });
}

QString jsonValueToString(const QJsonValue &val, int indentationLevel, bool indented);

QString jsonToQmlProject(const QJsonObject &rootObject)
{
    QString qmlProjectString;
    QTextStream ts{&qmlProjectString};

    QJsonObject runConfig = rootObject["runConfig"].toObject();
    QJsonObject languageConfig = rootObject["language"].toObject();
    QJsonObject shaderConfig = rootObject["shaderTool"].toObject();
    QJsonObject effectComposerConfig = rootObject["effectComposer"].toObject();
    QJsonObject versionConfig = rootObject["versions"].toObject();
    QJsonObject environmentConfig = rootObject["environment"].toObject();
    QJsonObject deploymentConfig = rootObject["deployment"].toObject();
    const QJsonArray filesConfig = rootObject["fileGroups"].toArray();
    QJsonObject otherProperties = rootObject["otherProperties"].toObject();

    QJsonObject mcuObject = rootObject["mcu"].toObject();
    QJsonObject mcuConfig = mcuObject["config"].toObject();
    QJsonObject mcuModule = mcuObject["module"].toObject();

    int indentationLevel = 0;

    auto appendBreak = [&ts]() { ts << Qt::endl; };

    auto appendComment = [&ts, &indentationLevel](const QString &comment) {
        ts << QString(" ").repeated(indentationLevel * 4) << "// " << comment << Qt::endl;
    };

    auto appendItem =
        [&ts, &indentationLevel](const QString &key, const QString &value, const bool isEnclosed) {
            ts << QString(" ").repeated(indentationLevel * 4) << key << ": "
               << (isEnclosed ? "\"" : "") << value << (isEnclosed ? "\"" : "") << Qt::endl;
        };

    auto appendString = [&appendItem](const QString &key, const QString &val) {
        if (val.isEmpty())
            return;
        appendItem(key, val, true);
    };

    auto appendBool = [&appendItem](const QString &key, const bool &val) {
        appendItem(key, QString::fromStdString(val ? "true" : "false"), false);
    };

    auto appendBoolOpt = [&appendBool](const QString &key, const QJsonObject &source) {
        if (!source.keys().contains(key)) {
            return;
        }
        appendBool(key, source[key].toBool());
    };

    auto appendStringArray = [&appendItem](const QString &key, const QStringList &vals) {
        if (vals.isEmpty())
            return;
        QString finalString;
        for (const QString &value : vals)
            finalString.append("\"").append(value).append("\"").append(",");

        finalString.remove(finalString.length() - 1, 1);
        finalString.prepend("[ ").append(" ]");
        appendItem(key, finalString, false);
    };

    auto appendJsonArray = [&appendItem, &indentationLevel](const QString &key,
                                                            const QJsonArray &vals) {
        if (vals.isEmpty())
            return;
        appendItem(key, jsonValueToString(vals, indentationLevel, /*indented*/ true), false);
    };

    auto appendProperties = [&appendItem, &indentationLevel](const QJsonObject &props,
                                                             const QString &prefix) {
        for (const auto &key : props.keys()) {
            QJsonValue val = props[key];
            QString keyWithPrefix = key;
            if (!prefix.isEmpty()) {
                keyWithPrefix.prepend(prefix + ".");
            }
            appendItem(keyWithPrefix,
                       jsonValueToString(val, indentationLevel, /*indented*/ false),
                       false);
        }
    };

    auto startObject = [&ts, &indentationLevel](const QString &objectName) {
        ts << Qt::endl
           << QString(" ").repeated(indentationLevel * 4) << objectName << " {" << Qt::endl;
        indentationLevel++;
    };

    auto endObject = [&ts, &indentationLevel]() {
        indentationLevel--;
        ts << QString(" ").repeated(indentationLevel * 4) << "}" << Qt::endl;
    };

    auto appendFileGroup = [&startObject,
                            &endObject,
                            &appendString,
                            &appendProperties,
                            &appendJsonArray](const QJsonObject &fileGroup,
                                              const QString &nodeName) {
        startObject(nodeName);
        appendString("directory", fileGroup["directory"].toString());
        QStringList filters = fileGroup["filters"].toVariant().toStringList();
        QStringList filter = {};
        if (nodeName.toLower() == "qmlfiles") {
            filter = qmlFilesFilter;
        } else if (nodeName.toLower() == "imagefiles") {
            filter = imageFilesFilter();
        } else if (nodeName.toLower() == "javascriptfiles") {
            filter = javaScriptFilesFilter;
        }
        for (const QString &entry : std::as_const(filter)) {
            filters.removeOne(entry);
        }
        appendString("filter", filters.join(";"));
        appendJsonArray("files", fileGroup["files"].toArray());
        appendProperties(fileGroup["mcuProperties"].toObject(), "MCU");
        appendProperties(fileGroup["otherProperties"].toObject(), "");
        endObject();
    };

    // start creating the file content
    appendComment("prop: json-converted");
    appendComment("prop: auto-generated");

    ts << Qt::endl << "import QmlProject" << Qt::endl;
    {
        startObject("Project");

        // append non-object props
        appendString("mainFile", runConfig["mainFile"].toString());
        appendString("mainUiFile", runConfig["mainUiFile"].toString());
        appendString("targetDirectory", deploymentConfig["targetDirectory"].toString());
        appendBoolOpt("enableCMakeGeneration", deploymentConfig);
        appendBoolOpt("enablePythonGeneration", deploymentConfig);
        appendBoolOpt("standaloneApp", deploymentConfig);
        appendBool("widgetApp", runConfig["widgetApp"].toBool());
        appendStringArray("importPaths", rootObject["importPaths"].toVariant().toStringList());
        appendStringArray("mockImports", rootObject["mockImports"].toVariant().toStringList());
        appendBreak();
        appendString("qdsVersion", versionConfig["designStudio"].toString());
        appendString("quickVersion", versionConfig["qtQuick"].toString());
        appendBool("qt6Project", versionConfig["qt"].toString() == "6");
        appendBool("qtForMCUs",
                   mcuObject["enabled"].toBool() || !mcuConfig.isEmpty() || !mcuModule.isEmpty());
        if (!languageConfig.isEmpty()) {
            appendBreak();
            appendBool("multilanguageSupport", languageConfig["multiLanguageSupport"].toBool());
            appendString("primaryLanguage", languageConfig["primaryLanguage"].toString());
            appendStringArray("supportedLanguages",
                              languageConfig["supportedLanguages"].toVariant().toStringList());
        }

        // Since different versions of Qt for MCUs may introduce new properties, we collect all
        // unknown properties in a separate object.
        // We need to avoid losing content regardless of which QDS/QUL version combo is used.
        if (!otherProperties.isEmpty()) {
            appendBreak();
            appendProperties(otherProperties, "");
        }

        // append Environment object
        if (!environmentConfig.isEmpty()) {
            startObject("Environment");
            for (const QString &key : environmentConfig.keys()) {
                appendItem(key, environmentConfig[key].toString(), true);
            }
            endObject();
        }

        // append ShaderTool object
        if (!shaderConfig["args"].toVariant().toStringList().isEmpty()) {
            startObject("ShaderTool");
            appendString("args",
                         shaderConfig["args"].toVariant().toStringList().join(" ").replace("\"",
                                                                                           "\\\""));
            appendStringArray("files", shaderConfig["files"].toVariant().toStringList());
            endObject();
        }

        // append EffectComposer object
        if (!effectComposerConfig["qsbArgs"].toVariant().toStringList().isEmpty()) {
            startObject("EffectComposer");
            appendString("qsbArgs",
                         effectComposerConfig["qsbArgs"].toVariant().toStringList().join(" ")
                             .replace("\"", "\\\""));
            endObject();
        }

        // append the MCU.Config object
        if (!mcuConfig.isEmpty()) {
            // Append MCU.Config
            startObject("MCU.Config");
            appendProperties(mcuConfig, "");
            endObject();
        }

        // Append the MCU.Module object
        if (!mcuModule.isEmpty()) {
            // Append MCU.Module
            startObject("MCU.Module");
            appendProperties(mcuModule, "");
            endObject();
        }

        // append files objects
        for (const QJsonValue &fileGroup : filesConfig) {
            QString nodeType = QString("%1Files").arg(fileGroup["type"].toString());
            if (fileGroup["type"].toString().isEmpty()
                && fileGroup["filters"].toArray().contains("*.qml")) {
                // TODO: IS this important? It turns Files node with *.qml in the filters into QmlFiles nodes
                nodeType = "QmlFiles";
            }
            appendFileGroup(fileGroup.toObject(), nodeType);
        }

        endObject(); // Closing 'Project'
    }
    return qmlProjectString;
}

QStringList qmlprojectsFromFilesNodes(const QJsonArray &fileGroups,
                                      const Utils::FilePath &projectRootPath)
{
    QStringList qmlProjectFiles;
    for (const QJsonValue &fileGroup : fileGroups) {
        if (fileGroup["type"].toString() != "Module") {
            continue;
        }
        // In Qul, paths are relative to the project root directory, not the "directory" entry
        qmlProjectFiles.append(fileGroup["files"].toVariant().toStringList());

        // If the "directory" property is set, all qmlproject files in the directory are also added
        // as relative paths from the project root directory, in addition to explicitly added files
        const QString directoryProp = fileGroup["directory"].toString("");
        if (directoryProp.isEmpty()) {
            continue;
        }
        const Utils::FilePath dir = projectRootPath / directoryProp;
        qmlProjectFiles.append(Utils::transform<QStringList>(
            dir.dirEntries(Utils::FileFilter({"*.qmlproject"}, QDir::Files)),
            [&projectRootPath](Utils::FilePath file) {
                return file.absoluteFilePath().relativePathFromDir(projectRootPath).toFSPathString();
            }));
    }

    return qmlProjectFiles;
}

QString moduleUriFromQmlProject(const QString &qmlProjectFilePath)
{
    QmlJS::SimpleReader simpleReader;
    const auto rootNode = simpleReader.readFile(qmlProjectFilePath);
    // Since the file wasn't explicitly added, skip qmlproject files with errors
    if (!rootNode || !simpleReader.errors().isEmpty()) {
        return QString();
    }

    for (const auto &child : rootNode->children()) {
        if (child->name() == "MCU.Module") {
            const auto prop = child->property("uri").isValid() ? child->property("uri")
                                                               : child->property("MCU.uri");
            if (prop.isValid()) {
                return prop.value.toString();
            }
            break;
        }
    }

    return QString();
}

QJsonObject nodeToJsonObject(const QmlJS::SimpleReaderNode::Ptr &node)
{
    QJsonObject tObj;
    for (const QString &childPropName : node->propertyNames())
        tObj.insert(childPropName, node->property(childPropName).value.toJsonValue());

    for (const auto &childNode : node->children())
        tObj.insert(childNode->name(), nodeToJsonObject(childNode));

    return tObj;
};

// Returns a list of qmlproject files in currentSearchPath which are valid modules,
// with URIs matching the relative path from importPathBase.
QStringList getModuleQmlProjectFiles(const Utils::FilePath &importPath,
                                     const Utils::FilePath &projectRootPath)
{
    QStringList qmlProjectFiles;

    QDirIterator it(importPath.toFSPathString(),
                    QDir::NoDotAndDotDot | QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        if (!file.endsWith(".qmlproject")) {
            continue;
        }

        // Add if matching
        QString uri = moduleUriFromQmlProject(file);
        if (uri.isEmpty()) {
            // If the qmlproject file is not a valid module, skip it
            continue;
        }

        const auto filePath = Utils::FilePath::fromUserInput(file);
        const bool isBaseImportPath = filePath.parentDir() == importPath;

        // Check the URI against the original import path before adding
        // If we look directly in the search path, the URI doesn't matter
        const QString relativePath = filePath.parentDir().relativePathFromDir(importPath).path();
        if (isBaseImportPath || uri.replace(".", "/") == relativePath) {
            // If the URI matches the path or the file is directly in the import path, add it
            qmlProjectFiles.emplace_back(filePath.relativePathFromDir(projectRootPath).toFSPathString());
        }
    }
    return qmlProjectFiles;
}

QStringList qmlprojectsFromImportPaths(const QStringList &importPaths,
                                       const Utils::FilePath &projectRootPath)
{
    return Utils::transform<QStringList>(importPaths, [&projectRootPath](const QString &importPath) {
        const auto importDir = projectRootPath / importPath;
        return getModuleQmlProjectFiles(importDir, projectRootPath);
    });
}

QJsonObject qmlProjectTojson(const Utils::FilePath &projectFile)
{
    QmlJS::SimpleReader simpleQmlJSReader;

    QmlJS::SimpleReaderNode::Ptr rootNode;

    if (!projectFile.isEmpty()) {
        rootNode = simpleQmlJSReader.readFile(projectFile.toFSPathString());
    } else {
        rootNode = simpleQmlJSReader.readFromSource("import QmlProject 1.1\n"

                                                    "Project {\n"
                                                    "QmlFiles {\n"
                                                    "directory: \".\"\n"
                                                    "}\n"
                                                    "qt6Project: true\n"
                                                    "}\n");
    }

    if (!simpleQmlJSReader.errors().isEmpty() || !rootNode->isValid()) {
        qCritical() << "Unable to parse:" << projectFile;
        qCritical() << simpleQmlJSReader.errors();
        return {};
    }

    if (rootNode->name() != QLatin1String("Project")) {
        qCritical() << "Cannot find root 'Project' item in the project file: " << projectFile;
        return {};
    }

    auto toCamelCase = [](const QString &s) { return QString(s).replace(0, 1, s[0].toLower()); };

    QJsonObject rootObject; // root object
    QJsonArray fileGroupsObject;
    QJsonObject languageObject;
    QJsonObject versionObject;
    QJsonObject runConfigObject;
    QJsonObject deploymentObject;
    QJsonObject mcuObject;
    QJsonObject mcuConfigObject;
    QJsonObject mcuModuleObject;
    QJsonObject shaderToolObject;
    QJsonObject effectComposerObject;
    QJsonObject otherProperties;

    bool qtForMCUs = false;

    QStringList importPaths;
    Utils::FilePath projectRootPath = projectFile.parentDir();

    // convert the non-object props
    for (const QString &propName : rootNode->propertyNames()) {
        QJsonObject *currentObj = &rootObject;
        QString objKey = QString(propName).remove("QDS.", Qt::CaseInsensitive);
        QJsonValue value = rootNode->property(propName).value.toJsonValue();

        if (propName.contains("language", Qt::CaseInsensitive)) {
            currentObj = &languageObject;
            if (propName.contains("multilanguagesupport", Qt::CaseInsensitive))
                // fixing the camelcase
                objKey = "multiLanguageSupport";
        } else if (propName.contains("version", Qt::CaseInsensitive)) {
            currentObj = &versionObject;
            if (propName.contains("qdsversion", Qt::CaseInsensitive))
                objKey = "designStudio";
            else if (propName.contains("quickversion", Qt::CaseInsensitive))
                objKey = "qtQuick";
        } else if (propName.contains("widgetapp", Qt::CaseInsensitive)
                   || propName.contains("fileselector", Qt::CaseInsensitive)
                   || propName.contains("mainfile", Qt::CaseInsensitive)
                   || propName.contains("mainuifile", Qt::CaseInsensitive)
                   || propName.contains("forcefreetype", Qt::CaseInsensitive)) {
            currentObj = &runConfigObject;
        } else if (propName.contains("targetdirectory", Qt::CaseInsensitive)
                   || propName.contains("enableCMakeGeneration", Qt::CaseInsensitive)
                   || propName.contains("enablePythonGeneration", Qt::CaseInsensitive)
                   || propName.contains("standaloneApp", Qt::CaseInsensitive)) {
            currentObj = &deploymentObject;
        } else if (propName.contains("qtformcus", Qt::CaseInsensitive)) {
            qtForMCUs = value.toBool();
            continue;
        } else if (propName.contains("qt6project", Qt::CaseInsensitive)) {
            currentObj = &versionObject;
            objKey = "qt";
            value = rootNode->property(propName).value.toBool() ? "6" : "5";
        } else if (propName.contains("importpaths", Qt::CaseInsensitive)) {
            objKey = "importPaths";
            importPaths = value.toVariant().toStringList();
        } else if (propName.contains("mockImports", Qt::CaseInsensitive)) {
            objKey = "mockImports";
        } else {
            currentObj = &otherProperties;
            objKey = propName; // With prefix
        }

        currentObj->insert(objKey, value);
    }

    // add missing non-object props if any
    if (!runConfigObject.contains("fileSelectors")) {
        runConfigObject.insert("fileSelectors", QJsonArray{});
    }

    if (!versionObject.contains("qt")) {
        versionObject.insert("qt", "5");
    }

    rootObject.insert("otherProperties", otherProperties);

    auto quotedArgs = [](const QmlJS::SimpleReaderNode::Property &prop) -> QJsonArray {
        const QStringList quotedArgs = prop.value.toString().split('\"', Qt::SkipEmptyParts);
        QStringList args;
        for (int i = 0; i < quotedArgs.size(); ++i) {
            // Each odd arg in this list is a single quoted argument, which we should
            // not split further
            if (i % 2 == 0)
                args.append(quotedArgs[i].trimmed().split(' '));
            else
                args.append(QString(quotedArgs[i]).prepend("\"").append("\""));
        }
        return QJsonArray::fromStringList(args);
    };

    // convert the object props
    for (const QmlJS::SimpleReaderNode::Ptr &childNode : rootNode->children()) {
        if (childNode->name().contains("files", Qt::CaseInsensitive)) {
            QString childNodeName = childNode->name().remove("qds.", Qt::CaseInsensitive);
            qsizetype filesPos = childNodeName.indexOf("files", 0, Qt::CaseInsensitive);
            const QString childNodeType = childNodeName.first(filesPos);
            childNodeName = childNodeName.toLower();

            QJsonArray childNodeFiles = childNode->property("files").value.toJsonArray();
            QString childNodeDirectory = childNode->property("directory").value.toString();
            QStringList filters
                = childNode->property("filter").value.toString().split(";", Qt::SkipEmptyParts);
            QJsonArray childNodeFilters = QJsonArray::fromStringList(filters);

            // files have priority over filters
            // if explicit files are given, then filters will be ignored
            // and all files are prefixed such as "directory/<filename>".
            // if directory is empty, then the files are prefixed with the project directory
            if (childNodeFiles.empty()) {
                auto inserter = [&childNodeFilters](auto &filterSource) {
                    if (!childNodeFilters.empty())
                        return;

                    std::for_each(std::cbegin(filterSource),
                                  std::cend(filterSource),
                                  [&childNodeFilters](const auto &value) {
                                      if (!childNodeFilters.contains(value)) {
                                          childNodeFilters << value;
                                      }
                                  });
                };

                // Those 4 file groups are the special ones
                // that have a default set of filters.
                // The default filters are written to the
                // qmlproject file after conversion
                if (childNodeName == "qmlfiles") {
                    inserter(qmlFilesFilter);
                } else if (childNodeName == "javascriptfiles") {
                    inserter(javaScriptFilesFilter);
                } else if (childNodeName == "imagefiles") {
                    inserter(imageFilesFilter());
                } else if (childNodeName == "fontfiles") {
                    inserter(QmlProjectManager::Constants::QDS_FONT_FILES_FILTERS);
                }
            }

            // create the file group object
            QJsonObject targetObject;
            targetObject.insert("directory", childNodeDirectory);
            targetObject.insert("filters", childNodeFilters);
            targetObject.insert("files", childNodeFiles);
            targetObject.insert("type", childNodeType);

            QJsonObject mcuPropertiesObject;
            QJsonObject otherPropertiesObject;
            for (const auto &propName : childNode->propertyNames()) {
                if (propName == "directory" || propName == "filter" || propName == "files") {
                    continue;
                }

                auto val = QJsonValue::fromVariant(childNode->property(propName).value);

                if (propName.startsWith("MCU.", Qt::CaseInsensitive)) {
                    mcuPropertiesObject.insert(propName.mid(4), val);
                } else {
                    otherPropertiesObject.insert(propName, val);
                }
            }

            targetObject.insert("mcuProperties", mcuPropertiesObject);
            targetObject.insert("otherProperties", otherPropertiesObject);

            fileGroupsObject.append(targetObject);
        } else if (childNode->name().contains("shadertool", Qt::CaseInsensitive)) {
            shaderToolObject.insert("args", quotedArgs(childNode->property("args")));
            shaderToolObject.insert("files", childNode->property("files").value.toJsonValue());
        } else if (childNode->name().contains("effectcomposer", Qt::CaseInsensitive)) {
            effectComposerObject.insert("qsbArgs", quotedArgs(childNode->property("qsbArgs")));
        } else if (childNode->name().contains("config", Qt::CaseInsensitive)) {
            mcuConfigObject = nodeToJsonObject(childNode);
            if (const auto fileSelector = childNode->property("fileSelector"); fileSelector.isValid()) {
                auto currentSelectors = runConfigObject.value("fileSelectors").toArray();
                const auto mcuSelectors = fileSelector.value.toJsonArray();
                for (const auto &elem : mcuSelectors) {
                    if (!currentSelectors.contains(elem)) {
                        currentSelectors.append(elem);
                    }
                }
                runConfigObject.insert("fileSelectors", currentSelectors);
            }
        } else if (childNode->name().contains("module", Qt::CaseInsensitive)) {
            mcuModuleObject = nodeToJsonObject(childNode);
        } else {
            rootObject.insert(toCamelCase(childNode->name().remove("qds.", Qt::CaseInsensitive)),
                              nodeToJsonObject(childNode));
        }
    }

    QStringList qmlProjectDependencies;
    qmlProjectDependencies.append(qmlprojectsFromImportPaths(importPaths, projectRootPath));
    qmlProjectDependencies.append(qmlprojectsFromFilesNodes(fileGroupsObject, projectRootPath));
    qmlProjectDependencies.removeDuplicates();
    qmlProjectDependencies.sort();
    rootObject.insert("qmlprojectDependencies", QJsonArray::fromStringList(qmlProjectDependencies));

    mcuObject.insert("config", mcuConfigObject);
    mcuObject.insert("module", mcuModuleObject);
    qtForMCUs |= !(mcuModuleObject.isEmpty() && mcuConfigObject.isEmpty());
    mcuObject.insert("enabled", qtForMCUs);

    rootObject.insert("fileGroups", fileGroupsObject);
    rootObject.insert("language", languageObject);
    rootObject.insert("versions", versionObject);
    rootObject.insert("runConfig", runConfigObject);
    rootObject.insert("deployment", deploymentObject);
    rootObject.insert("mcu", mcuObject);
    if (!shaderToolObject.isEmpty())
        rootObject.insert("shaderTool", shaderToolObject);
    if (!effectComposerObject.isEmpty())
        rootObject.insert("effectComposer", effectComposerObject);
    rootObject.insert("fileVersion", 1);
    return rootObject;
}

QString jsonValueToString(const QJsonValue &val, int indentationLevel, bool indented)
{
    if (val.isArray()) {
        auto jsonFormat = indented ? QJsonDocument::JsonFormat::Indented
                                   : QJsonDocument::JsonFormat::Compact;
        QString str = QString::fromUtf8((QJsonDocument(val.toArray()).toJson(jsonFormat)));
        if (indented) {
            // Strip trailing newline
            str.chop(1);
        }
        return str.replace("\n", QString(" ").repeated(indentationLevel * 4).prepend("\n"));
    } else if (val.isBool()) {
        return val.toBool() ? QString("true") : QString("false");
    } else if (val.isDouble()) {
        return QString("%1").arg(val.toDouble());
    } else if (val.isObject()) {
        QString nodeContent = "{\n";
        QJsonObject obj = val.toObject();
        for (QString key : obj.keys()) {
            QJsonValue val = obj[key];
            nodeContent += key.append(": ").prepend(QString(" ").repeated((indentationLevel + 1) * 4));
            nodeContent += jsonValueToString(val, indentationLevel + 1, indented) + "\n";
        }
        return nodeContent + QString(" ").repeated(indentationLevel * 4).append("}");
    } else {
        return val.toString().prepend("\"").append("\"");
    }
}

} // namespace QmlProjectManager::Converters
