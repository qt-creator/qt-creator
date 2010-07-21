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

#include "debuggeragents.h"

#include "debuggerengine.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "stackframe.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/basetextdocument.h>

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QMetaObject>

#include <QtGui/QMessageBox>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextCursor>

#include <limits.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// MemoryViewAgent
//
///////////////////////////////////////////////////////////////////////

/*!
    \class MemoryViewAgent

    Objects form this class are created in response to user actions in
    the Gui for showing raw memory from the inferior. After creation
    it handles communication between the engine and the bineditor.
*/

namespace { const int DataRange = 1024 * 1024; }

MemoryViewAgent::MemoryViewAgent(DebuggerEngine *engine, quint64 addr)
    : QObject(engine), m_engine(engine)
{
    QTC_ASSERT(engine, /**/);
    createBinEditor(addr);
}

MemoryViewAgent::MemoryViewAgent(DebuggerEngine *engine, const QString &addr)
    : QObject(engine), m_engine(engine)
{
    QTC_ASSERT(engine, /**/);
    bool ok = true;
    createBinEditor(addr.toULongLong(&ok, 0));
    //qDebug() <<  " ADDRESS: " << addr <<  addr.toUInt(&ok, 0);
}

MemoryViewAgent::~MemoryViewAgent()
{
    foreach (QPointer<Core::IEditor> editor, m_editors) {
        if (editor)
            editor->deleteLater();
    }
}

void MemoryViewAgent::createBinEditor(quint64 addr)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    QString titlePattern = tr("Memory $");
    Core::IEditor *editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_BINARY_EDITOR_ID,
        &titlePattern);
    if (editor) {
        connect(editor->widget(), SIGNAL(lazyDataRequested(Core::IEditor *, quint64,bool)),
            this, SLOT(fetchLazyData(Core::IEditor *, quint64,bool)));
        connect(editor->widget(), SIGNAL(newWindowRequested(quint64)),
            this, SLOT(createBinEditor(quint64)));
        connect(editor->widget(),
            SIGNAL(newRangeRequested(Core::IEditor *, quint64)), this,
            SLOT(provideNewRange(Core::IEditor*,quint64)));
        connect(editor->widget(), SIGNAL(startOfFileRequested(Core::IEditor *)),
            this, SLOT(handleStartOfFileRequested(Core::IEditor*)));
        connect(editor->widget(), SIGNAL(endOfFileRequested(Core::IEditor *)),
            this, SLOT(handleEndOfFileRequested(Core::IEditor*)));
        m_editors << editor;
        editorManager->activateEditor(editor);
        QMetaObject::invokeMethod(editor->widget(), "setNewWindowRequestAllowed");
        QMetaObject::invokeMethod(editor->widget(), "setLazyData",
            Q_ARG(quint64, addr), Q_ARG(int, DataRange), Q_ARG(int, BinBlockSize));
    } else {
        DebuggerEngine::showMessageBox(QMessageBox::Warning,
            tr("No memory viewer available"),
            tr("The memory contents cannot be shown as no viewer plugin "
               "for binary data has been loaded."));
        deleteLater();
    }
}

void MemoryViewAgent::fetchLazyData(Core::IEditor *editor, quint64 block, bool sync)
{
    Q_UNUSED(sync); // FIXME: needed support for incremental searching
    m_engine->fetchMemory(this, editor, BinBlockSize * block, BinBlockSize);
}

void MemoryViewAgent::addLazyData(QObject *editorToken, quint64 addr,
                                  const QByteArray &ba)
{
    Core::IEditor *editor = qobject_cast<Core::IEditor *>(editorToken);
    if (editor && editor->widget()) {
        Core::EditorManager::instance()->activateEditor(editor);
        QMetaObject::invokeMethod(editor->widget(), "addLazyData",
            Q_ARG(quint64, addr / BinBlockSize), Q_ARG(QByteArray, ba));
    }
}

void MemoryViewAgent::provideNewRange(Core::IEditor *editor, quint64 address)
{
    QMetaObject::invokeMethod(editor->widget(), "setLazyData",
        Q_ARG(quint64, address), Q_ARG(int, DataRange),
        Q_ARG(int, BinBlockSize));
}

// Since we are not dealing with files, we take these signals to mean
// "move to start/end of range". This seems to make more sense than
// jumping to the start or end of the address space, respectively.
void MemoryViewAgent::handleStartOfFileRequested(Core::IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, 0));
}

void MemoryViewAgent::handleEndOfFileRequested(Core::IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, DataRange - 1));
}



///////////////////////////////////////////////////////////////////////
//
// DisassemblerViewAgent
//
///////////////////////////////////////////////////////////////////////

// Used for the disassembler view
class LocationMark2 : public TextEditor::ITextMark
{
public:
    LocationMark2() {}

    QIcon icon() const { return DebuggerPlugin::instance()->locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
    void documentClosing() {}
};

struct DisassemblerViewAgentPrivate
{
    DisassemblerViewAgentPrivate();
    void configureMimeType();

    QPointer<TextEditor::ITextEditor> editor;
    StackFrame frame;
    bool tryMixed;
    QPointer<DebuggerEngine> engine;
    LocationMark2 *locationMark;
    QHash<QString, QString> cache;
    QString mimeType;
};

DisassemblerViewAgentPrivate::DisassemblerViewAgentPrivate() :
    editor(0),
    tryMixed(true),
    locationMark(new LocationMark2),
    mimeType(_("text/x-qtcreator-generic-asm"))
{
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
    if (d->editor)
        d->editor->deleteLater();
    d->editor = 0;
    delete d->locationMark;
    d->locationMark = 0;
    delete d;
    d = 0;
}

void DisassemblerViewAgent::cleanup()
{
    d->cache.clear();
    //if (d->editor)
    //    d->editor->deleteLater();
    //d->editor = 0;
}

void DisassemblerViewAgent::resetLocation()
{
    if (d->editor)
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

void DisassemblerViewAgent::setFrame(const StackFrame &frame, bool tryMixed)
{
    d->frame = frame;
    d->tryMixed = tryMixed;
    if (isMixed()) {
        QHash<QString, QString>::ConstIterator it = d->cache.find(frameKey(frame));
        if (it != d->cache.end()) {
            QString msg = _("Use cache disassembler for '%1' in '%2'")
                .arg(frame.function).arg(frame.file);
            d->engine->showMessage(msg);
            setContents(*it);
            return;
        }
    }
    d->engine->fetchDisassembler(this);
}

void DisassemblerViewAgentPrivate::configureMimeType()
{
    QTC_ASSERT(editor, return);

    TextEditor::BaseTextDocument *doc = qobject_cast<TextEditor::BaseTextDocument *>(editor->file());
    QTC_ASSERT(doc, return);
    doc->setMimeType(mimeType);

    TextEditor::PlainTextEditor *pe = qobject_cast<TextEditor::PlainTextEditor *>(editor->widget());
    QTC_ASSERT(pe, return);

    if (const Core::MimeType mtype = Core::ICore::instance()->mimeDatabase()->findByType(mimeType)) {
        pe->configure(mtype);
    } else {
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
    }
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

void DisassemblerViewAgent::setContents(const QString &contents)
{
    QTC_ASSERT(d, return);
    using namespace Core;
    using namespace TextEditor;

    d->cache.insert(frameKey(d->frame), contents);
    QPlainTextEdit *plainTextEdit = 0;
    EditorManager *editorManager = EditorManager::instance();
    if (!d->editor) {
        QString titlePattern = "Disassembler";
        d->editor = qobject_cast<ITextEditor *>(
            editorManager->openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                &titlePattern));
        QTC_ASSERT(d->editor, return);
        d->editor->setProperty("OpenedByDebugger", true);
        d->editor->setProperty("DisassemblerView", true);
        d->configureMimeType();
    }

    editorManager->activateEditor(d->editor);

    plainTextEdit = qobject_cast<QPlainTextEdit *>(d->editor->widget());
    if (plainTextEdit) {
        plainTextEdit->setPlainText(contents);
        plainTextEdit->setReadOnly(true);
    }

    d->editor->markableInterface()->removeMark(d->locationMark);
    d->editor->setDisplayName(_("Disassembler (%1)").arg(d->frame.function));

    for (int pos = 0, line = 0; ; ++line, ++pos) {
        if (contents.midRef(pos, d->frame.address.size()) == d->frame.address) {
            d->editor->markableInterface()->addMark(d->locationMark, line + 1);
            if (plainTextEdit) {
                QTextCursor tc = plainTextEdit->textCursor();
                tc.setPosition(pos);
                plainTextEdit->setTextCursor(tc);
            }
            break;
        }
        pos = contents.indexOf('\n', pos + 1);
        if (pos == -1)
            break;
    }
}

bool DisassemblerViewAgent::contentsCoversAddress(const QString &contents) const
{
    QTC_ASSERT(d, return false);
    for (int pos = 0, line = 0; ; ++line, ++pos) {
        if (contents.midRef(pos, d->frame.address.size()) == d->frame.address)
            return true;
        pos = contents.indexOf('\n', pos + 1);
        if (pos == -1)
            break;
    }
    return false;
}

QString DisassemblerViewAgent::address() const
{
    return d->frame.address;
}

} // namespace Internal
} // namespace Debugger
