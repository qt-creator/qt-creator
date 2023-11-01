// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "converters.h"

#include <QJsonArray>

namespace QmlProjectManager::Converters {

QString jsonToQmlProject(const QJsonObject &rootObject)
{
    QString qmlProjectString;
    QTextStream ts{&qmlProjectString};

    QJsonObject runConfig = rootObject["runConfig"].toObject();
    QJsonObject languageConfig = rootObject["language"].toObject();
    QJsonObject shaderConfig = rootObject["shaderTool"].toObject();
    QJsonObject versionConfig = rootObject["versions"].toObject();
    QJsonObject environmentConfig = rootObject["environment"].toObject();
    QJsonObject deploymentConfig = rootObject["deployment"].toObject();
    QJsonArray filesConfig = rootObject["fileGroups"].toArray();

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
        appendItem(key, val, true);
    };

    auto appendBool = [&appendItem](const QString &key, const bool &val) {
        appendItem(key, QString::fromStdString(val ? "true" : "false"), false);
    };

    auto appendArray = [&appendItem](const QString &key, const QStringList &vals) {
        QString finalString;
        for (const QString &value : vals)
            finalString.append("\"").append(value).append("\"").append(",");

        finalString.remove(finalString.length() - 1, 1);
        finalString.prepend("[ ").append(" ]");
        appendItem(key, finalString, false);
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

    auto appendFileGroup =
        [&startObject, &endObject, &appendString, &appendArray](const QJsonObject &fileGroup,
                                                                const QString &qmlKey) {
            startObject(qmlKey);
            appendString("directory", fileGroup["directory"].toString());
            appendString("filter", fileGroup["filters"].toVariant().toStringList().join(";"));
            appendArray("files", fileGroup["files"].toVariant().toStringList());
            endObject();
        };

    auto appendQmlFileGroup =
        [&startObject, &endObject, &appendString](const QJsonObject &fileGroup) {
            startObject("QmlFiles");
            appendString("directory", fileGroup["directory"].toString());
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
        appendBool("widgetApp", runConfig["widgetApp"].toBool());
        appendArray("importPaths", rootObject["importPaths"].toVariant().toStringList());
        appendBreak();
        appendString("qdsVersion", versionConfig["designStudio"].toString());
        appendString("quickVersion", versionConfig["qtQuick"].toString());
        appendBool("qt6Project", versionConfig["qt"].toString() == "6");
        appendBool("qtForMCUs", !(rootObject["mcuConfig"].toObject().isEmpty()));
        appendBreak();
        appendBool("multilanguageSupport", languageConfig["multiLanguageSupport"].toBool());
        appendString("primaryLanguage", languageConfig["primaryLanguage"].toString());
        appendArray("supportedLanguages",
                    languageConfig["supportedLanguages"].toVariant().toStringList());

        // append Environment object
        startObject("Environment");
        for (const QString &key : environmentConfig.keys())
            appendItem(key, environmentConfig[key].toString(), true);

        endObject();

        // append ShaderTool object
        if (!shaderConfig["args"].toVariant().toStringList().isEmpty()) {
            startObject("ShaderTool");
            appendString("args",
                         shaderConfig["args"].toVariant().toStringList().join(" ").replace("\"",
                                                                                           "\\\""));
            appendArray("files", shaderConfig["files"].toVariant().toStringList());
            endObject();
        }

        // append files objects
        for (const QJsonValue &fileGroup : filesConfig) {
            if (fileGroup["filters"].toArray().contains("*.qml"))
                appendQmlFileGroup(fileGroup.toObject());
            else
                appendFileGroup(fileGroup.toObject(), "Files");
        }

        endObject(); // Closing 'Project'
    }
    return qmlProjectString;
}

QJsonObject qmlProjectTojson(const Utils::FilePath &projectFile)
{
    QmlJS::SimpleReader simpleQmlJSReader;

    const QmlJS::SimpleReaderNode::Ptr rootNode = simpleQmlJSReader.readFile(projectFile.toString());

    if (!simpleQmlJSReader.errors().isEmpty() || !rootNode->isValid()) {
        qCritical() << "Unable to parse:" << projectFile;
        qCritical() << simpleQmlJSReader.errors();
        return {};
    }

    if (rootNode->name() != QLatin1String("Project")) {
        qCritical() << "Cannot find root 'Project' item in the project file: " << projectFile;
        return {};
    }

    auto nodeToJsonObject = [](const QmlJS::SimpleReaderNode::Ptr &node) {
        QJsonObject tObj;
        for (const QString &childPropName : node->propertyNames())
            tObj.insert(childPropName, node->property(childPropName).value.toJsonValue());

        return tObj;
    };

    auto toCamelCase = [](const QString &s) { return QString(s).replace(0, 1, s[0].toLower()); };

    QJsonObject rootObject; // root object
    QJsonArray fileGroupsObject;
    QJsonObject languageObject;
    QJsonObject versionObject;
    QJsonObject runConfigObject;
    QJsonObject deploymentObject;
    QJsonObject mcuObject;
    QJsonObject shaderToolObject;

    // convert the the non-object props
    for (const QString &propName : rootNode->propertyNames()) {
        QJsonObject *currentObj = &rootObject;
        QString objKey = QString(propName).remove("QDS.", Qt::CaseInsensitive);
        QJsonValue value = rootNode->property(propName).value.toJsonValue();

        if (propName.startsWith("mcu.", Qt::CaseInsensitive)) {
            currentObj = &mcuObject;
            objKey = QString(propName).remove("MCU.");
        } else if (propName.contains("language", Qt::CaseInsensitive)) {
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
        } else if (propName.contains("targetdirectory", Qt::CaseInsensitive)) {
            currentObj = &deploymentObject;
        } else if (propName.contains("qtformcus", Qt::CaseInsensitive)) {
            currentObj = &mcuObject;
            objKey = "mcuEnabled";
        } else if (propName.contains("qt6project", Qt::CaseInsensitive)) {
            currentObj = &versionObject;
            objKey = "qt";
            value = rootNode->property(propName).value.toBool() ? "6" : "5";
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

    // convert the the object props
    for (const QmlJS::SimpleReaderNode::Ptr &childNode : rootNode->children()) {
        if (childNode->name().contains("files", Qt::CaseInsensitive)) {
            const QString childNodeName = childNode->name().toLower().remove("qds.");
            QJsonArray childNodeFiles = childNode->property("files").value.toJsonArray();
            QString childNodeDirectory = childNode->property("directory").value.toString();
            QStringList filters = childNode->property("filter").value.toString().split(";");
            filters.removeAll("");
            QJsonArray childNodeFilters = QJsonArray::fromStringList(filters);

            // files have priority over filters
            // if explicit files are given, then filters will be ignored
            // and all files are prefixed such as "directory/<filename>".
            // if directory is empty, then the files are prefixed with the project directory
            if (childNodeFiles.empty()) {
                auto inserter = [&childNodeFilters](const QStringList &filterSource) {
                    std::for_each(filterSource.begin(),
                                  filterSource.end(),
                                  [&childNodeFilters](const auto &value) {
                                      childNodeFilters << value;
                                  });
                };

                // Those 3 file groups are the special ones
                // that have a default set of filters. After the first
                // conversion (QmlProject -> JSON) they are converted to
                // the generic file group format ('Files' or 'QDS.Files')
                if (childNodeName == "qmlfiles") {
                    inserter({QStringLiteral("*.qml")});
                } else if (childNodeName == "javascriptfiles") {
                    inserter({QStringLiteral("*.js"), QStringLiteral("*.ts")});
                } else if (childNodeName == "imagefiles") {
                    inserter({QStringLiteral("*.jpeg"),
                              QStringLiteral("*.jpg"),
                              QStringLiteral("*.png"),
                              QStringLiteral("*.svg"),
                              QStringLiteral("*.hdr"),
                              QStringLiteral(".ktx")});
                }
            }

            // create the file group object
            QJsonObject targetObject;
            targetObject.insert("directory", childNodeDirectory);
            targetObject.insert("filters", childNodeFilters);
            targetObject.insert("files", childNodeFiles);

            fileGroupsObject.append(targetObject);
        } else if (childNode->name().contains("shadertool", Qt::CaseInsensitive)) {
            QStringList quotedArgs
                = childNode->property("args").value.toString().split('\"', Qt::SkipEmptyParts);
            QStringList args;
            for (int i = 0; i < quotedArgs.size(); ++i) {
                // Each odd arg in this list is a single quoted argument, which we should
                // not be split further
                if (i % 2 == 0)
                    args.append(quotedArgs[i].trimmed().split(' '));
                else
                    args.append(quotedArgs[i].prepend("\"").append("\""));
            }

            shaderToolObject.insert("args", QJsonArray::fromStringList(args));
            shaderToolObject.insert("files", childNode->property("files").value.toJsonValue());
        } else {
            rootObject.insert(toCamelCase(childNode->name().remove("qds.", Qt::CaseInsensitive)),
                              nodeToJsonObject(childNode));
        }
    }

    rootObject.insert("fileGroups", fileGroupsObject);
    rootObject.insert("language", languageObject);
    rootObject.insert("versions", versionObject);
    rootObject.insert("runConfig", runConfigObject);
    rootObject.insert("deployment", deploymentObject);
    rootObject.insert("mcuConfig", mcuObject);
    if (!shaderToolObject.isEmpty())
        rootObject.insert("shaderTool", shaderToolObject);
    rootObject.insert("fileVersion", 1);
    return rootObject;
}
} // namespace QmlProjectManager::Converters
