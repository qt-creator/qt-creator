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

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerstartparameters.h"
#include "disassemblerlines.h"
#include "sourceutils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/textmark.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QTextBlock>
#include <QDir>

using namespace Core;
using namespace TextEditor;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// DisassemblerBreakpointMarker
//
///////////////////////////////////////////////////////////////////////

// The red blob on the left side in the cpp editor.
class DisassemblerBreakpointMarker : public TextMark
{
public:
    DisassemblerBreakpointMarker(const Breakpoint &bp, int lineNumber)
        : TextMark(QString(), lineNumber, Constants::TEXT_MARK_CATEGORY_BREAKPOINT), m_bp(bp)
    {
        setIcon(bp.icon());
        setPriority(TextMark::NormalPriority);
    }

    bool isClickable() const { return true; }
    void clicked() { m_bp.removeBreakpoint(); }

public:
    Breakpoint m_bp;
};

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
    DisassemblerAgentPrivate(DebuggerEngine *engine);
    ~DisassemblerAgentPrivate();
    void configureMimeType();
    int lineForAddress(quint64 address) const;

public:
    QPointer<TextDocument> document;
    Location location;
    QPointer<DebuggerEngine> engine;
    LocationMark locationMark;
    QList<DisassemblerBreakpointMarker *> breakpointMarks;
    QList<CacheEntry> cache;
    QString mimeType;
    bool resetLocationScheduled;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate(DebuggerEngine *engine)
  : document(0),
    engine(engine),
    locationMark(engine, QString(), 0),
    mimeType("text/x-qtcreator-generic-asm"),
    resetLocationScheduled(false)
{}

DisassemblerAgentPrivate::~DisassemblerAgentPrivate()
{
    EditorManager::closeDocuments(QList<IDocument *>() << document);
    document = 0;
    qDeleteAll(breakpointMarks);
}

int DisassemblerAgentPrivate::lineForAddress(quint64 address) const
{
    for (int i = 0, n = cache.size(); i != n; ++i) {
        const CacheEntry &entry = cache.at(i);
        if (entry.first.matches(location))
            return entry.second.lineForAddress(address);
    }
    return 0;
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
    : d(new DisassemblerAgentPrivate(engine))
{}

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
    if (!d->document)
        return;
    if (d->resetLocationScheduled) {
        d->resetLocationScheduled = false;
        d->document->removeMark(&d->locationMark);
    }
}

const Location &DisassemblerAgent::location() const
{
    return d->location;
}

void DisassemblerAgent::reload()
{
    d->cache.clear();
    d->engine->fetchDisassembler(this);
}

void DisassemblerAgent::setLocation(const Location &loc)
{
    d->location = loc;
    int index = indexOf(loc);
    if (index != -1) {
        // Refresh when not displaying a function and there is not sufficient
        // context left past the address.
        if (d->cache.at(index).first.endAddress - loc.address() < 24) {
            index = -1;
            d->cache.removeAt(index);
        }
    }
    if (index != -1) {
        const FrameKey &key = d->cache.at(index).first;
        const QString msg =
            QString("Using cached disassembly for 0x%1 (0x%2-0x%3) in \"%4\"/ \"%5\"")
                .arg(loc.address(), 0, 16)
                .arg(key.startAddress, 0, 16).arg(key.endAddress, 0, 16)
                .arg(loc.functionName(), QDir::toNativeSeparators(loc.fileName()));
        d->engine->showMessage(msg);
        setContentsToDocument(d->cache.at(index).second);
        d->resetLocationScheduled = false; // In case reset from previous run still pending.
    } else {
        d->engine->fetchDisassembler(this);
    }
}

void DisassemblerAgentPrivate::configureMimeType()
{
    QTC_ASSERT(document, return);

    document->setMimeType(mimeType);

    Utils::MimeDatabase mdb;
    Utils::MimeType mtype = mdb.mimeTypeForName(mimeType);
    if (mtype.isValid()) {
        foreach (IEditor *editor, DocumentModel::editorsForDocument(document))
            if (TextEditorWidget *widget = qobject_cast<TextEditorWidget *>(editor->widget()))
                widget->configureGenericHighlighter();
    } else {
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
    }
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
    if (d->document)
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
    setContentsToDocument(contents);
}

void DisassemblerAgent::setContentsToDocument(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    if (!d->document) {
        QString titlePattern = QLatin1String("Disassembler");
        IEditor *editor = EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                &titlePattern);
        QTC_ASSERT(editor, return);
        if (TextEditorWidget *widget = qobject_cast<TextEditorWidget *>(editor->widget())) {
            widget->setReadOnly(true);
            widget->setRequestMarkEnabled(true);
        }
        d->document = qobject_cast<TextDocument *>(editor->document());
        QTC_ASSERT(d->document, return);
        d->document->setTemporary(true);
        // FIXME: This is accumulating quite a bit out-of-band data.
        // Make that a proper TextDocument reimplementation.
        d->document->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->document->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->document->setProperty(Debugger::Constants::DISASSEMBLER_SOURCE_FILE, d->location.fileName());
        d->configureMimeType();
    } else {
        EditorManager::activateEditorForDocument(d->document);
    }

    d->document->setPlainText(contents.toString());

    d->document->setPreferredDisplayName(QString("Disassembler (%1)")
        .arg(d->location.functionName()));

    Breakpoints bps = breakHandler()->engineBreakpoints(d->engine);
    foreach (Breakpoint bp, bps)
        updateBreakpointMarker(bp);

    updateLocationMarker();
}

void DisassemblerAgent::updateLocationMarker()
{
    QTC_ASSERT(d->document, return);
    int lineNumber = d->lineForAddress(d->location.address());
    if (d->location.needsMarker()) {
        d->document->removeMark(&d->locationMark);
        d->locationMark.updateLineNumber(lineNumber);
        d->document->addMark(&d->locationMark);
    }

    // Center cursor.
    if (EditorManager::currentDocument() == d->document)
        if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(EditorManager::currentEditor()))
            textEditor->gotoLine(lineNumber);
}

void DisassemblerAgent::removeBreakpointMarker(const Breakpoint &bp)
{
    if (!d->document)
        return;

    BreakpointModelId id = bp.id();
    foreach (DisassemblerBreakpointMarker *marker, d->breakpointMarks) {
        if (marker->m_bp.id() == id) {
            d->breakpointMarks.removeOne(marker);
            d->document->removeMark(marker);
            delete marker;
            return;
        }
    }
}

void DisassemblerAgent::updateBreakpointMarker(const Breakpoint &bp)
{
    removeBreakpointMarker(bp);
    const quint64 address = bp.response().address;
    if (!address)
        return;

    int lineNumber = d->lineForAddress(address);
    if (!lineNumber)
        return;

    // HACK: If it's a FileAndLine breakpoint, and there's a source line
    // above, move the marker up there. That allows setting and removing
    // normal breakpoints from within the disassembler view.
    if (bp.type() == BreakpointByFileAndLine) {
        ContextData context = getLocationContext(d->document, lineNumber - 1);
        if (context.type == LocationByFile)
            --lineNumber;
    }

    auto marker = new DisassemblerBreakpointMarker(bp, lineNumber);
    d->breakpointMarks.append(marker);
    d->document->addMark(marker);
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

} // namespace Internal
} // namespace Debugger
