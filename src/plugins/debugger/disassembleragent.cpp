/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "disassembleragent.h"

#include "disassemblerlines.h"
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
#include <QtCore/QPointer>

using namespace Core;
using namespace TextEditor;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgent
//
///////////////////////////////////////////////////////////////////////

class DisassemblerAgentPrivate
{
public:
    DisassemblerAgentPrivate();
    ~DisassemblerAgentPrivate();
    void configureMimeType();

public:
    QPointer<TextEditor::ITextEditor> editor;
    Location location;
    bool tryMixed;
    QPointer<DebuggerEngine> engine;
    ITextMark *locationMark;
    QList<ITextMark *> breakpointMarks;
    
    QHash<QString, DisassemblerLines> cache;
    QString mimeType;
    bool m_resetLocationScheduled;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate()
  : editor(0),
    tryMixed(true),
    mimeType(_("text/x-qtcreator-generic-asm")),
    m_resetLocationScheduled(false)
{
    locationMark = new ITextMark;
    locationMark->setIcon(debuggerCore()->locationMarkIcon());
    locationMark->setPriority(TextEditor::ITextMark::HighPriority);
}

DisassemblerAgentPrivate::~DisassemblerAgentPrivate()
{
    if (editor) {
        EditorManager *editorManager = EditorManager::instance();
        editorManager->closeEditors(QList<IEditor *>() << editor);
    }
    editor = 0;
    delete locationMark;
}

/*!
    \class Debugger::Internal::DisassemblerAgent

     Objects from this class are created in response to user actions in
     the Gui for showing disassembled memory from the inferior. After creation
     it handles communication between the engine and the editor.
*/

DisassemblerAgent::DisassemblerAgent(DebuggerEngine *engine)
    : QObject(0), d(new DisassemblerAgentPrivate)
{
    d->engine = engine;
}

DisassemblerAgent::~DisassemblerAgent()
{
    delete d;
    d = 0;
}

void DisassemblerAgent::cleanup()
{
    d->cache.clear();
}

void DisassemblerAgent::scheduleResetLocation()
{
    d->m_resetLocationScheduled = true;
}

void DisassemblerAgent::resetLocation()
{
    if (!d->editor)
        return;
    if (d->m_resetLocationScheduled) {
        d->m_resetLocationScheduled = false;
        d->editor->markableInterface()->removeMark(d->locationMark);
    }
}

static QString frameKey(const Location &loc)
{
    return _("%1:%2:%3").arg(loc.functionName())
        .arg(loc.fileName()).arg(loc.address());
}

const Location &DisassemblerAgent::location() const
{
    return d->location;
}

bool DisassemblerAgent::isMixed() const
{
    return d->tryMixed
        && d->location.lineNumber() > 0
        && !d->location.functionName().isEmpty()
        && d->location.functionName() != _("??");
}

void DisassemblerAgent::setLocation(const Location &loc)
{
    d->location = loc;
    if (isMixed()) {
        QHash<QString, DisassemblerLines>::ConstIterator it =
            d->cache.find(frameKey(loc));
        if (it != d->cache.end()) {
            QString msg = _("Use cache disassembler for '%1' in '%2'")
                .arg(loc.functionName()).arg(loc.fileName());
            d->engine->showMessage(msg);
            setContents(*it);
            updateBreakpointMarkers();
            updateLocationMarker();
            return;
        }
    }
    d->engine->fetchDisassembler(this);
}

void DisassemblerAgentPrivate::configureMimeType()
{
    QTC_ASSERT(editor, return);

    TextEditor::BaseTextDocument *doc =
        qobject_cast<TextEditor::BaseTextDocument *>(editor->file());
    QTC_ASSERT(doc, return);
    doc->setMimeType(mimeType);

    TextEditor::PlainTextEditorWidget *pe =
        qobject_cast<TextEditor::PlainTextEditorWidget *>(editor->widget());
    QTC_ASSERT(pe, return);

    MimeType mtype = ICore::instance()->mimeDatabase()->findByType(mimeType);
    if (mtype)
        pe->configure(mtype);
    else
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
}

QString DisassemblerAgent::mimeType() const
{
    return d->mimeType;
}

void DisassemblerAgent::setMimeType(const QString &mt)
{
    if (mt == d->mimeType)
        return;
    d->mimeType = mt;
    if (d->editor)
       d->configureMimeType();
}

void DisassemblerAgent::setContents(const DisassemblerLines &contents)
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

        BaseTextEditorWidget *baseTextEdit =
                qobject_cast<BaseTextEditorWidget *>(d->editor->widget());
        if (baseTextEdit)
            baseTextEdit->setRequestMarkEnabled(true);
    }

    editorManager->activateEditor(d->editor);

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return);

    QString str;
    for (int i = 0, n = contents.size(); i != n; ++i) {
        str += contents.at(i).toString();
        str += QLatin1Char('\n');
    }
    plainTextEdit->setPlainText(str);
    plainTextEdit->setReadOnly(true);

    d->cache.insert(frameKey(d->location), contents);
    d->editor->setDisplayName(_("Disassembler (%1)")
        .arg(d->location.functionName()));

    updateBreakpointMarkers();
    updateLocationMarker();
}

void DisassemblerAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);

    const DisassemblerLines &contents = d->cache.value(frameKey(d->location));
    int lineNumber = contents.lineForAddress(d->location.address());

    if (d->location.needsMarker()) {
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

void DisassemblerAgent::updateBreakpointMarkers()
{
    if (!d->editor)
        return;

    BreakHandler *handler = breakHandler();
    BreakpointIds ids = handler->engineBreakpointIds(d->engine);
    if (ids.isEmpty())
        return;

    const DisassemblerLines &contents = d->cache.value(frameKey(d->location));

    foreach (TextEditor::ITextMark *marker, d->breakpointMarks)
        d->editor->markableInterface()->removeMark(marker);
    d->breakpointMarks.clear();
    foreach (BreakpointId id, ids) {
        const quint64 address = handler->response(id).address;
        if (!address)
            continue;
        const int lineNumber = contents.lineForAddress(address);
        if (!lineNumber)
            continue;
        ITextMark *marker = new ITextMark;
        marker->setIcon(handler->icon(id));
        marker->setPriority(ITextMark::NormalPriority);
        d->breakpointMarks.append(marker);
        d->editor->markableInterface()->addMark(marker, lineNumber);
    }
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

void DisassemblerAgent::setTryMixed(bool on)
{
    d->tryMixed = on;
}

} // namespace Internal
} // namespace Debugger
