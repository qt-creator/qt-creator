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

#include "memoryagent.h"

#include "debuggerengine.h"
#include "debuggercore.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

using namespace Core;

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

MemoryViewAgent::MemoryViewAgent(Debugger::DebuggerEngine *engine, quint64 addr)
    : QObject(engine), m_engine(engine)
{
    QTC_ASSERT(engine, /**/);
    createBinEditor(addr);
}

MemoryViewAgent::~MemoryViewAgent()
{
    EditorManager *editorManager = EditorManager::instance();
    QList<IEditor *> editors;
    foreach (QPointer<IEditor> editor, m_editors)
        if (editor)
            editors.append(editor.data());
    editorManager->closeEditors(editors);
}

void MemoryViewAgent::createBinEditor(quint64 addr)
{
    EditorManager *editorManager = EditorManager::instance();
    QString titlePattern = tr("Memory $");
    IEditor *editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_BINARY_EDITOR_ID,
        &titlePattern);
    if (editor) {
        editor->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        editor->setProperty(Debugger::Constants::OPENED_WITH_MEMORY, true);
        connect(editor->widget(),
            SIGNAL(lazyDataRequested(Core::IEditor *, quint64,bool)),
            SLOT(fetchLazyData(Core::IEditor *, quint64,bool)));
        connect(editor->widget(),
            SIGNAL(newWindowRequested(quint64)),
            SLOT(createBinEditor(quint64)));
        connect(editor->widget(),
            SIGNAL(newRangeRequested(Core::IEditor *, quint64)),
            SLOT(provideNewRange(Core::IEditor*,quint64)));
        connect(editor->widget(),
            SIGNAL(startOfFileRequested(Core::IEditor *)),
            SLOT(handleStartOfFileRequested(Core::IEditor*)));
        connect(editor->widget(),
            SIGNAL(endOfFileRequested(Core::IEditor *)),
            SLOT(handleEndOfFileRequested(Core::IEditor*)));
        m_editors << editor;
        QMetaObject::invokeMethod(editor->widget(), "setNewWindowRequestAllowed");
        QMetaObject::invokeMethod(editor->widget(), "setLazyData",
            Q_ARG(quint64, addr), Q_ARG(int, DataRange), Q_ARG(int, BinBlockSize));
    } else {
        showMessageBox(QMessageBox::Warning,
            tr("No memory viewer available"),
            tr("The memory contents cannot be shown as no viewer plugin "
               "for binary data has been loaded."));
        deleteLater();
    }
}

void MemoryViewAgent::fetchLazyData(IEditor *editor, quint64 block, bool sync)
{
    Q_UNUSED(sync); // FIXME: needed support for incremental searching
    m_engine->fetchMemory(this, editor, BinBlockSize * block, BinBlockSize);
}

void MemoryViewAgent::addLazyData(QObject *editorToken, quint64 addr,
                                  const QByteArray &ba)
{
    IEditor *editor = qobject_cast<IEditor *>(editorToken);
    if (editor && editor->widget()) {
        Core::EditorManager::instance()->activateEditor(editor);
        QMetaObject::invokeMethod(editor->widget(), "addLazyData",
            Q_ARG(quint64, addr / BinBlockSize), Q_ARG(QByteArray, ba));
    }
}

void MemoryViewAgent::provideNewRange(IEditor *editor, quint64 address)
{
    QMetaObject::invokeMethod(editor->widget(), "setLazyData",
        Q_ARG(quint64, address), Q_ARG(int, DataRange),
        Q_ARG(int, BinBlockSize));
}

// Since we are not dealing with files, we take these signals to mean
// "move to start/end of range". This seems to make more sense than
// jumping to the start or end of the address space, respectively.
void MemoryViewAgent::handleStartOfFileRequested(IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, 0));
}

void MemoryViewAgent::handleEndOfFileRequested(IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, DataRange - 1));
}

} // namespace Internal
} // namespace Debugger
