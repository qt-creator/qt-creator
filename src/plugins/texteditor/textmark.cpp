/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "textmark.h"
#include "textdocument.h"
#include "textmarkregistry.h"
#include "texteditor.h"
#include "texteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/documentmanager.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace Utils;
using namespace TextEditor::Internal;

namespace TextEditor {

TextMark::TextMark(const QString &fileName, int lineNumber, Id category)
    : m_baseTextDocument(0),
      m_fileName(fileName),
      m_lineNumber(lineNumber),
      m_priority(NormalPriority),
      m_visible(true),
      m_category(category),
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

TextMark::TextMark(TextMark &&other) Q_DECL_NOEXCEPT
    : m_baseTextDocument(std::move(other.m_baseTextDocument)),
      m_fileName(std::move(other.m_fileName)),
      m_lineNumber(std::move(other.m_lineNumber)),
      m_priority(std::move(other.m_priority)),
      m_visible(std::move(other.m_visible)),
      m_icon(std::move(other.m_icon)),
      m_color(std::move(other.m_color)),
      m_category(std::move(other.m_category)),
      m_widthFactor(std::move(other.m_widthFactor))
{
    other.m_baseTextDocument = nullptr;
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

Theme::Color TextMark::categoryColor(Id category)
{
    return TextEditorPlugin::baseTextMarkRegistry()->categoryColor(category);
}

bool TextMark::categoryHasColor(Id category)
{
    return TextEditorPlugin::baseTextMarkRegistry()->categoryHasColor(category);
}

void TextMark::setCategoryColor(Id category, Theme::Color color)
{
    TextEditorPlugin::baseTextMarkRegistry()->setCategoryColor(category, color);
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

Id TextMark::category() const
{
    return m_category;
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

TextDocument *TextMark::baseTextDocument() const
{
    return m_baseTextDocument;
}

void TextMark::setBaseTextDocument(TextDocument *baseTextDocument)
{
    m_baseTextDocument = baseTextDocument;
}


TextMarkRegistry::TextMarkRegistry(QObject *parent)
    : QObject(parent)
{
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &TextMarkRegistry::editorOpened);

    connect(DocumentManager::instance(), &DocumentManager::allDocumentsRenamed,
            this, &TextMarkRegistry::allDocumentsRenamed);
    connect(DocumentManager::instance(), &DocumentManager::documentRenamed,
            this, &TextMarkRegistry::documentRenamed);
}

void TextMarkRegistry::add(TextMark *mark)
{
    m_marks[FileName::fromString(mark->fileName())].insert(mark);
    auto document = qobject_cast<TextDocument*>(DocumentModel::documentForFilePath(mark->fileName()));
    if (!document)
        return;
    document->addMark(mark);
}

bool TextMarkRegistry::remove(TextMark *mark)
{
    return m_marks[FileName::fromString(mark->fileName())].remove(mark);
}

Theme::Color TextMarkRegistry::categoryColor(Id category)
{
    return m_colors.value(category, Theme::ProjectExplorer_TaskWarn_TextMarkColor);
}

bool TextMarkRegistry::categoryHasColor(Id category)
{
    return m_colors.contains(category);
}

void TextMarkRegistry::setCategoryColor(Id category, Theme::Color color)
{
    if (m_colors[category] == color)
        return;
    m_colors[category] = color;
}

void TextMarkRegistry::editorOpened(IEditor *editor)
{
    auto document = qobject_cast<TextDocument *>(editor ? editor->document() : 0);
    if (!document)
        return;
    if (!m_marks.contains(document->filePath()))
        return;

    foreach (TextMark *mark, m_marks.value(document->filePath()))
        document->addMark(mark);
}

void TextMarkRegistry::documentRenamed(IDocument *document, const
                                           QString &oldName, const QString &newName)
{
    TextDocument *baseTextDocument = qobject_cast<TextDocument *>(document);
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
