/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "basetextmarkregistry.h"
#include "basetextmark.h"
#include "itexteditor.h"
#include "basetextdocument.h"
#include "texteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace Utils;

namespace TextEditor {
namespace Internal {

BaseTextMarkRegistry::BaseTextMarkRegistry(QObject *parent)
    : QObject(parent)
{
    connect(EditorManager::instance(), SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));

    connect(DocumentManager::instance(), SIGNAL(allDocumentsRenamed(QString,QString)),
            this, SLOT(allDocumentsRenamed(QString,QString)));
    connect(DocumentManager::instance(), SIGNAL(documentRenamed(Core::IDocument*,QString,QString)),
            this, SLOT(documentRenamed(Core::IDocument*,QString,QString)));
}

void BaseTextMarkRegistry::add(BaseTextMark *mark)
{
    m_marks[FileName::fromString(mark->fileName())].insert(mark);
    DocumentModel *documentModel = EditorManager::documentModel();
    ITextEditorDocument *document
            = qobject_cast<ITextEditorDocument*>(documentModel->documentForFilePath(mark->fileName()));
    if (!document)
        return;
    document->markableInterface()->addMark(mark);
}

bool BaseTextMarkRegistry::remove(BaseTextMark *mark)
{
    return m_marks[FileName::fromString(mark->fileName())].remove(mark);
}

void BaseTextMarkRegistry::editorOpened(Core::IEditor *editor)
{
    ITextEditorDocument *document = qobject_cast<ITextEditorDocument *>(editor ? editor->document() : 0);
    if (!document)
        return;
    if (!m_marks.contains(FileName::fromString(document->filePath())))
        return;

    foreach (BaseTextMark *mark, m_marks.value(FileName::fromString(document->filePath())))
        document->markableInterface()->addMark(mark);
}

void BaseTextMarkRegistry::documentRenamed(IDocument *document, const
                                           QString &oldName, const QString &newName)
{
    TextEditor::BaseTextDocument *baseTextDocument
            = qobject_cast<TextEditor::BaseTextDocument *>(document);
    if (!document)
        return;
    FileName oldFileName = FileName::fromString(oldName);
    FileName newFileName = FileName::fromString(newName);
    if (!m_marks.contains(oldFileName))
        return;

    QSet<BaseTextMark *> toBeMoved;
    foreach (ITextMark *mark, baseTextDocument->markableInterface()->marks())
        if (BaseTextMark *baseTextMark = dynamic_cast<BaseTextMark *>(mark))
            toBeMoved.insert(baseTextMark);

    m_marks[oldFileName].subtract(toBeMoved);
    m_marks[newFileName].unite(toBeMoved);

    foreach (BaseTextMark *mark, toBeMoved)
        mark->updateFileName(newName);
}

void BaseTextMarkRegistry::allDocumentsRenamed(const QString &oldName, const QString &newName)
{
    FileName oldFileName = FileName::fromString(oldName);
    FileName newFileName = FileName::fromString(newName);
    if (!m_marks.contains(oldFileName))
        return;

    QSet<BaseTextMark *> oldFileNameMarks = m_marks.value(oldFileName);

    m_marks[newFileName].unite(oldFileNameMarks);
    m_marks[oldFileName].clear();

    foreach (BaseTextMark *mark, oldFileNameMarks)
        mark->updateFileName(newName);
}

} // namespace Internal

BaseTextMark::BaseTextMark(const QString &fileName, int lineNumber)
    : ITextMark(lineNumber), m_fileName(fileName)
{
}

// we need two phase initilization, since we are calling virtual functions
// of BaseTextMark in add() and also accessing widthFactor
// which might be set in the derived constructor
void BaseTextMark::init()
{
    Internal::TextEditorPlugin::instance()->baseTextMarkRegistry()->add(this);
}

BaseTextMark::~BaseTextMark()
{
    // oha we are deleted
    bool b = Internal::TextEditorPlugin::instance()->baseTextMarkRegistry()->remove(this);
    // If you get a assertion in this line, init() was never called
    QTC_CHECK(b);
}

void BaseTextMark::updateFileName(const QString &fileName)
{
    m_fileName = fileName;
}

} // namespace TextEditor
