// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectfileformat.h"

#include "qmlprojectitem.h"
#include "filefilteritems.h"

#include <qmljs/qmljssimplereader.h>

#include <utils/filepath.h>

#include <QDebug>
#include <QVariant>

#include <memory>

using namespace Utils;

enum {
    debug = false
};

namespace   {

std::unique_ptr<QmlProjectManager::FileFilterBaseItem> setupFileFilterItem(
    std::unique_ptr<QmlProjectManager::FileFilterBaseItem> fileFilterItem,
    const QmlJS::SimpleReaderNode::Ptr &node)
{
    const auto directoryProperty = node->property(QLatin1String("directory"));
    if (directoryProperty.isValid())
        fileFilterItem->setDirectory(directoryProperty.value.toString());

    const auto recursiveProperty = node->property(QLatin1String("recursive"));
    if (recursiveProperty.isValid())
        fileFilterItem->setRecursive(recursiveProperty.value.toBool());

    const auto pathsProperty = node->property(QLatin1String("paths"));
    if (pathsProperty.isValid())
        fileFilterItem->setPathsProperty(pathsProperty.value.toStringList());

    // "paths" and "files" have the same functionality
    const auto filesProperty = node->property(QLatin1String("files"));
    if (filesProperty.isValid())
        fileFilterItem->setPathsProperty(filesProperty.value.toStringList());

    const auto filterProperty = node->property(QLatin1String("filter"));
    if (filterProperty.isValid())
        fileFilterItem->setFilter(filterProperty.value.toString());

    if (debug)
        qDebug() << "directory:" << directoryProperty.value << "recursive" << recursiveProperty.value
                 << "paths" << pathsProperty.value << "files" << filesProperty.value;
    return fileFilterItem;
}

} //namespace

namespace QmlProjectManager {

std::unique_ptr<QmlProjectItem> QmlProjectFileFormat::parseProjectFile(const Utils::FilePath &fileName,
                                                                       QString *errorMessage)
{
    QmlJS::SimpleReader simpleQmlJSReader;

    const QmlJS::SimpleReaderNode::Ptr rootNode = simpleQmlJSReader.readFile(fileName.toString());

    if (!simpleQmlJSReader.errors().isEmpty() || !rootNode->isValid()) {
        qWarning() << "unable to parse:" << fileName;
        qWarning() << simpleQmlJSReader.errors();
        if (errorMessage)
            *errorMessage = simpleQmlJSReader.errors().join(QLatin1String(", "));
        return nullptr;
    }

    if (rootNode->name() == QLatin1String("Project")) {
        auto projectItem = std::make_unique<QmlProjectItem>();

        const auto mainFileProperty = rootNode->property(QLatin1String("mainFile"));
        if (mainFileProperty.isValid())
            projectItem->setMainFile(mainFileProperty.value.toString());

        const auto mainUiFileProperty = rootNode->property(QLatin1String("mainUiFile"));
        if (mainUiFileProperty.isValid())
            projectItem->setMainUiFile(mainUiFileProperty.value.toString());

        const auto importPathsProperty = rootNode->property(QLatin1String("importPaths"));
        if (importPathsProperty.isValid()) {
            QStringList list = importPathsProperty.value.toStringList();
            list.removeAll(".");
            projectItem->setImportPaths(list);
        }

        const auto fileSelectorsProperty = rootNode->property(QLatin1String("fileSelectors"));
        if (fileSelectorsProperty.isValid())
            projectItem->setFileSelectors(fileSelectorsProperty.value.toStringList());

        const auto multilanguageSupportProperty = rootNode->property(
            QLatin1String("multilanguageSupport"));
        if (multilanguageSupportProperty.isValid())
            projectItem->setMultilanguageSupport(multilanguageSupportProperty.value.toBool());

        const auto languagesProperty = rootNode->property(QLatin1String("supportedLanguages"));
        if (languagesProperty.isValid())
            projectItem->setSupportedLanguages(languagesProperty.value.toStringList());

        const auto primaryLanguageProperty = rootNode->property(QLatin1String("primaryLanguage"));
        if (primaryLanguageProperty.isValid())
            projectItem->setPrimaryLanguage(primaryLanguageProperty.value.toString());

        const auto forceFreeTypeProperty = rootNode->property("forceFreeType");
        if (forceFreeTypeProperty.isValid())
            projectItem->setForceFreeType(forceFreeTypeProperty.value.toBool());

        const auto targetDirectoryPropery = rootNode->property("targetDirectory");
        if (targetDirectoryPropery.isValid())
            projectItem->setTargetDirectory(FilePath::fromVariant(targetDirectoryPropery.value));

        const auto qtForMCUProperty = rootNode->property("qtForMCUs");
        if (qtForMCUProperty.isValid() && qtForMCUProperty.value.toBool())
            projectItem->setQtForMCUs(qtForMCUProperty.value.toBool());

        const auto qt6ProjectProperty = rootNode->property("qt6Project");
        if (qt6ProjectProperty.isValid() && qt6ProjectProperty.value.toBool())
            projectItem->setQt6Project(qt6ProjectProperty.value.toBool());

        const auto widgetAppProperty = rootNode->property("widgetApp");
        if (widgetAppProperty.isValid())
            projectItem->setWidgetApp(widgetAppProperty.value.toBool());

        if (debug)
            qDebug() << "importPath:" << importPathsProperty.value << "mainFile:" << mainFileProperty.value;

        const QList<QmlJS::SimpleReaderNode::Ptr> childNodes = rootNode->children();
        for (const QmlJS::SimpleReaderNode::Ptr &childNode : childNodes) {
            if (debug)
                qDebug() << "reading type:" << childNode->name();

            if (childNode->name() == QLatin1String("QmlFiles")) {
                projectItem->appendContent(
                    setupFileFilterItem(std::make_unique<FileFilterItem>("*.qml"), childNode));
            } else if (childNode->name() == QLatin1String("JavaScriptFiles")) {
                projectItem->appendContent(
                    setupFileFilterItem(std::make_unique<FileFilterItem>("*.js"), childNode));
            } else if (childNode->name() == QLatin1String("ImageFiles")) {
                projectItem->appendContent(
                    setupFileFilterItem(std::make_unique<ImageFileFilterItem>(), childNode));
            } else if (childNode->name() == QLatin1String("CssFiles")) {
                projectItem->appendContent(
                    setupFileFilterItem(std::make_unique<FileFilterItem>("*.css"), childNode));
            } else if (childNode->name() == QLatin1String("FontFiles")) {
                projectItem->appendContent(
                    setupFileFilterItem(std::make_unique<FileFilterItem>("*.ttf;*.otf"), childNode));
            } else if (childNode->name() == QLatin1String("Files")) {
                projectItem->appendContent(
                    setupFileFilterItem(std::make_unique<FileFilterBaseItem>(), childNode));
            } else if (childNode->name() == "Environment") {
                const auto properties = childNode->properties();
                auto i = properties.constBegin();
                while (i != properties.constEnd()) {
                    projectItem->addToEnviroment(i.key(), i.value().value.toString());
                    ++i;
                }
            } else if (childNode->name() == "ShaderTool") {
                QmlJS::SimpleReaderNode::Property commandLine = childNode->property("args");
                if (commandLine.isValid()) {
                    const QStringList quotedArgs = commandLine.value.toString().split('\"');
                    QStringList args;
                    for (int i = 0; i < quotedArgs.size(); ++i) {
                        // Each odd arg in this list is a single quoted argument, which we should
                        // not be split further
                        if (i % 2 == 0)
                            args.append(quotedArgs[i].trimmed().split(' '));
                        else
                            args.append(quotedArgs[i]);
                    }
                    args.removeAll({});
                    args.append("-o"); // Prepare for adding output file as next arg
                    projectItem->setShaderToolArgs(args);
                }
                QmlJS::SimpleReaderNode::Property files = childNode->property("files");
                if (files.isValid())
                    projectItem->setShaderToolFiles(files.value.toStringList());
            } else {
                qWarning() << "Unknown type:" << childNode->name();
            }
        }
        return projectItem;
    }

    if (errorMessage)
        *errorMessage = tr("Invalid root element: %1").arg(rootNode->name());

    return nullptr;
}

} // namespace QmlProjectManager
