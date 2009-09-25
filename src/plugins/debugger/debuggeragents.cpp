/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include "debuggerstringutils.h"
#include "idebuggerengine.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/qtcassert.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QTextCursor>
#include <QtGui/QSyntaxHighlighter>

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

MemoryViewAgent::MemoryViewAgent(DebuggerManager *manager, quint64 addr)
    : QObject(manager), m_engine(manager->currentEngine())
{
    init(addr);
}

MemoryViewAgent::MemoryViewAgent(DebuggerManager *manager, const QString &addr)
    : QObject(manager), m_engine(manager->currentEngine())
{
    bool ok = true;
    init(addr.toULongLong(&ok, 0));
    //qDebug() <<  " ADDRESS: " << addr <<  addr.toUInt(&ok, 0);
}

MemoryViewAgent::~MemoryViewAgent()
{
    if (m_editor)
        m_editor->deleteLater();
}

void MemoryViewAgent::init(quint64 addr)
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    QString titlePattern = tr("Memory $");
    m_editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_BINARY_EDITOR,
        &titlePattern);
    connect(m_editor->widget(), SIGNAL(lazyDataRequested(quint64,bool)),
        this, SLOT(fetchLazyData(quint64,bool)));
    editorManager->activateEditor(m_editor);
    QMetaObject::invokeMethod(m_editor->widget(), "setLazyData",
        Q_ARG(quint64, addr), Q_ARG(int, 1024 * 1024), Q_ARG(int, BinBlockSize));
}

void MemoryViewAgent::fetchLazyData(quint64 block, bool sync)
{
    Q_UNUSED(sync); // FIXME: needed support for incremental searching
    if (m_engine)
        m_engine->fetchMemory(this, BinBlockSize * block, BinBlockSize);
}

void MemoryViewAgent::addLazyData(quint64 addr, const QByteArray &ba)
{
    if (m_editor && m_editor->widget())
        QMetaObject::invokeMethod(m_editor->widget(), "addLazyData",
            Q_ARG(quint64, addr / BinBlockSize), Q_ARG(QByteArray, ba));
}


///////////////////////////////////////////////////////////////////////
//
// DisassemblerViewAgent
//
///////////////////////////////////////////////////////////////////////

static QIcon locationMarkIcon()
{
    static const QIcon icon(":/debugger/images/location.svg");
    return icon;
}

// Used for the disassembler view
class LocationMark2 : public TextEditor::ITextMark
{
public:
    LocationMark2() {}

    QIcon icon() const { return locationMarkIcon(); }
    void updateLineNumber(int /*lineNumber*/) {}
    void updateBlock(const QTextBlock & /*block*/) {}
    void removedFromEditor() {}
    void documentClosing() {}
};

struct DisassemblerViewAgentPrivate
{
    QPointer<TextEditor::ITextEditor> editor;
    QString address;
    QString function;
    QPointer<DebuggerManager> manager;
    LocationMark2 *locationMark;
};

/*!
    \class DisassemblerSyntaxHighlighter

     Simple syntax highlighter to make the disassembler text less prominent.
*/

class DisassemblerHighlighter : public QSyntaxHighlighter
{
public:
    DisassemblerHighlighter(QPlainTextEdit *parent)
        : QSyntaxHighlighter(parent->document())
    {}

private:
    void highlightBlock(const QString &text)
    {
        if (!text.isEmpty() && text.at(0) != ' ') {
            QTextCharFormat format;
            format.setForeground(QColor(128, 128, 128));
            setFormat(0, text.size(), format);
        }
    }
};

/*!
    \class DisassemblerViewAgent

     Objects from this class are created in response to user actions in
     the Gui for showing disassembled memory from the inferior. After creation
     it handles communication between the engine and the editor.
*/

DisassemblerViewAgent::DisassemblerViewAgent(DebuggerManager *manager)
    : QObject(manager), d(new DisassemblerViewAgentPrivate)
{
    d->editor = 0;
    d->locationMark = new LocationMark2();
    d->manager = manager;
}

DisassemblerViewAgent::~DisassemblerViewAgent()
{
    if (d->editor)
        d->editor->deleteLater();
    delete d;
}

void DisassemblerViewAgent::setFrame(const StackFrame &frame)
{
    IDebuggerEngine *engine = d->manager->currentEngine();
    QTC_ASSERT(engine, return);
    engine->fetchDisassembler(this, frame);
    d->address = frame.address;
    d->function = frame.function;
}

void DisassemblerViewAgent::setContents(const QString &contents)
{
    using namespace Core;
    using namespace TextEditor;

    if (!d->editor)
        return;

    QPlainTextEdit *plainTextEdit = 0;
    EditorManager *editorManager = EditorManager::instance();
    if (!d->editor) {
        QString titlePattern = "Disassembler";
        d->editor = qobject_cast<ITextEditor *>(
            editorManager->openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR,
                &titlePattern));
        QTC_ASSERT(d->editor, return);
        if ((plainTextEdit = qobject_cast<QPlainTextEdit *>(d->editor->widget())))
            (void) new DisassemblerHighlighter(plainTextEdit);
    }

    editorManager->activateEditor(d->editor);

    plainTextEdit = qobject_cast<QPlainTextEdit *>(d->editor->widget());
    if (plainTextEdit)
        plainTextEdit->setPlainText(contents);

    d->editor->markableInterface()->removeMark(d->locationMark);
    d->editor->setDisplayName(_("Disassembler (%1)").arg(d->function));

    for (int pos = 0, line = 0; ; ++line, ++pos) {
        if (contents.midRef(pos, d->address.size()) == d->address) {
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

QString DisassemblerViewAgent::address() const
{
    return d->address;
}

} // namespace Internal
} // namespace Debugger
