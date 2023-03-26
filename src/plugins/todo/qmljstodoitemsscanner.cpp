// Copyright (C) 2016 Dmitry Savchenko
// Copyright (C) 2016 Vasiliy Sorokin
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljstodoitemsscanner.h"

#include <projectexplorer/project.h>

#include <qmljs/parser/qmljsengine_p.h>

#include <utils/stringutils.h>

namespace Todo {
namespace Internal {

QmlJsTodoItemsScanner::QmlJsTodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    TodoItemsScanner(keywordList, parent)
{
    QmlJS::ModelManagerInterface *model = QmlJS::ModelManagerInterface::instance();
    connect(model, &QmlJS::ModelManagerInterface::documentUpdated,
            this, &QmlJsTodoItemsScanner::documentUpdated, Qt::DirectConnection);

    setParams(keywordList);
}

bool QmlJsTodoItemsScanner::shouldProcessFile(const Utils::FilePath &fileName)
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    const QList<QmlJS::ModelManagerInterface::ProjectInfo> infoList = modelManager->projectInfos();
    for (const QmlJS::ModelManagerInterface::ProjectInfo &info : infoList) {
        if (info.sourceFiles.contains(fileName))
            return true;
    }

    return false;
}

void QmlJsTodoItemsScanner::scannerParamsChanged()
{
    // We need to rescan everything known to the code model
    // TODO: It would be nice to only tokenize the source files, not update the code model entirely.

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    Utils::FilePaths filesToBeUpdated;
    for (const QmlJS::ModelManagerInterface::ProjectInfo &info : modelManager->projectInfos())
        filesToBeUpdated << info.sourceFiles;

    modelManager->updateSourceFiles(filesToBeUpdated, false);
}

void QmlJsTodoItemsScanner::documentUpdated(QmlJS::Document::Ptr doc)
{
    if (shouldProcessFile(doc->fileName()))
        processDocument(doc);
}

void QmlJsTodoItemsScanner::processDocument(QmlJS::Document::Ptr doc)
{
    QList<TodoItem> itemList;

    const QList<QmlJS::SourceLocation> sourceLocations = doc->engine()->comments();
    for (const QmlJS::SourceLocation &sourceLocation :  sourceLocations) {
        QString source = doc->source().mid(sourceLocation.begin(), sourceLocation.length).trimmed();

        // Process every line
        // TODO: Do not create QStringList, just iterate through a string tracking line endings.
        QStringList commentLines = source.split('\n', Qt::SkipEmptyParts);
        quint32 startLine = sourceLocation.startLine;
        for (int j = 0; j < commentLines.count(); ++j) {
            const QString &commentLine = commentLines.at(j);
            processCommentLine(doc->fileName().toString(), commentLine, startLine + j, itemList);
        }

    }

    emit itemsFetched(doc->fileName().toString(), itemList);
}

}
}
