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

#include "memoryagent.h"
#include "memoryview.h"

#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggerstartparameters.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagebox.h>

#include <utils/qtcassert.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>

#include <cstring>

using namespace Core;

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// MemoryAgent
//
///////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::MemoryAgent

    Objects form this class are created in response to user actions in
    the Gui for showing raw memory from the inferior. After creation
    it handles communication between the engine and the bineditor.

    Memory can be shown as
    \list
    \li Editor: Create an IEditor using the normal editor factory
       interface (m_editors)
    \li View: Separate top-level view consisting of a Bin Editor widget
       (m_view).
    \endlist

    Views are asked to update themselves directly by the owning
    DebuggerEngine.
    An exception are views of class Debugger::RegisterMemoryView tracking the
    content pointed to by a register (eg stack pointer, instruction pointer).
    They are connected to the set/changed signals of
    the engine's register handler.

    \sa Debugger::MemoryView,  Debugger::RegisterMemoryView
*/

MemoryAgent::MemoryAgent(DebuggerEngine *engine)
    : QObject(engine), m_engine(engine)
{
    QTC_CHECK(engine);
    connect(engine, &DebuggerEngine::stackFrameCompleted,
            this, &MemoryAgent::updateContents);
}

MemoryAgent::~MemoryAgent()
{
    closeEditors();
    closeViews();
}

void MemoryAgent::closeEditors()
{
    if (m_editors.isEmpty())
        return;

    QSet<IDocument *> documents;
    foreach (QPointer<IEditor> editor, m_editors)
        if (editor)
            documents.insert(editor->document());
    EditorManager::closeDocuments(documents.toList());
    m_editors.clear();
}

void MemoryAgent::closeViews()
{
    foreach (const QPointer<MemoryView> &w, m_views)
        if (w)
            w->close();
    m_views.clear();
}

void MemoryAgent::updateMemoryView(quint64 address, quint64 length)
{
    m_engine->fetchMemory(this, sender(), address, length);
}

void MemoryAgent::connectBinEditorWidget(QWidget *w)
{
    connect(w, SIGNAL(dataRequested(quint64)), SLOT(fetchLazyData(quint64)));
    connect(w, SIGNAL(newWindowRequested(quint64)), SLOT(createBinEditor(quint64)));
    connect(w, SIGNAL(newRangeRequested(quint64)), SLOT(provideNewRange(quint64)));
    connect(w, SIGNAL(dataChanged(quint64,QByteArray)), SLOT(handleDataChanged(quint64,QByteArray)));
    connect(w, SIGNAL(dataChanged(quint64,QByteArray)), SLOT(handleDataChanged(quint64,QByteArray)));
    connect(w, SIGNAL(addWatchpointRequested(quint64,uint)), SLOT(handleWatchpointRequest(quint64,uint)));
}

bool MemoryAgent::doCreateBinEditor(const MemoryViewSetupData &data)
{
    const bool readOnly = (data.flags & DebuggerEngine::MemoryReadOnly) != 0;
    QString title = data.title.isEmpty() ? tr("Memory at 0x%1").arg(data.startAddress, 0, 16) : data.title;
    // Separate view?
    if (data.flags & DebuggerEngine::MemoryView) {
        // Ask BIN editor plugin for factory service and have it create a bin editor widget.
        QWidget *binEditor = 0;
        if (QObject *factory = ExtensionSystem::PluginManager::getObjectByClassName(QLatin1String("BinEditor::BinEditorWidgetFactory")))
            binEditor = ExtensionSystem::invoke<QWidget *>(factory, "createWidget", (QWidget *)0);
        if (!binEditor)
            return false;
        connectBinEditorWidget(binEditor);
        MemoryView::setBinEditorReadOnly(binEditor, readOnly);
        MemoryView::setBinEditorNewWindowRequestAllowed(binEditor, true);
        MemoryView *topLevel = 0;
        // Memory view tracking register value, providing its own updating mechanism.
        if (data.flags & DebuggerEngine::MemoryTrackRegister) {
            topLevel = new RegisterMemoryView(binEditor, data.startAddress, data.registerName, m_engine->registerHandler(), data.parent);
        } else {
            // Ordinary memory view
            MemoryView::setBinEditorMarkup(binEditor, data.markup);
            MemoryView::setBinEditorRange(binEditor, data.startAddress, MemoryAgent::DataRange, MemoryAgent::BinBlockSize);
            topLevel = new MemoryView(binEditor, data.parent);
            topLevel->setWindowTitle(title);
        }
        m_views << topLevel;
        topLevel->move(data.pos);
        topLevel->show();
        return true;
    }
    // Editor: Register tracking not supported.
    QTC_ASSERT(!(data.flags & DebuggerEngine::MemoryTrackRegister), return false);
    if (!title.endsWith(QLatin1Char('$')))
        title.append(QLatin1String(" $"));
    IEditor *editor = EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_BINARY_EDITOR_ID, &title);
    if (!editor)
        return false;
    editor->document()->setProperty(Constants::OPENED_BY_DEBUGGER, QVariant(true));
    editor->document()->setTemporary(true);
    QWidget *editorBinEditor = editor->widget();
    connectBinEditorWidget(editorBinEditor);
    MemoryView::setBinEditorReadOnly(editorBinEditor, readOnly);
    MemoryView::setBinEditorNewWindowRequestAllowed(editorBinEditor, true);
    MemoryView::setBinEditorRange(editorBinEditor, data.startAddress, MemoryAgent::DataRange, MemoryAgent::BinBlockSize);
    MemoryView::setBinEditorMarkup(editorBinEditor, data.markup);
    m_editors << editor;
    return true;
}

void MemoryAgent::createBinEditor(const MemoryViewSetupData &data)
{
    if (!doCreateBinEditor(data))
        AsynchronousMessageBox::warning(
            tr("No Memory Viewer Available"),
            tr("The memory contents cannot be shown as no viewer plugin "
               "for binary data has been loaded."));
}

void MemoryAgent::createBinEditor(quint64 addr)
{
    MemoryViewSetupData data;
    data.startAddress = addr;
    createBinEditor(data);
}

void MemoryAgent::fetchLazyData(quint64 block)
{
    m_engine->fetchMemory(this, sender(), BinBlockSize * block, BinBlockSize);
}

void MemoryAgent::addLazyData(QObject *editorToken, quint64 addr,
                                  const QByteArray &ba)
{
    QWidget *w = qobject_cast<QWidget *>(editorToken);
    QTC_ASSERT(w, return);
    MemoryView::binEditorAddData(w, addr, ba);
}

void MemoryAgent::provideNewRange(quint64 address)
{
    QWidget *w = qobject_cast<QWidget *>(sender());
    QTC_ASSERT(w, return);
    MemoryView::setBinEditorRange(w, address, DataRange, BinBlockSize);
}

void MemoryAgent::handleDataChanged(quint64 addr, const QByteArray &data)
{
    m_engine->changeMemory(this, sender(), addr, data);
}

void MemoryAgent::handleWatchpointRequest(quint64 address, uint size)
{
    m_engine->breakHandler()->setWatchpointAtAddress(address, size);
}

void MemoryAgent::updateContents()
{
    foreach (const QPointer<IEditor> &e, m_editors)
        if (e)
            MemoryView::binEditorUpdateContents(e->widget());
    // Update all views except register views, which trigger on
    // register value set/changed.
    foreach (const QPointer<MemoryView> &w, m_views)
        if (w && !qobject_cast<RegisterMemoryView *>(w.data()))
            w->updateContents();
}

bool MemoryAgent::hasVisibleEditor() const
{
    QList<IEditor *> visible = EditorManager::visibleEditors();
    foreach (QPointer<IEditor> editor, m_editors)
        if (visible.contains(editor.data()))
            return true;
    return false;
}

void MemoryAgent::handleDebuggerFinished()
{
    foreach (const QPointer<IEditor> &editor, m_editors) {
        if (editor) { // Prevent triggering updates, etc.
            MemoryView::setBinEditorReadOnly(editor->widget(), true);
            editor->widget()->disconnect(this);
        }
    }
}

bool MemoryAgent::isBigEndian(const ProjectExplorer::Abi &a)
{
    switch (a.architecture()) {
    case ProjectExplorer::Abi::UnknownArchitecture:
    case ProjectExplorer::Abi::X86Architecture:
    case ProjectExplorer::Abi::ItaniumArchitecture: // Configureable
    case ProjectExplorer::Abi::ArmArchitecture:     // Configureable
    case ProjectExplorer::Abi::ShArchitecture:     // Configureable
        break;
    case ProjectExplorer::Abi::MipsArchitecture:     // Configureable
    case ProjectExplorer::Abi::PowerPCArchitecture: // Configureable
        return true;
    }
    return false;
}

// Read a POD variable from a memory location. Swap bytes if endianness differs
template <class POD> POD readPod(const unsigned char *data, bool swapByteOrder)
{
    POD pod = 0;
    if (swapByteOrder) {
        unsigned char *target = reinterpret_cast<unsigned char *>(&pod) + sizeof(POD) - 1;
        for (size_t i = 0; i < sizeof(POD); i++)
            *target-- = data[i];
    } else {
        std::memcpy(&pod, data, sizeof(POD));
    }
    return pod;
}

// Read memory from debuggee
quint64 MemoryAgent::readInferiorPointerValue(const unsigned char *data, const ProjectExplorer::Abi &a)
{
    const bool swapByteOrder = isBigEndian(a) != isBigEndian(ProjectExplorer::Abi::hostAbi());
    return a.wordWidth() == 32 ? readPod<quint32>(data, swapByteOrder) :
                                 readPod<quint64>(data, swapByteOrder);
}

} // namespace Internal
} // namespace Debugger

