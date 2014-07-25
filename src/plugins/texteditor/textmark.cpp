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

#include "textmark.h"
#include "basetextdocument.h"
#include "textmarkregistry.h"
#include "basetexteditor.h"
#include "texteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace Utils;
using namespace TextEditor::Internal;

namespace TextEditor {

TextMark::TextMark(const QString &fileName, int lineNumber)
    : m_baseTextDocument(0),
      m_fileName(fileName),
      m_lineNumber(lineNumber),
      m_priority(NormalPriority),
      m_visible(true),
      m_widthFactor(1.0)
{
    if (!m_fileName.isEmpty())
        TextEditorPlugin::baseTextMarkRegistry()->add(this);
}

TextMark::~TextMark()
{
    TextEditorPlugin::baseTextMarkRegistry()->remove(this);
    if (m_baseTextDocument)
        m_baseTextDocument->removeMark(this);
    m_baseTextDocument = 0;
}

QString TextMark::fileName() const
{
    return m_fileName;
}

void TextMark::updateFileName(const QString &fileName)
{
    if (fileName == m_fileName)
        return;
    if (!m_fileName.isEmpty())
        TextEditorPlugin::baseTextMarkRegistry()->remove(this);
    m_fileName = fileName;
    if (!m_fileName.isEmpty())
        TextEditorPlugin::baseTextMarkRegistry()->add(this);
}

int TextMark::lineNumber() const
{
    return m_lineNumber;
}

void TextMark::paint(QPainter *painter, const QRect &rect) const
{
    m_icon.paint(painter, rect, Qt::AlignCenter);
}

void TextMark::updateLineNumber(int lineNumber)
{
    m_lineNumber = lineNumber;
}

void TextMark::move(int line)
{
    if (line == m_lineNumber)
        return;
    const int previousLine = m_lineNumber;
    m_lineNumber = line;
    if (m_baseTextDocument)
        m_baseTextDocument->moveMark(this, previousLine);
}

void TextMark::updateBlock(const QTextBlock &)
{}

void TextMark::removedFromEditor()
{}

void TextMark::setIcon(const QIcon &icon)
{
    m_icon = icon;
}

void TextMark::updateMarker()
{
    if (m_baseTextDocument)
        m_baseTextDocument->updateMark(this);
}

void TextMark::setPriority(Priority priority)
{
    m_priority = priority;
}

TextMark::Priority TextMark::priority() const
{
    return m_priority;
}

bool TextMark::isVisible() const
{
    return m_visible;
}

void TextMark::setVisible(bool visible)
{
    m_visible = visible;
    if (m_baseTextDocument)
        m_baseTextDocument->updateMark(this);
}

double TextMark::widthFactor() const
{
    return m_widthFactor;
}

void TextMark::setWidthFactor(double factor)
{
    m_widthFactor = factor;
}

bool TextMark::isClickable() const
{
    return false;
}

void TextMark::clicked()
{}

bool TextMark::isDraggable() const
{
    return false;
}

void TextMark::dragToLine(int lineNumber)
{
    Q_UNUSED(lineNumber);
}

BaseTextDocument *TextMark::baseTextDocument() const
{
    return m_baseTextDocument;
}

void TextMark::setBaseTextDocument(BaseTextDocument *baseTextDocument)
{
    m_baseTextDocument = baseTextDocument;
}


TextMarkRegistry::TextMarkRegistry(QObject *parent)
    : QObject(parent)
{
    connect(EditorManager::instance(), SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));

    connect(DocumentManager::instance(), SIGNAL(allDocumentsRenamed(QString,QString)),
            this, SLOT(allDocumentsRenamed(QString,QString)));
    connect(DocumentManager::instance(), SIGNAL(documentRenamed(Core::IDocument*,QString,QString)),
            this, SLOT(documentRenamed(Core::IDocument*,QString,QString)));
}

void TextMarkRegistry::add(TextMark *mark)
{
    m_marks[FileName::fromString(mark->fileName())].insert(mark);
    auto document = qobject_cast<BaseTextDocument*>(DocumentModel::documentForFilePath(mark->fileName()));
    if (!document)
        return;
    document->addMark(mark);
}

bool TextMarkRegistry::remove(TextMark *mark)
{
    return m_marks[FileName::fromString(mark->fileName())].remove(mark);
}

void TextMarkRegistry::editorOpened(Core::IEditor *editor)
{
    auto document = qobject_cast<BaseTextDocument *>(editor ? editor->document() : 0);
    if (!document)
        return;
    if (!m_marks.contains(FileName::fromString(document->filePath())))
        return;

    foreach (TextMark *mark, m_marks.value(FileName::fromString(document->filePath())))
        document->addMark(mark);
}

void TextMarkRegistry::documentRenamed(IDocument *document, const
                                           QString &oldName, const QString &newName)
{
    BaseTextDocument *baseTextDocument = qobject_cast<BaseTextDocument *>(document);
    if (!document)
        return;
    FileName oldFileName = FileName::fromString(oldName);
    FileName newFileName = FileName::fromString(newName);
    if (!m_marks.contains(oldFileName))
        return;

    QSet<TextMark *> toBeMoved;
    foreach (TextMark *mark, baseTextDocument->marks())
        toBeMoved.insert(mark);

    m_marks[oldFileName].subtract(toBeMoved);
    m_marks[newFileName].unite(toBeMoved);

    foreach (TextMark *mark, toBeMoved)
        mark->updateFileName(newName);
}

void TextMarkRegistry::allDocumentsRenamed(const QString &oldName, const QString &newName)
{
    FileName oldFileName = FileName::fromString(oldName);
    FileName newFileName = FileName::fromString(newName);
    if (!m_marks.contains(oldFileName))
        return;

    QSet<TextMark *> oldFileNameMarks = m_marks.value(oldFileName);

    m_marks[newFileName].unite(oldFileNameMarks);
    m_marks[oldFileName].clear();

    foreach (TextMark *mark, oldFileNameMarks)
        mark->updateFileName(newName);
}

} // namespace TextEditor
