/**************************************************************************
**
** Copyright (c) 2013 Dmitry Savchenko
** Copyright (c) 2013 Vasiliy Sorokin
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljstodoitemsscanner.h"

#include <projectexplorer/project.h>

namespace Todo {
namespace Internal {

QmlJsTodoItemsScanner::QmlJsTodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    TodoItemsScanner(keywordList, parent)
{
    QmlJS::ModelManagerInterface *model = QmlJS::ModelManagerInterface::instance();
    connect(model, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
        this, SLOT(documentUpdated(QmlJS::Document::Ptr)), Qt::DirectConnection);
}

bool QmlJsTodoItemsScanner::shouldProcessFile(const QString &fileName)
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    foreach (const QmlJS::ModelManagerInterface::ProjectInfo &info, modelManager->projectInfos())
        if (info.project->files(ProjectExplorer::Project::ExcludeGeneratedFiles).contains(fileName))
            return true;

    return false;
}

void QmlJsTodoItemsScanner::keywordListChanged()
{
    // We need to rescan everything known to the code model
    // TODO: It would be nice to only tokenize the source files, not update the code model entirely.

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    QStringList filesToBeUpdated;
    foreach (const QmlJS::ModelManagerInterface::ProjectInfo &info, modelManager->projectInfos())
        filesToBeUpdated << info.project->files(ProjectExplorer::Project::ExcludeGeneratedFiles);

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
