/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "memoryagent.h"

#include "debuggerengine.h"
#include "debuggerstartparameters.h"
#include "debuggercore.h"
#include "memoryviewwidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

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
*/

namespace { const int DataRange = 1024 * 1024; }

MemoryAgent::MemoryAgent(DebuggerEngine *engine)
    : QObject(engine), m_engine(engine)
{
    QTC_ASSERT(engine, /**/);
}

MemoryAgent::~MemoryAgent()
{
    QList<IEditor *> editors;
    foreach (QPointer<IEditor> editor, m_editors)
        if (editor)
            editors.append(editor.data());
    EditorManager::instance()->closeEditors(editors);
}

void MemoryAgent::openMemoryView(quint64 address, quint64 length, const QPoint &pos)
{
    MemoryViewWidget *w = new MemoryViewWidget(Core::ICore::instance()->mainWindow());
    w->setUpdateOnInferiorStop(true);
    w->move(pos);
    w->requestMemory(address, length);
    addMemoryView(w);
}

void MemoryAgent::addMemoryView(MemoryViewWidget *w)
{
    w->setAbi(m_engine->startParameters().toolChainAbi);
    connect(w, SIGNAL(memoryRequested(quint64,quint64)),
            this, SLOT(updateMemoryView(quint64,quint64)));
    connect(m_engine, SIGNAL(stateChanged(Debugger::DebuggerState)),
            w, SLOT(engineStateChanged(Debugger::DebuggerState)));
    connect(w, SIGNAL(openViewRequested(quint64,quint64,QPoint)),
            this, SLOT(openMemoryView(quint64,quint64,QPoint)));
    w->requestMemory();
    w->show();
}

void MemoryAgent::updateMemoryView(quint64 address, quint64 length)
{
    m_engine->fetchMemory(this, sender(), address, length);
}

void MemoryAgent::createBinEditor(quint64 addr)
{
    EditorManager *editorManager = EditorManager::instance();
    QString titlePattern = tr("Memory $");
    IEditor *editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_BINARY_EDITOR_ID,
        &titlePattern);
    if (editor) {
        editor->setProperty(Constants::OPENED_BY_DEBUGGER, true);
        editor->setProperty(Constants::OPENED_WITH_MEMORY, true);
        connect(editor->widget(),
            SIGNAL(dataRequested(Core::IEditor*,quint64)),
            SLOT(fetchLazyData(Core::IEditor*,quint64)));
        connect(editor->widget(),
            SIGNAL(newWindowRequested(quint64)),
            SLOT(createBinEditor(quint64)));
        connect(editor->widget(),
            SIGNAL(newRangeRequested(Core::IEditor*,quint64)),
            SLOT(provideNewRange(Core::IEditor*,quint64)));
        connect(editor->widget(),
            SIGNAL(startOfFileRequested(Core::IEditor*)),
            SLOT(handleStartOfFileRequested(Core::IEditor*)));
        connect(editor->widget(),
            SIGNAL(endOfFileRequested(Core::IEditor *)),
            SLOT(handleEndOfFileRequested(Core::IEditor*)));
        connect(editor->widget(),
            SIGNAL(dataChanged(Core::IEditor*,quint64,QByteArray)),
            SLOT(handleDataChanged(Core::IEditor*,quint64,QByteArray)));
        m_editors << editor;
        QMetaObject::invokeMethod(editor->widget(), "setNewWindowRequestAllowed");
        QMetaObject::invokeMethod(editor->widget(), "setSizes",
            Q_ARG(quint64, addr),
            Q_ARG(int, DataRange),
            Q_ARG(int, BinBlockSize));
        editorManager->activateEditor(editor);
    } else {
        showMessageBox(QMessageBox::Warning,
            tr("No memory viewer available"),
            tr("The memory contents cannot be shown as no viewer plugin "
               "for binary data has been loaded."));
        deleteLater();
    }
}

void MemoryAgent::fetchLazyData(IEditor *editor, quint64 block)
{
    m_engine->fetchMemory(this, editor, BinBlockSize * block, BinBlockSize);
}

void MemoryAgent::addLazyData(QObject *editorToken, quint64 addr,
                                  const QByteArray &ba)
{

    if (IEditor *editor = qobject_cast<IEditor *>(editorToken)) {
        if (QWidget *editorWidget = editor->widget()) {
            QMetaObject::invokeMethod(editorWidget , "addData",
                Q_ARG(quint64, addr / BinBlockSize), Q_ARG(QByteArray, ba));
        }
        return;
    }
    if (MemoryViewWidget *mvw = qobject_cast<MemoryViewWidget*>(editorToken))
        mvw->setData(ba);
}

void MemoryAgent::provideNewRange(IEditor *editor, quint64 address)
{
    QMetaObject::invokeMethod(editor->widget(), "setSizes",
        Q_ARG(quint64, address),
        Q_ARG(int, DataRange),
        Q_ARG(int, BinBlockSize));
}

// Since we are not dealing with files, we take these signals to mean
// "move to start/end of range". This seems to make more sense than
// jumping to the start or end of the address space, respectively.
void MemoryAgent::handleStartOfFileRequested(IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, 0));
}

void MemoryAgent::handleEndOfFileRequested(IEditor *editor)
{
    QMetaObject::invokeMethod(editor->widget(),
        "setCursorPosition", Q_ARG(int, DataRange - 1));
}

void MemoryAgent::handleDataChanged(IEditor *editor,
    quint64 addr, const QByteArray &data)
{
    m_engine->changeMemory(this, editor, addr, data);
}

void MemoryAgent::updateContents()
{
    foreach (QPointer<IEditor> editor, m_editors)
        if (editor)
            QMetaObject::invokeMethod(editor->widget(), "updateContents");
}

bool MemoryAgent::hasVisibleEditor() const
{
    QList<IEditor *> visible = EditorManager::instance()->visibleEditors();
    foreach (QPointer<IEditor> editor, m_editors)
        if (visible.contains(editor.data()))
            return true;
    return false;
}

bool MemoryAgent::isBigEndian(const ProjectExplorer::Abi &a)
{
    switch (a.architecture()) {
    case ProjectExplorer::Abi::UnknownArchitecture:
    case ProjectExplorer::Abi::X86Architecture:
    case ProjectExplorer::Abi::ItaniumArchitecture: // Configureable
    case ProjectExplorer::Abi::ArmArchitecture:     // Configureable
        break;
    case ProjectExplorer::Abi::MipsArcitecture:     // Configureable
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
