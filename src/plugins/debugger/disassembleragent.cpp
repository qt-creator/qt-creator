/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggercore.h"
#include "debuggerstringutils.h"
#include "stackframe.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include <QtGui/QTextBlock>
#include <QtGui/QIcon>


using namespace Core;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// DisassemblerViewAgent
//
///////////////////////////////////////////////////////////////////////

class LocationMark2 : public TextEditor::ITextMark
{
public:
    LocationMark2() {}

    QIcon icon() const { return debuggerCore()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
    void documentClosing() {}
};

class BreakpointMarker2 : public TextEditor::ITextMark
{
public:
    BreakpointMarker2(const QIcon &icon) : m_icon(icon) {}

    QIcon icon() const { return m_icon; }
    void updateLineNumber(int) {}
    void updateBlock(const QTextBlock &) {}
    void removedFromEditor() {}
    void documentClosing() {}

private:
    QIcon m_icon;
};


class DisassemblerViewAgentPrivate
{
public:
    DisassemblerViewAgentPrivate();
    ~DisassemblerViewAgentPrivate();
    void configureMimeType();

public:
    QPointer<TextEditor::ITextEditor> editor;
    StackFrame frame;
    bool tryMixed;
    bool setMarker;
    QPointer<DebuggerEngine> engine;
    TextEditor::ITextMark *locationMark;
    QList<TextEditor::ITextMark *> breakpointMarks;
    
    QHash<QString, DisassemblerLines> cache;
    QString mimeType;
};

DisassemblerViewAgentPrivate::DisassemblerViewAgentPrivate()
  : editor(0),
    tryMixed(true),
    setMarker(true),
    locationMark(new LocationMark2),
    mimeType(_("text/x-qtcreator-generic-asm"))
{
}

DisassemblerViewAgentPrivate::~DisassemblerViewAgentPrivate()
{
    if (editor) {
        EditorManager *editorManager = EditorManager::instance();
        editorManager->closeEditors(QList<IEditor *>() << editor);
    }
    editor = 0;
    delete locationMark;
}

/*!
    \class DisassemblerViewAgent

     Objects from this class are created in response to user actions in
     the Gui for showing disassembled memory from the inferior. After creation
     it handles communication between the engine and the editor.
*/

DisassemblerViewAgent::DisassemblerViewAgent(DebuggerEngine *engine)
    : QObject(0), d(new DisassemblerViewAgentPrivate)
{
    d->engine = engine;
}

DisassemblerViewAgent::~DisassemblerViewAgent()
{
    delete d;
    d = 0;
}

void DisassemblerViewAgent::cleanup()
{
    d->cache.clear();
}

void DisassemblerViewAgent::resetLocation()
{
    if (!d->editor)
        return;
    d->editor->markableInterface()->removeMark(d->locationMark);
}

QString frameKey(const StackFrame &frame)
{
    return _("%1:%2:%3").arg(frame.function).arg(frame.file).arg(frame.from);
}

const StackFrame &DisassemblerViewAgent::frame() const
{
    return d->frame;
}

bool DisassemblerViewAgent::isMixed() const
{
    return d->tryMixed
        && d->frame.line > 0
        && !d->frame.function.isEmpty()
        && d->frame.function != _("??");
}

void DisassemblerViewAgent::setFrame(const StackFrame &frame,
    bool tryMixed, bool setMarker)
{
    d->frame = frame;
    d->tryMixed = tryMixed;
    d->setMarker = setMarker;
    if (isMixed()) {
        QHash<QString, DisassemblerLines>::ConstIterator it =
            d->cache.find(frameKey(frame));
        if (it != d->cache.end()) {
            QString msg = _("Use cache disassembler for '%1' in '%2'")
                .arg(frame.function).arg(frame.file);
            d->engine->showMessage(msg);
            setContents(*it);
            updateBreakpointMarkers();
            updateLocationMarker();
            return;
        }
    }
    d->engine->fetchDisassembler(this);
}

void DisassemblerViewAgentPrivate::configureMimeType()
{
    QTC_ASSERT(editor, return);

    TextEditor::BaseTextDocument *doc =
        qobject_cast<TextEditor::BaseTextDocument *>(editor->file());
    QTC_ASSERT(doc, return);
    doc->setMimeType(mimeType);

    TextEditor::PlainTextEditor *pe =
        qobject_cast<TextEditor::PlainTextEditor *>(editor->widget());
    QTC_ASSERT(pe, return);

    MimeType mtype = ICore::instance()->mimeDatabase()->findByType(mimeType);
    if (mtype)
        pe->configure(mtype);
    else
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
}

QString DisassemblerViewAgent::mimeType() const
{
    return d->mimeType;
}

void DisassemblerViewAgent::setMimeType(const QString &mt)
{
    if (mt == d->mimeType)
        return;
    d->mimeType = mt;
    if (d->editor)
       d->configureMimeType();
}

void DisassemblerViewAgent::setContents(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    using namespace Core;
    using namespace TextEditor;

    EditorManager *editorManager = EditorManager::instance();
    if (!d->editor) {
        QString titlePattern = "Disassembler";
        d->editor = qobject_cast<ITextEditor *>(
            editorManager->openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                &titlePattern));
        QTC_ASSERT(d->editor, return);
        d->editor->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->editor->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->configureMimeType();

        BaseTextEditor *baseTextEdit =
                qobject_cast<BaseTextEditor *>(d->editor->widget());
        if (baseTextEdit)
            baseTextEdit->setRequestMarkEnabled(true);
    }

    editorManager->activateEditor(d->editor);

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return);

    QString str;
    for (int i = 0, n = contents.size(); i != n; ++i) {
        const DisassemblerLine &dl = contents.at(i);
        if (dl.address) {
            str += QString("0x");
            str += QString::number(dl.address, 16);
            str += "  ";
        }
        str += dl.data;
        str += "\n";
    }
    plainTextEdit->setPlainText(str);
    plainTextEdit->setReadOnly(true);

    d->cache.insert(frameKey(d->frame), contents);
    d->editor->setDisplayName(_("Disassembler (%1)").arg(d->frame.function));

    updateBreakpointMarkers();
    updateLocationMarker();
}

void DisassemblerViewAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);

    const DisassemblerLines &contents = d->cache.value(frameKey(d->frame));
    int lineNumber = contents.lineForAddress(d->frame.address);

    if (d->setMarker) {
        d->editor->markableInterface()->removeMark(d->locationMark);
        if (lineNumber)
            d->editor->markableInterface()->addMark(d->locationMark, lineNumber);
    }

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return); 
    QTextCursor tc = plainTextEdit->textCursor();
    QTextBlock block = tc.document()->findBlockByNumber(lineNumber - 1);
    tc.setPosition(block.position());
    plainTextEdit->setTextCursor(tc);
}

void DisassemblerViewAgent::updateBreakpointMarkers()
{
    if (!d->editor)
        return;

    BreakHandler *handler = breakHandler();
    BreakpointIds ids = handler->engineBreakpointIds(d->engine);
    if (ids.isEmpty())
        return;

    const DisassemblerLines &contents = d->cache.value(frameKey(d->frame));

    foreach (TextEditor::ITextMark *marker, d->breakpointMarks)
        d->editor->markableInterface()->removeMark(marker);
    d->breakpointMarks.clear();
    foreach (BreakpointId id, ids) {
        const quint64 address = handler->address(id);
        if (!address)
            continue;
        const int lineNumber = contents.lineForAddress(address);
        if (!lineNumber)
            continue;
        BreakpointMarker2 *marker = new BreakpointMarker2(handler->icon(id));
        d->breakpointMarks.append(marker);
        d->editor->markableInterface()->addMark(marker, lineNumber);
    }
}

quint64 DisassemblerViewAgent::address() const
{
    return d->frame.address;
}

// Return address of an assembly line "0x0dfd  bla"
quint64 DisassemblerViewAgent::addressFromDisassemblyLine(const QString &line)
{
    return DisassemblerLine(line).address;
}

} // namespace Internal
} // namespace Debugger
