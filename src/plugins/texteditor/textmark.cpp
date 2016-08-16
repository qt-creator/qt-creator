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

#include <QGridLayout>

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

const QIcon &TextMark::icon() const
{
    return m_icon;
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

void TextMark::setDefaultToolTip(Id category, const QString &toolTip)
{
    TextEditorPlugin::baseTextMarkRegistry()->setDefaultToolTip(category, toolTip);
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

void TextMark::addToToolTipLayout(QGridLayout *target)
{
    auto *contentLayout = new QVBoxLayout;
    addToolTipContent(contentLayout);
    if (contentLayout->count() > 0) {
        const int row = target->rowCount();
        if (!m_icon.isNull()) {
            auto iconLabel = new QLabel;
            iconLabel->setPixmap(m_icon.pixmap(16, 16));
            target->addWidget(iconLabel, row, 0, Qt::AlignTop | Qt::AlignHCenter);
        }
        target->addLayout(contentLayout, row, 1);
    }
}

bool TextMark::addToolTipContent(QLayout *target)
{
    QString text = m_toolTip;
    if (text.isEmpty()) {
        text = TextEditorPlugin::baseTextMarkRegistry()->defaultToolTip(m_category);
        if (text.isEmpty())
            return false;
    }

    auto textLabel = new QLabel;
    textLabel->setText(text);
    // Differentiate between tool tips that where explicitly set and default tool tips.
    textLabel->setEnabled(!m_toolTip.isEmpty());
    target->addWidget(textLabel);

    return true;
}

TextDocument *TextMark::baseTextDocument() const
{
    return m_baseTextDocument;
}

void TextMark::setBaseTextDocument(TextDocument *baseTextDocument)
{
    m_baseTextDocument = baseTextDocument;
}

QString TextMark::toolTip() const
{
    return m_toolTip;
}

void TextMark::setToolTip(const QString &toolTip)
{
    m_toolTip = toolTip;
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

void TextMarkRegistry::setCategoryColor(Id category, Theme::Color newColor)
{
    Theme::Color &color = m_colors[category];
    if (color == newColor)
        return;
    color = newColor;
}

QString TextMarkRegistry::defaultToolTip(Id category) const
{
    return m_defaultToolTips[category];
}

void TextMarkRegistry::setDefaultToolTip(Id category, const QString &toolTip)
{
    QString &defaultToolTip = m_defaultToolTips[category];
    if (defaultToolTip == toolTip)
        return;
    defaultToolTip = toolTip;
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
    if (!baseTextDocument)
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
