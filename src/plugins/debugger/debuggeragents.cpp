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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "debuggeragents.h"
#include "idebuggerengine.h"

#include <coreplugin/editormanager/editormanager.h>

#include <limits.h>

namespace Debugger {
namespace Internal {

MemoryViewAgent::MemoryViewAgent(DebuggerManager *manager, quint64 addr)
    : QObject(manager), m_engine(manager->currentEngine())
{
    Core::EditorManager *editorManager = Core::EditorManager::instance();
    QString titlePattern = "Memory $";
    m_editor = editorManager->openEditorWithContents(
        Core::Constants::K_DEFAULT_BINARY_EDITOR,
        &titlePattern);
    connect(m_editor->widget(), SIGNAL(lazyDataRequested(int,bool)),
        this, SLOT(fetchLazyData(int,bool)));
    editorManager->activateEditor(m_editor);
    QMetaObject::invokeMethod(m_editor->widget(), "setLazyData",
    Q_ARG(int, addr), Q_ARG(int, INT_MAX), Q_ARG(int, BinBlockSize));
}

MemoryViewAgent::~MemoryViewAgent()
{
    m_editor->deleteLater();
}

void MemoryViewAgent::fetchLazyData(int block, bool sync)
{
    Q_UNUSED(sync); // FIXME: needed support for incremental searching
    m_engine->fetchMemory(this, BinBlockSize * block, BinBlockSize); 
}

void MemoryViewAgent::addLazyData(quint64 addr, const QByteArray &ba)
{
    QMetaObject::invokeMethod(m_editor->widget(), "addLazyData",
        Q_ARG(int, addr / BinBlockSize), Q_ARG(QByteArray, ba));
}


} // namespace Internal
} // namespace Debugger

