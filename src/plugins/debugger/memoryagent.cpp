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

#include "memoryagent.h"

#include "breakhandler.h"
#include "debuggerengine.h"
#include "debuggerstartparameters.h"
#include "debuggercore.h"
#include "debuggerinternalconstants.h"
#include "registerhandler.h"

#include <bineditor/bineditorservice.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagebox.h>

#include <utils/qtcassert.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/invoker.h>

#include <QVBoxLayout>

#include <cstring>

using namespace Core;
using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

enum { BinBlockSize = 1024 };
enum { DataRange = 1024 * 1024 };

/*!
    \class Debugger::Internal::MemoryView
    \brief The MemoryView class is a base class for memory view tool windows.

    This class is a small tool-window that stays on top and displays a chunk
    of memory based on the widget provided by the BinEditor plugin.

    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
*/

class MemoryView : public QWidget
{
public:
    explicit MemoryView(BinEditor::EditorService *service)
        : QWidget(ICore::dialogParent(), Qt::Tool), m_service(service)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        auto layout = new QVBoxLayout(this);
        layout->addWidget(service->widget());
        layout->setContentsMargins(0, 0, 0, 0);
        setMinimumWidth(400);
        resize(800, 200);
    }

protected:
    void setMarkup(const QList<MemoryMarkup> &markup)
    {
        if (m_service) {
            m_service->clearMarkup();
            for (const MemoryMarkup &m : markup)
                m_service->addMarkup(m.address, m.length, m.color, m.toolTip);
            m_service->commitMarkup();
        }
    }

protected:
    BinEditor::EditorService *m_service;
};


/*!
    \class Debugger::Internal::RegisterMemoryView
    \brief The RegisterMemoryView class provides a memory view that shows the
           memory around the contents of a register (such as stack pointer,
           program counter), tracking changes of the register value.

    Connects to Debugger::Internal::RegisterHandler to listen for changes
    of the register value.

    \note Some registers might switch to 0 (for example, 'rsi','rbp'
    while stepping out of a function with parameters).
    This must be handled gracefully.

    \sa Debugger::Internal::RegisterHandler, Debugger::Internal::RegisterWindow
    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
*/

class RegisterMemoryView : public MemoryView
{
public:
    RegisterMemoryView(BinEditor::EditorService *service, quint64 addr, const QString &regName,
                       RegisterHandler *rh)
        : MemoryView(service), m_registerName(regName), m_registerAddress(addr)
    {
        connect(rh, &QAbstractItemModel::modelReset, this, &QWidget::close);
        connect(rh, &RegisterHandler::registerChanged, this, &RegisterMemoryView::onRegisterChanged);
        m_service->updateContents();
    }

private:
    void onRegisterChanged(const QString &name, quint64 value)
    {
        if (name == m_registerName)
            setRegisterAddress(value);
    }

    void setRegisterAddress(quint64 v)
    {
        if (v == m_registerAddress) {
            if (m_service)
                m_service->updateContents();
            return;
        }
        m_registerAddress = v;
        if (m_service)
            m_service->setSizes(v, DataRange, BinBlockSize);

        setWindowTitle(registerViewTitle(m_registerName, v));
        if (v)
            setMarkup(registerViewMarkup(v, m_registerName));
    }

    QString m_registerName;
    quint64 m_registerAddress;
};

QString registerViewTitle(const QString &registerName, quint64 addr)
{
    return MemoryAgent::tr("Memory at Register \"%1\" (0x%2)").arg(registerName).arg(addr, 0, 16);
}

QList<MemoryMarkup> registerViewMarkup(quint64 a, const QString &regName)
{
    return {MemoryMarkup(a, 1, QColor(Qt::blue).lighter(),
                         MemoryAgent::tr("Register \"%1\"").arg(regName))};
}

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

BinEditor::FactoryService *binEditorFactory()
{
    static auto theBinEditorFactory = ExtensionSystem::PluginManager::getObject<BinEditor::FactoryService>();
    return theBinEditorFactory;
}

bool MemoryAgent::hasBinEditor()
{
    return binEditorFactory() != nullptr;
}

MemoryAgent::MemoryAgent(const MemoryViewSetupData &data, DebuggerEngine *engine)
    : m_engine(engine), m_trackRegisters(data.trackRegisters)
{
    auto factory = binEditorFactory();
    if (!factory)
        return;

    QString title = data.title.isEmpty() ? tr("Memory at 0x%1").arg(data.startAddress, 0, 16) : data.title;
    if (!data.separateView && !title.endsWith('$'))
        title.append(" $");

    if (data.separateView) {
        // Ask BIN editor plugin for factory service and have it create a bin editor widget.
        m_service = factory->createEditorService(title, false);
    } else {
        // Editor: Register tracking not supported.
        m_service = factory->createEditorService(title, true);
    }

    if (!m_service)
        return;

    m_service->setNewRangeRequestHandler([this](quint64 address) {
        m_service->setSizes(address, DataRange, BinBlockSize);
    });

    m_service->setFetchDataHandler([this](quint64 address) {
        m_engine->fetchMemory(this, address, BinBlockSize);
    });

    m_service->setNewWindowRequestHandler([this](quint64 address) {
        MemoryViewSetupData data;
        data.startAddress = address;
        auto agent = new MemoryAgent(data, m_engine);
        if (!agent->isUsable())
            delete agent;
    });

    m_service->setDataChangedHandler([this](quint64 address, const QByteArray &data) {
        m_engine->changeMemory(this, address, data);
    });

    m_service->setWatchPointRequestHandler([this](quint64 address, uint size) {
        m_engine->breakHandler()->setWatchpointAtAddress(address, size);
    });

    m_service->setAboutToBeDestroyedHandler([this] { m_service = nullptr; });

    // Separate view?
    if (data.separateView) {
        // Memory view tracking register value, providing its own updating mechanism.
        if (data.trackRegisters) {
            auto view = new RegisterMemoryView(m_service, data.startAddress, data.registerName,
                                               m_engine->registerHandler());
            view->show();
        } else {
            // Ordinary memory view
            auto view = new MemoryView(m_service);
            view->setWindowTitle(title);
            view->show();
        }
    } else {
        m_service->editor()->document()->setTemporary(true);
        m_service->editor()->document()->setProperty(Constants::OPENED_BY_DEBUGGER, QVariant(true));
    }

    m_service->setReadOnly(data.readOnly);
    m_service->setNewWindowRequestAllowed(true);
    m_service->setSizes(data.startAddress, DataRange, BinBlockSize);
    m_service->clearMarkup();
    for (const MemoryMarkup &m : data.markup)
        m_service->addMarkup(m.address, m.length, m.color, m.toolTip);
    m_service->commitMarkup();
}

MemoryAgent::~MemoryAgent()
{
    if (m_service && m_service->editor())
        EditorManager::closeDocument(m_service->editor()->document());
    if (m_service && m_service->widget()) // m_service might be set to null by closeDocument
        m_service->widget()->close();
}

void MemoryAgent::updateContents()
{
    // Update all views except register views, which trigger on
    // register value set/changed.
    if (!m_trackRegisters && m_service)
        m_service->updateContents();
}

void MemoryAgent::addData(quint64 address, const QByteArray &a)
{
    QTC_ASSERT(m_service, return);
    m_service->addData(address, a);
}

void MemoryAgent::setFinished()
{
    if (m_service)
        m_service->setFinished();
}

bool MemoryAgent::isUsable()
{
    return m_service != nullptr;
}

} // namespace Internal
} // namespace Debugger

