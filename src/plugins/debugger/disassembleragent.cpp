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

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
#include "disassemblerlines.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>

#include <texteditor/basetextdocument.h>
#include <texteditor/plaintexteditor.h>

#include <utils/qtcassert.h>

#include <QTextBlock>
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
    QPointer<BaseTextDocument> document;
    Location location;
    QPointer<DebuggerEngine> engine;
    ITextMark locationMark;
    QList<ITextMark *> breakpointMarks;
    QList<CacheEntry> cache;
    QString mimeType;
    bool tryMixedInitialized;
    bool tryMixed;
    bool resetLocationScheduled;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate()
  : document(0),
    locationMark(0),
    mimeType(_("text/x-qtcreator-generic-asm")),
    tryMixedInitialized(false),
    tryMixed(true),
    resetLocationScheduled(false)
{
    locationMark.setIcon(debuggerCore()->locationMarkIcon());
    locationMark.setPriority(TextEditor::ITextMark::HighPriority);
}

DisassemblerAgentPrivate::~DisassemblerAgentPrivate()
{
    EditorManager::closeDocuments(QList<IDocument *>() << document);
    document = 0;
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
    if (!d->document)
        return;
    if (d->resetLocationScheduled) {
        d->resetLocationScheduled = false;
        d->document->markableInterface()->removeMark(&d->locationMark);
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
            _("Using cached disassembly for 0x%1 (0x%2-0x%3) in \"%4\"/ \"%5\"")
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

    MimeType mtype = MimeDatabase::findByType(mimeType);
    if (mtype) {
        foreach (IEditor *editor, DocumentModel::editorsForDocument(document))
            if (PlainTextEditorWidget *widget = qobject_cast<PlainTextEditorWidget *>(editor->widget()))
                widget->configure(mtype);
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
        if (BaseTextEditorWidget *widget = qobject_cast<BaseTextEditorWidget *>(editor->widget())) {
            widget->setReadOnly(true);
            widget->setRequestMarkEnabled(true);
        }
        d->document = qobject_cast<BaseTextDocument *>(editor->document());
        QTC_ASSERT(d->document, return);
        d->document->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->document->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->configureMimeType();
    }

    d->document->setPlainText(contents.toString());

    d->document->setDisplayName(_("Disassembler (%1)")
        .arg(d->location.functionName()));

    updateBreakpointMarkers();
    updateLocationMarker();
}

void DisassemblerAgent::updateLocationMarker()
{
    QTC_ASSERT(d->document, return);
    const DisassemblerLines contents = d->contentsAtCurrentLocation();
    int lineNumber = contents.lineForAddress(d->location.address());
    if (d->location.needsMarker()) {
        d->document->markableInterface()->removeMark(&d->locationMark);
        d->locationMark.updateLineNumber(lineNumber);
        d->document->markableInterface()->addMark(&d->locationMark);
    }

    // Center cursor.
    if (EditorManager::currentDocument() == d->document)
        if (BaseTextEditor *textEditor = qobject_cast<BaseTextEditor *>(EditorManager::currentEditor()))
            textEditor->gotoLine(lineNumber);
}

void DisassemblerAgent::updateBreakpointMarkers()
{
    if (!d->document)
        return;

    BreakHandler *handler = breakHandler();
    BreakpointModelIds ids = handler->engineBreakpointIds(d->engine);
    if (ids.isEmpty())
        return;

    const DisassemblerLines contents = d->contentsAtCurrentLocation();
    foreach (TextEditor::ITextMark *marker, d->breakpointMarks)
        d->document->markableInterface()->removeMark(marker);
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
        d->document->markableInterface()->addMark(marker);
    }
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

} // namespace Internal
} // namespace Debugger
