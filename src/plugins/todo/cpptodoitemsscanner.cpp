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

#include "cpptodoitemsscanner.h"

#include <projectexplorer/project.h>
#include <TranslationUnit.h>

namespace Todo {
namespace Internal {

CppTodoItemsScanner::CppTodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    TodoItemsScanner(keywordList, parent)
{
    CPlusPlus::CppModelManagerInterface *modelManager = CPlusPlus::CppModelManagerInterface::instance();

    connect(modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)), this,
        SLOT(documentUpdated(CPlusPlus::Document::Ptr)), Qt::DirectConnection);
}

bool CppTodoItemsScanner::shouldProcessFile(const QString &fileName)
{
    CPlusPlus::CppModelManagerInterface *modelManager = CPlusPlus::CppModelManagerInterface::instance();

    foreach (const CPlusPlus::CppModelManagerInterface::ProjectInfo &info, modelManager->projectInfos())
        if (info.project().data()->files(ProjectExplorer::Project::ExcludeGeneratedFiles).contains(fileName))
            return true;

    return false;
}

void CppTodoItemsScanner::keywordListChanged()
{
    // We need to rescan everything known to the code model
    // TODO: It would be nice to only tokenize the source files, not update the code model entirely.

    CPlusPlus::CppModelManagerInterface *modelManager = CPlusPlus::CppModelManagerInterface::instance();

    QStringList filesToBeUpdated;
    foreach (const CPlusPlus::CppModelManagerInterface::ProjectInfo &info, modelManager->projectInfos())
        filesToBeUpdated << info.project().data()->files(ProjectExplorer::Project::ExcludeGeneratedFiles);

    modelManager->updateSourceFiles(filesToBeUpdated);
}

void CppTodoItemsScanner::documentUpdated(CPlusPlus::Document::Ptr doc)
{
    if (shouldProcessFile(doc->fileName()))
        processDocument(doc);
}

void CppTodoItemsScanner::processDocument(CPlusPlus::Document::Ptr doc)
{
    QList<TodoItem> itemList;

    CPlusPlus::TranslationUnit *translationUnit = doc->translationUnit();

    for (unsigned i = 0; i < translationUnit->commentCount(); ++i) {

        // Get comment source
        CPlusPlus::Token token = doc->translationUnit()->commentAt(i);
        QByteArray source = doc->utf8Source().mid(token.begin(), token.length()).trimmed();

        if ((token.kind() == CPlusPlus::T_COMMENT) || (token.kind() == CPlusPlus::T_DOXY_COMMENT)) {
            // Remove trailing "*/"
            source = source.left(source.length() - 2);
        }

        // Process every line of the comment
        // TODO: Do not create QStringList, just iterate through a string tracking line endings.
        const QStringList commentLines =
            QString::fromUtf8(source).split(QLatin1Char('\n'), QString::SkipEmptyParts);
        unsigned lineNumber = 0;
        translationUnit->getPosition(token.begin(), &lineNumber);
        for (int j = 0; j < commentLines.count(); ++j) {
            const QString &commentLine = commentLines.at(j);
            processCommentLine(doc->fileName(), commentLine, lineNumber + j, itemList);
        }
    }
    emit itemsFetched(doc->fileName(), itemList);
}

}
}
