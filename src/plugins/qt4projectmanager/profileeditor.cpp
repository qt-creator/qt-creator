/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "profileeditor.h"

#include "profilehighlighter.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "profileeditorfactory.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QFileInfo>
#include <QDir>
#include <QSharedPointer>

namespace Qt4ProjectManager {
namespace Internal {

//
// ProFileEditor
//

ProFileEditor::ProFileEditor(ProFileEditorWidget *editor)
  : BaseTextEditor(editor)
{
    setContext(Core::Context(Constants::C_PROFILEEDITOR,
              TextEditor::Constants::C_TEXTEDITOR));
}

Core::IEditor *ProFileEditor::duplicate(QWidget *parent)
{
    ProFileEditorWidget *ret = new ProFileEditorWidget(parent, qobject_cast<ProFileEditorWidget*>(editorWidget())->factory(),
                                           qobject_cast<ProFileEditorWidget*>(editorWidget())->actionHandler());
    ret->duplicateFrom(editorWidget());
    TextEditor::TextEditorSettings::instance()->initializeEditor(ret);
    return ret->editor();
}

Core::Id ProFileEditor::id() const
{
    return Core::Id(Constants::PROFILE_EDITOR_ID);
}

//
// ProFileEditorWidget
//

ProFileEditorWidget::ProFileEditorWidget(QWidget *parent, ProFileEditorFactory *factory, TextEditor::TextEditorActionHandler *ah)
    : BaseTextEditorWidget(parent), m_factory(factory), m_ah(ah)
{
    QSharedPointer<ProFileDocument> doc(new ProFileDocument());
    doc->setMimeType(QLatin1String(Constants::PROFILE_MIMETYPE));
    setBaseTextDocument(doc);

    ah->setupActions(this);

    baseTextDocument()->setSyntaxHighlighter(new ProFileHighlighter);
    m_commentDefinition.clearCommentStyles();
    m_commentDefinition.setSingleLine(QString(QLatin1Char('#')));
}

void ProFileEditorWidget::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

static bool isValidFileNameChar(const QChar &c)
{
    return c.isLetterOrNumber()
            || c == QLatin1Char('.')
            || c == QLatin1Char('_')
            || c == QLatin1Char('-')
            || c == QLatin1Char('/')
            || c == QLatin1Char('\\');
}

ProFileEditorWidget::Link ProFileEditorWidget::findLinkAt(const QTextCursor &cursor,
                                      bool /* resolveTarget */)
{
    Link link;

    int lineNumber = 0, positionInBlock = 0;
    convertPosition(cursor.position(), &lineNumber, &positionInBlock);

    const QString block = cursor.block().text();

    // check if the current position is commented out
    const int hashPos = block.indexOf(QLatin1Char('#'));
    if (hashPos >= 0 && hashPos < positionInBlock)
        return link;

    // find the beginning of a filename
    QString buffer;
    int beginPos = positionInBlock - 1;
    while (beginPos >= 0) {
        QChar c = block.at(beginPos);
        if (isValidFileNameChar(c)) {
            buffer.prepend(c);
            beginPos--;
        } else {
            break;
        }
    }

    // find the end of a filename
    int endPos = positionInBlock;
    while (endPos < block.count()) {
        QChar c = block.at(endPos);
        if (isValidFileNameChar(c)) {
            buffer.append(c);
            endPos++;
        } else {
            break;
        }
    }

    if (buffer.isEmpty())
        return link;

    // remove trailing '\' since it can be line continuation char
    if (buffer.at(buffer.size() - 1) == QLatin1Char('\\')) {
        buffer.chop(1);
        endPos--;
    }

    // if the buffer starts with $$PWD accept it
    if (buffer.startsWith(QLatin1String("PWD/")) ||
            buffer.startsWith(QLatin1String("PWD\\"))) {
        if (beginPos > 0 && block.mid(beginPos - 1, 2) == QLatin1String("$$")) {
            beginPos -=2;
            buffer = buffer.mid(4);
        }
    }

    QDir dir(QFileInfo(editorDocument()->fileName()).absolutePath());
    QString fileName = dir.filePath(buffer);
    QFileInfo fi(fileName);
    if (fi.exists()) {
        if (fi.isDir()) {
            QDir subDir(fi.absoluteFilePath());
            QString subProject = subDir.filePath(subDir.dirName() + QLatin1String(".pro"));
            if (QFileInfo(subProject).exists())
                fileName = subProject;
            else
                return link;
        }
        link.fileName = QDir::cleanPath(fileName);
        link.begin = cursor.position() - positionInBlock + beginPos + 1;
        link.end = cursor.position() - positionInBlock + endPos;
    }
    return link;
}

TextEditor::BaseTextEditor *ProFileEditorWidget::createEditor()
{
    return new ProFileEditor(this);
}

void ProFileEditorWidget::contextMenuEvent(QContextMenuEvent *e)
{
    showDefaultContextMenu(e, Constants::M_CONTEXT);
}

void ProFileEditorWidget::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditorWidget::setFontSettings(fs);
    ProFileHighlighter *highlighter = qobject_cast<ProFileHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    static QVector<TextEditor::TextStyle> categories;
    if (categories.isEmpty()) {
        categories << TextEditor::C_TYPE
                   << TextEditor::C_KEYWORD
                   << TextEditor::C_COMMENT
                   << TextEditor::C_VISUAL_WHITESPACE;
    }

    highlighter->setFormats(fs.toTextCharFormats(categories));
    highlighter->rehighlight();
}

//
// ProFileDocument
//

ProFileDocument::ProFileDocument()
        : TextEditor::BaseTextDocument()
{
}

QString ProFileDocument::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString ProFileDocument::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}

} // namespace Internal
} // namespace Qt4ProjectManager
