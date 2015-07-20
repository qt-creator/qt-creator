/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerstartparameters.h"
#include "debuggerstringutils.h"
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
    DisassemblerLines contentsAtCurrentLocation() const;

public:
    QPointer<TextDocument> document;
    Location location;
    QPointer<DebuggerEngine> engine;
    LocationMark locationMark;
    QList<TextMark *> breakpointMarks;
    QList<CacheEntry> cache;
    QString mimeType;
    bool resetLocationScheduled;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate(DebuggerEngine *engine)
  : document(0),
    engine(engine),
    locationMark(engine, QString(), 0),
    mimeType(_("text/x-qtcreator-generic-asm")),
    resetLocationScheduled(false)
{}

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
        // FIXME: This is accumulating quite a bit out-of-band data.
        // Make that a proper TextDocument reimplementation.
        d->document = qobject_cast<TextDocument *>(editor->document());
        QTC_ASSERT(d->document, return);
        d->document->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->document->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->document->setProperty(Debugger::Constants::DISASSEMBLER_SOURCE_FILE, d->location.fileName());
        d->configureMimeType();
    } else {
        EditorManager::activateEditorForDocument(d->document);
    }

    d->document->setPlainText(contents.toString());

    d->document->setPreferredDisplayName(_("Disassembler (%1)")
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
        d->document->removeMark(&d->locationMark);
        d->locationMark.updateLineNumber(lineNumber);
        d->document->addMark(&d->locationMark);
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

    Breakpoints bps = breakHandler()->engineBreakpoints(d->engine);
    if (bps.isEmpty())
        return;

    const DisassemblerLines contents = d->contentsAtCurrentLocation();
    foreach (TextMark *marker, d->breakpointMarks)
        d->document->removeMark(marker);
    qDeleteAll(d->breakpointMarks);
    d->breakpointMarks.clear();
    foreach (Breakpoint bp, bps) {
        const quint64 address = bp.response().address;
        if (!address)
            continue;
        int lineNumber = contents.lineForAddress(address);
        if (!lineNumber)
            continue;

        // HACK: If it's a FileAndLine breakpoint, and there's a source line
        // above, move the marker up there. That allows setting and removing
        // normal breakpoints from within the disassembler view.
        if (bp.type() == BreakpointByFileAndLine) {
            ContextData context = getLocationContext(d->document, lineNumber - 1);
            if (context.type == LocationByFile)
                --lineNumber;
        }

        TextMark *marker = new TextMark(QString(), lineNumber,
                                        Constants::TEXT_MARK_CATEGORY_BREAKPOINT);
        marker->setIcon(bp.icon());
        marker->setPriority(TextMark::NormalPriority);
        d->breakpointMarks.append(marker);
        d->document->addMark(marker);
    }
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

} // namespace Internal
} // namespace Debugger
