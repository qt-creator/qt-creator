/**************************************************************************
**
** Copyright (c) 2014 Dmitry Savchenko
** Copyright (c) 2014 Vasiliy Sorokin
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
#include <cplusplus/TranslationUnit.h>

#include <cctype>

namespace Todo {
namespace Internal {

CppTodoItemsScanner::CppTodoItemsScanner(const KeywordList &keywordList, QObject *parent) :
    TodoItemsScanner(keywordList, parent)
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    connect(modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)), this,
        SLOT(documentUpdated(CPlusPlus::Document::Ptr)), Qt::DirectConnection);
}

void CppTodoItemsScanner::keywordListChanged()
{
    // We need to rescan everything known to the code model
    // TODO: It would be nice to only tokenize the source files, not update the code model entirely.

    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    QSet<QString> filesToBeUpdated;
    foreach (const CppTools::ProjectInfo &info, modelManager->projectInfos())
        filesToBeUpdated.unite(info.project().data()->files(ProjectExplorer::Project::ExcludeGeneratedFiles).toSet());

    modelManager->updateSourceFiles(filesToBeUpdated);
}

void CppTodoItemsScanner::documentUpdated(CPlusPlus::Document::Ptr doc)
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    if (!modelManager->projectPart(doc->fileName()).isEmpty())
        processDocument(doc);
}

void CppTodoItemsScanner::processDocument(CPlusPlus::Document::Ptr doc)
{
    QList<TodoItem> itemList;

    CPlusPlus::TranslationUnit *translationUnit = doc->translationUnit();

    for (unsigned i = 0; i < translationUnit->commentCount(); ++i) {

        // Get comment source
        CPlusPlus::Token token = doc->translationUnit()->commentAt(i);
        QByteArray source = doc->utf8Source().mid(token.bytesBegin(), token.bytes()).trimmed();

        if ((token.kind() == CPlusPlus::T_COMMENT) || (token.kind() == CPlusPlus::T_DOXY_COMMENT)) {
            // Remove trailing "*/"
            source = source.left(source.length() - 2);
        }

        // Process every line of the comment
        unsigned lineNumber = 0;
        translationUnit->getPosition(token.utf16charsBegin(), &lineNumber);

        for (int from = 0, sz = source.size(); from < sz; ++lineNumber) {
            int to = source.indexOf('\n', from);
            if (to == -1)
                to = sz - 1;

            const char *start = source.constData() + from;
            const char *end = source.constData() + to;
            while (start != end && std::isspace(*start))
                ++start;
            while (start != end && std::isspace(*end))
                --end;
            const int length = end - start + 1;
            if (length > 0) {
                QString commentLine = QString::fromUtf8(start, length);
                processCommentLine(doc->fileName(), commentLine, lineNumber, itemList);
            }

            from = to + 1;
        }
    }
    emit itemsFetched(doc->fileName(), itemList);
}

}
}
