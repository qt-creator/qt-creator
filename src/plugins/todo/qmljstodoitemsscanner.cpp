/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljstodoitemsscanner.h"

#include <projectexplorer/project.h>

#include <qmljs/parser/qmljsengine_p.h>

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

bool QmlJsTodoItemsScanner::shouldProcessFile(const QString &fileName)
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    foreach (const QmlJS::ModelManagerInterface::ProjectInfo &info, modelManager->projectInfos()) {
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

    QStringList filesToBeUpdated;
    foreach (const QmlJS::ModelManagerInterface::ProjectInfo &info, modelManager->projectInfos())
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

    foreach (const QmlJS::AST::SourceLocation &sourceLocation, doc->engine()->comments()) {
        QString source = doc->source().mid(sourceLocation.begin(), sourceLocation.length).trimmed();

        // Process every line
        // TODO: Do not create QStringList, just iterate through a string tracking line endings.
        QStringList commentLines = source.split(QLatin1Char('\n'), QString::SkipEmptyParts);
        quint32 startLine = sourceLocation.startLine;
        for (int j = 0; j < commentLines.count(); ++j) {
            const QString &commentLine = commentLines.at(j);
            processCommentLine(doc->fileName(), commentLine, startLine + j, itemList);
        }

    }

    emit itemsFetched(doc->fileName(), itemList);
}

}
}
