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

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
#include "disassemblerlines.h"
#include "stackframe.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <projectexplorer/abi.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include <QTextBlock>
#include <QIcon>
#include <QPointer>
#include <QPair>
#include <QDir>

using namespace Core;
using namespace TextEditor;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// FrameKey
//
///////////////////////////////////////////////////////////////////////

class FrameKey
{
public:
    FrameKey() : startAddress(0), endAddress(0) {}
    inline bool matches(const Location &loc) const;

    QString functionName;
    QString fileName;
    quint64 startAddress;
    quint64 endAddress;
};

bool FrameKey::matches(const Location &loc) const
{
    return loc.address() >= startAddress
            && loc.address() <= endAddress
            && loc.fileName() == fileName
            && loc.functionName() == functionName;
}

typedef QPair<FrameKey, DisassemblerLines> CacheEntry;


///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgentPrivate
//
///////////////////////////////////////////////////////////////////////

class DisassemblerAgentPrivate
{
public:
    DisassemblerAgentPrivate();
    ~DisassemblerAgentPrivate();
    void configureMimeType();
    DisassemblerLines contentsAtCurrentLocation() const;

public:
    QPointer<TextEditor::ITextEditor> editor;
    Location location;
    QPointer<DebuggerEngine> engine;
    ITextMark *locationMark;
    QList<ITextMark *> breakpointMarks;
    QList<CacheEntry> cache;
    QString mimeType;
    bool tryMixedInitialized;
    bool tryMixed;
    bool resetLocationScheduled;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate()
  : editor(0),
    locationMark(0),
    mimeType(_("text/x-qtcreator-generic-asm")),
    tryMixedInitialized(false),
    tryMixed(true),
    resetLocationScheduled(false)
{

}

DisassemblerAgentPrivate::~DisassemblerAgentPrivate()
{
    if (editor) {
        EditorManager *editorManager = EditorManager::instance();
        editorManager->closeEditors(QList<IEditor *>() << editor);
    }
    editor = 0;
    delete locationMark;
    qDeleteAll(breakpointMarks);
}

DisassemblerLines DisassemblerAgentPrivate::contentsAtCurrentLocation() const
{
    for (int i = 0, n = cache.size(); i != n; ++i) {
        const CacheEntry &entry = cache.at(i);
        if (entry.first.matches(location))
            return entry.second;
    }
    return DisassemblerLines();
}


///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgent
//
///////////////////////////////////////////////////////////////////////

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

int DisassemblerAgent::indexOf(const Location &loc) const
{
    for (int i = 0; i < d->cache.size(); i++)
        if (d->cache.at(i).first.matches(loc))
            return i;
    return -1;
}

void DisassemblerAgent::cleanup()
{
    d->cache.clear();
}

void DisassemblerAgent::scheduleResetLocation()
{
    d->resetLocationScheduled = true;
}

void DisassemblerAgent::resetLocation()
{
    if (!d->editor)
        return;
    if (d->resetLocationScheduled) {
        d->resetLocationScheduled = false;
        if (d->locationMark)
            d->editor->markableInterface()->removeMark(d->locationMark);
    }
}

const Location &DisassemblerAgent::location() const
{
    return d->location;
}

bool DisassemblerAgent::isMixed() const
{
    if (!d->tryMixedInitialized) {
        if (d->engine->startParameters().toolChainAbi.os() == ProjectExplorer::Abi::MacOS)
           d->tryMixed = false;
        d->tryMixedInitialized = true;
    }

    return d->tryMixed
        && d->location.lineNumber() > 0
        && !d->location.functionName().isEmpty()
        && d->location.functionName() != _("??");
}

void DisassemblerAgent::setLocation(const Location &loc)
{
    d->location = loc;
    int index = indexOf(loc);
    if (index != -1) {
        // Refresh when not displaying a function and there is not sufficient
        // context left past the address.
        if (!isMixed() && d->cache.at(index).first.endAddress - loc.address() < 24) {
            index = -1;
            d->cache.removeAt(index);
        }
    }
    if (index != -1) {
        const FrameKey &key = d->cache.at(index).first;
        const QString msg =
            _("Using cached disassembly for 0x%1 (0x%2-0x%3) in '%4'/ '%5'")
                .arg(loc.address(), 0, 16)
                .arg(key.startAddress, 0, 16).arg(key.endAddress, 0, 16)
                .arg(loc.functionName(), QDir::toNativeSeparators(loc.fileName()));
        d->engine->showMessage(msg);
        setContentsToEditor(d->cache.at(index).second);
        d->resetLocationScheduled = false; // In case reset from previous run still pending.
    } else {
        d->engine->fetchDisassembler(this);
    }
}

void DisassemblerAgentPrivate::configureMimeType()
{
    QTC_ASSERT(editor, return);

    TextEditor::BaseTextDocument *doc =
        qobject_cast<TextEditor::BaseTextDocument *>(editor->document());
    QTC_ASSERT(doc, return);
    doc->setMimeType(mimeType);

    TextEditor::PlainTextEditorWidget *pe =
        qobject_cast<TextEditor::PlainTextEditorWidget *>(editor->widget());
    QTC_ASSERT(pe, return);

    MimeType mtype = ICore::mimeDatabase()->findByType(mimeType);
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
    if (contents.size()) {
        const quint64 startAddress = contents.startAddress();
        const quint64 endAddress = contents.endAddress();
        if (startAddress) {
            FrameKey key;
            key.fileName = d->location.fileName();
            key.functionName = d->location.functionName();
            key.startAddress = startAddress;
            key.endAddress = endAddress;
            d->cache.append(CacheEntry(key, contents));
        }
    }
    setContentsToEditor(contents);
}

void DisassemblerAgent::setContentsToEditor(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    using namespace Core;
    using namespace TextEditor;

    if (!d->editor) {
        QString titlePattern = QLatin1String("Disassembler");
        d->editor = qobject_cast<ITextEditor *>(
            EditorManager::openEditorWithContents(
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

    EditorManager::activateEditor(d->editor);

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

    d->editor->setDisplayName(_("Disassembler (%1)")
        .arg(d->location.functionName()));

    updateBreakpointMarkers();
    updateLocationMarker();
}

void DisassemblerAgent::updateLocationMarker()
{
    QTC_ASSERT(d->editor, return);
    const DisassemblerLines contents = d->contentsAtCurrentLocation();
    int lineNumber = contents.lineForAddress(d->location.address());
    if (d->location.needsMarker()) {
        if (d->locationMark)
            d->editor->markableInterface()->removeMark(d->locationMark);
        delete d->locationMark;
        d->locationMark = 0;
        if (lineNumber) {
            d->locationMark = new ITextMark(lineNumber);
            d->locationMark->setIcon(debuggerCore()->locationMarkIcon());
            d->locationMark->setPriority(TextEditor::ITextMark::HighPriority);
            d->editor->markableInterface()->addMark(d->locationMark);
        }
    }

    QPlainTextEdit *plainTextEdit =
        qobject_cast<QPlainTextEdit *>(d->editor->widget());
    QTC_ASSERT(plainTextEdit, return);
    QTextCursor tc = plainTextEdit->textCursor();
    QTextBlock block = tc.document()->findBlockByNumber(lineNumber - 1);
    tc.setPosition(block.position());
    plainTextEdit->setTextCursor(tc);
    plainTextEdit->centerCursor();
}

void DisassemblerAgent::updateBreakpointMarkers()
{
    if (!d->editor)
        return;

    BreakHandler *handler = breakHandler();
    BreakpointModelIds ids = handler->engineBreakpointIds(d->engine);
    if (ids.isEmpty())
        return;

    const DisassemblerLines contents = d->contentsAtCurrentLocation();
    foreach (TextEditor::ITextMark *marker, d->breakpointMarks)
        d->editor->markableInterface()->removeMark(marker);
    qDeleteAll(d->breakpointMarks);
    d->breakpointMarks.clear();
    foreach (BreakpointModelId id, ids) {
        const quint64 address = handler->response(id).address;
        if (!address)
            continue;
        const int lineNumber = contents.lineForAddress(address);
        if (!lineNumber)
            continue;
        ITextMark *marker = new ITextMark(lineNumber);
        marker->setIcon(handler->icon(id));
        marker->setPriority(ITextMark::NormalPriority);
        d->breakpointMarks.append(marker);
        d->editor->markableInterface()->addMark(marker);
    }
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

} // namespace Internal
} // namespace Debugger
