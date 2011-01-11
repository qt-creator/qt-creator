/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profileeditor.h"

#include "profilehighlighter.h"
#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "profileeditorfactory.h"
#include "addlibrarywizard.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QMenu>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

//
// ProFileEditorEditable
//

ProFileEditorEditable::ProFileEditorEditable(ProFileEditor *editor)
  : BaseTextEditorEditable(editor),
    m_context(Qt4ProjectManager::Constants::C_PROFILEEDITOR,
              TextEditor::Constants::C_TEXTEDITOR)
{
//    m_contexts << uidm->uniqueIdentifier(Qt4ProjectManager::Constants::PROJECT_KIND);
}

Core::Context ProFileEditorEditable::context() const
{
    return m_context;
}

Core::IEditor *ProFileEditorEditable::duplicate(QWidget *parent)
{
    ProFileEditor *ret = new ProFileEditor(parent, qobject_cast<ProFileEditor*>(editor())->factory(),
                                           qobject_cast<ProFileEditor*>(editor())->actionHandler());
    ret->duplicateFrom(editor());
    TextEditor::TextEditorSettings::instance()->initializeEditor(ret);
    return ret->editableInterface();
}

QString ProFileEditorEditable::id() const
{
    return QLatin1String(Qt4ProjectManager::Constants::PROFILE_EDITOR_ID);
}

//
// ProFileEditorEditor
//

ProFileEditor::ProFileEditor(QWidget *parent, ProFileEditorFactory *factory, TextEditor::TextEditorActionHandler *ah)
    : BaseTextEditor(parent), m_factory(factory), m_ah(ah)
{
    ProFileDocument *doc = new ProFileDocument();
    doc->setMimeType(QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE));
    setBaseTextDocument(doc);

    ah->setupActions(this);

    baseTextDocument()->setSyntaxHighlighter(new ProFileHighlighter);
    m_commentDefinition.clearCommentStyles();
    m_commentDefinition.setSingleLine(QString(QLatin1Char('#')));
}

ProFileEditor::~ProFileEditor()
{
}

void ProFileEditor::unCommentSelection()
{
    Utils::unCommentSelection(this, m_commentDefinition);
}

static bool isValidFileNameChar(const QChar &c)
{
    if (c.isLetterOrNumber()
            || c == QLatin1Char('.')
            || c == QLatin1Char('_')
            || c == QLatin1Char('-')
            || c == QLatin1Char('/')
            || c == QLatin1Char('\\'))
        return true;
    return false;
}

ProFileEditor::Link ProFileEditor::findLinkAt(const QTextCursor &cursor,
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

    QDir dir(QFileInfo(file()->fileName()).absolutePath());
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
        link.fileName = fileName;
        link.begin = cursor.position() - positionInBlock + beginPos + 1;
        link.end = cursor.position() - positionInBlock + endPos;
    }
    return link;
}

TextEditor::BaseTextEditorEditable *ProFileEditor::createEditableInterface()
{
    return new ProFileEditorEditable(this);
}

void ProFileEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = new QMenu();

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    Core::ActionContainer *mcontext = am->actionContainer(Qt4ProjectManager::Constants::M_CONTEXT);
    QMenu *contextMenu = mcontext->menu();

    foreach (QAction *action, contextMenu->actions())
        menu->addAction(action);

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    delete menu;
}

void ProFileEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    ProFileHighlighter *highlighter = qobject_cast<ProFileHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_TYPE)
                   << QLatin1String(TextEditor::Constants::C_KEYWORD)
                   << QLatin1String(TextEditor::Constants::C_COMMENT)
                   << QLatin1String(TextEditor::Constants::C_VISUAL_WHITESPACE);
    }

    const QVector<QTextCharFormat> formats = fs.toTextCharFormats(categories);
    highlighter->setFormats(formats.constBegin(), formats.constEnd());
    highlighter->rehighlight();
}

void ProFileEditor::addLibrary()
{
    AddLibraryWizard wizard(file()->fileName(), this);
    if (wizard.exec() != QDialog::Accepted)
        return;

    TextEditor::BaseTextEditorEditable *editable = editableInterface();
    const int endOfDoc = editable->position(TextEditor::ITextEditor::EndOfDoc);
    editable->setCurPos(endOfDoc);
    QString snippet = wizard.snippet();

    // add extra \n in case the last line is not empty
    int line, column;
    editable->convertPosition(endOfDoc, &line, &column);
    if (!editable->textAt(endOfDoc - column, column).simplified().isEmpty())
        snippet = QLatin1Char('\n') + snippet;

    editable->insert(snippet);
}

void ProFileEditor::jumpToFile()
{
    openLink(findLinkAt(textCursor()));
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
