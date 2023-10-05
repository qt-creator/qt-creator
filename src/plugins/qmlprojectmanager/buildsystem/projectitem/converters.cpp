// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "converters.h"

#include <QJsonArray>

namespace QmlProjectManager::Converters {

using PropsPair = QPair<QString, QStringList>;
struct FileProps
{
    const PropsPair image{"image",
                          QStringList{"*.jpeg", "*.jpg", "*.png", "*.svg", "*.hdr", ".ktx"}};
    const PropsPair qml{"qml", QStringList{"*.qml"}};
    const PropsPair qmlDir{"qmldir", QStringList{"qmldir"}};
    const PropsPair javaScr{"javaScript", QStringList{"*.js", "*.ts"}};
    const PropsPair video{"video", QStringList{"*.mp4"}};
    const PropsPair sound{"sound", QStringList{"*.mp3", "*.wav"}};
    const PropsPair font{"font", QStringList{"*.ttf", "*.otf"}};
    const PropsPair config{"config", QStringList{"*.conf"}};
    const PropsPair styling{"styling", QStringList{"*.css"}};
    const PropsPair mesh{"meshes", QStringList{"*.mesh"}};
    const PropsPair
        shader{"shader",
               QStringList{"*.glsl", "*.glslv", "*.glslf", "*.vsh", "*.fsh", "*.vert", "*.frag"}};
};

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
    QJsonObject filesConfig = rootObject["fileGroups"].toObject();

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

    auto appendDirectories =
        [&startObject, &endObject, &appendString, &filesConfig](const QString &jsonKey,
                                                                const QString &qmlKey) {
            QJsonValue dirsObj = filesConfig[jsonKey].toObject()["directories"];
            const QStringList dirs = dirsObj.toVariant().toStringList();
            for (const QString &directory : dirs) {
                startObject(qmlKey);
                appendString("directory", directory);
                endObject();
            }
        };

    auto appendFiles = [&startObject,
                        &endObject,
                        &appendString,
                        &appendArray,
                        &filesConfig](const QString &jsonKey, const QString &qmlKey) {
        QJsonValue dirsObj = filesConfig[jsonKey].toObject()["directories"];
        QJsonValue filesObj = filesConfig[jsonKey].toObject()["files"];
        QJsonValue filtersObj = filesConfig[jsonKey].toObject()["filters"];

        const QStringList directories = dirsObj.toVariant().toStringList();
        for (const QString &directory : directories) {
            startObject(qmlKey);
            appendString("directory", directory);
            appendString("filters", filtersObj.toVariant().toStringList().join(";"));

            if (!filesObj.toArray().isEmpty()) {
                QStringList fileList;
                const QJsonArray files = filesObj.toArray();
                for (const QJsonValue &file : files)
                    fileList.append(file.toObject()["name"].toString());
                appendArray("files", fileList);
            }
            endObject();
        }
    };

    // start creating the file content
    appendComment("prop: json-converted");
    appendComment("prop: auto-generated");

    ts << Qt::endl << "import QmlProject" << Qt::endl;
    {
        startObject("Project");

        { // append non-object props
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
        }

        { // append Environment object
            startObject("Environment");
            const QStringList keys = environmentConfig.keys();
            for (const QString &key : keys)
                appendItem(key, environmentConfig[key].toString(), true);
            endObject();
        }

        { // append ShaderTool object
            if (!shaderConfig["args"].toVariant().toStringList().isEmpty()) {
                startObject("ShaderTool");
                appendString("args",
                             shaderConfig["args"].toVariant().toStringList().join(" ").replace(
                                 "\"", "\\\""));
                appendArray("files", shaderConfig["files"].toVariant().toStringList());
                endObject();
            }
        }

        { // append files objects
            appendDirectories("qml", "QmlFiles");
            appendDirectories("javaScript", "JavaScriptFiles");
            appendDirectories("image", "ImageFiles");
            appendFiles("config", "Files");
            appendFiles("font", "Files");
            appendFiles("meshes", "Files");
            appendFiles("qmldir", "Files");
            appendFiles("shader", "Files");
            appendFiles("sound", "Files");
            appendFiles("video", "Files");
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
        const QStringList childPropNames = node->propertyNames();
        for (const QString &childPropName : childPropNames)
            tObj.insert(childPropName, node->property(childPropName).value.toJsonValue());
        return tObj;
    };

    auto toCamelCase = [](const QString &s) { return QString(s).replace(0, 1, s[0].toLower()); };

    QJsonObject rootObject; // root object
    QJsonObject fileGroupsObject;
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
            PropsPair propsPair;
            FileProps fileProps;
            const QString childNodeName = childNode->name().toLower().remove("qds.");
            const QmlJS::SimpleReaderNode::Property childNodeFilter = childNode->property("filter");
            const QmlJS::SimpleReaderNode::Property childNodeDirectory = childNode->property(
                "directory");
            const QmlJS::SimpleReaderNode::Property childNodeFiles = childNode->property("files");
            const QString childNodeFilterValue = childNodeFilter.value.toString();

            if (childNodeName == "qmlfiles" || childNodeFilterValue.contains("*.qml")) {
                propsPair = fileProps.qml;
            } else if (childNodeName == "javascriptfiles") {
                propsPair = fileProps.javaScr;
            } else if (childNodeName == "imagefiles") {
                propsPair = fileProps.image;
            } else {
                if (childNodeFilter.isValid()) {
                    if (childNodeFilterValue.contains(".conf"))
                        propsPair = fileProps.config;
                    else if (childNodeFilterValue.contains(".ttf"))
                        propsPair = fileProps.font;
                    else if (childNodeFilterValue.contains("qmldir"))
                        propsPair = fileProps.qmlDir;
                    else if (childNodeFilterValue.contains(".wav"))
                        propsPair = fileProps.sound;
                    else if (childNodeFilterValue.contains(".mp4"))
                        propsPair = fileProps.video;
                    else if (childNodeFilterValue.contains(".mesh"))
                        propsPair = fileProps.mesh;
                    else if (childNodeFilterValue.contains(".glsl"))
                        propsPair = fileProps.shader;
                    else if (childNodeFilterValue.contains(".css"))
                        propsPair = fileProps.styling;
                }
            }

            // get all objects we'll work on
            QJsonObject targetObject = fileGroupsObject[propsPair.first].toObject();
            QJsonArray directories = targetObject["directories"].toArray();
            QJsonArray filters = targetObject["filters"].toArray();
            QJsonArray files = targetObject["files"].toArray();

            // populate & update filters
            if (filters.isEmpty()) {
                filters = QJsonArray::fromStringList(
                    propsPair.second); // populate the filters with the predefined ones
            }

            if (childNodeFilter.isValid()) { // append filters from qmlproject (merge)
                const QStringList filtersFromProjectFile = childNodeFilterValue.split(";");
                for (const QString &filter : filtersFromProjectFile) {
                    if (!filters.contains(QJsonValue(filter))) {
                        filters.append(QJsonValue(filter));
                    }
                }
            }

            // populate & update directories
            if (childNodeDirectory.isValid()) {
                directories.append(childNodeDirectory.value.toJsonValue());
            }
            if (directories.isEmpty())
                directories.append(".");

            // populate & update files
            if (childNodeFiles.isValid()) {
                const QJsonArray fileList = childNodeFiles.value.toJsonArray();
                for (const QJsonValue &file : fileList)
                    files.append(QJsonObject{{"name", file.toString()}});
            }

            // put everything back into the root object
            targetObject.insert("directories", directories);
            targetObject.insert("filters", filters);
            targetObject.insert("files", files);
            fileGroupsObject.insert(propsPair.first, targetObject);
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
