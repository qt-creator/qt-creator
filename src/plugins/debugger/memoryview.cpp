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

#include "memoryview.h"
#include "registerhandler.h"
#include "memoryagent.h"

#include <bineditor/markup.h>

#include <QVBoxLayout>
#include <QDebug>

namespace Debugger {
namespace Internal {

/*!
    \class Debugger::Internal::MemoryView
    \brief The MemoryView class is a base class for memory view tool windows.

    This class is a small tool-window that stays on top and displays a chunk of memory
    based on the widget provided by the Bin editor plugin.

    Provides static functionality for handling a Bin Editor Widget
    (soft dependency) via QMetaObject invocation.

    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
*/

MemoryView::MemoryView(QWidget *binEditor, QWidget *parent) :
    QWidget(parent, Qt::Tool), m_binEditor(binEditor)
{
    setAttribute(Qt::WA_DeleteOnClose);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(binEditor);
    layout->setContentsMargins(0, 0, 0, 0);
    setMinimumWidth(400);
}

void MemoryView::setBinEditorRange(QWidget *w, quint64 address, int range, int blockSize)
{
    QMetaObject::invokeMethod(w, "setSizes",
        Q_ARG(quint64, address), Q_ARG(int, range), Q_ARG(int, blockSize));
}

void MemoryView::setBinEditorReadOnly(QWidget *w, bool readOnly)
{
    w->setProperty("readOnly", QVariant(readOnly));
}

void MemoryView::setBinEditorNewWindowRequestAllowed(QWidget *w, bool a)
{
    w->setProperty("newWindowRequestAllowed", QVariant(a));
}

void MemoryView::setBinEditorMarkup(QWidget *w, const QList<MemoryMarkup> &ml)
{
    // Convert into bin editor markup and set.
    QList<BinEditor::Markup> bml;
    foreach (const MemoryMarkup &m, ml)
        bml.push_back(BinEditor::Markup(m.address, m.length, m.color, m.toolTip));
    w->setProperty("markup", qVariantFromValue(bml));
}

void MemoryView::binEditorUpdateContents(QWidget *w)
{
    QMetaObject::invokeMethod(w, "updateContents");
}

void MemoryView::binEditorSetCursorPosition(QWidget *w, int p)
{
    QMetaObject::invokeMethod(w, "setCursorPosition", Q_ARG(int, p));
}

void MemoryView::binEditorAddData(QWidget *w, quint64 addr, const QByteArray &ba)
{
    QMetaObject::invokeMethod(w, "addData",
                              Q_ARG(quint64, addr / MemoryAgent::BinBlockSize),
                              Q_ARG(QByteArray, ba));
}

void MemoryView::setAddress(quint64 a)
{
    setBinEditorRange(m_binEditor, a, MemoryAgent::DataRange, MemoryAgent::BinBlockSize);
}

void MemoryView::updateContents()
{
    binEditorUpdateContents(m_binEditor);
}

void MemoryView::setMarkup(const QList<MemoryMarkup> &m)
{
    MemoryView::setBinEditorMarkup(m_binEditor, m);
}

/*!
    \class Debugger::Internal::RegisterMemoryView
    \brief The RegisterMemoryView class provides a memory view that shows the
           memory around the contents of a register
           (such as stack pointer, program counter),
           tracking changes of the register value.

    Connects to Debugger::Internal::RegisterHandler to listen for changes
    of the register value.

    \note Some registers might switch to 0 (for example, 'rsi','rbp'
     while stepping out of a function with parameters).
    This must be handled gracefully.

    \sa Debugger::Internal::RegisterHandler, Debugger::Internal::RegisterWindow
    \sa Debugger::Internal::MemoryAgent, Debugger::DebuggerEngine
*/

RegisterMemoryView::RegisterMemoryView(QWidget *binEditor, quint64 addr,
                                       const QByteArray &regName,
                                       RegisterHandler *handler, QWidget *parent) :
    MemoryView(binEditor, parent),
    m_registerName(regName), m_registerAddress(addr)
{
    connect(handler, &QAbstractItemModel::modelReset, this, &QWidget::close);
    connect(handler, &RegisterHandler::registerChanged, this, &RegisterMemoryView::onRegisterChanged);
    updateContents();
}

void RegisterMemoryView::onRegisterChanged(const QByteArray &name, quint64 value)
{
    if (name == m_registerName)
        setRegisterAddress(value);
}

QString RegisterMemoryView::title(const QByteArray &registerName, quint64 a)
{
    return tr("Memory at Register \"%1\" (0x%2)")
            .arg(QString::fromUtf8(registerName)).arg(a, 0, 16);
}

void RegisterMemoryView::setRegisterAddress(quint64 v)
{
    if (v == m_registerAddress) {
        updateContents();
        return;
    }
    m_registerAddress = v;
    setAddress(v);
    setWindowTitle(title(m_registerName, v));
    if (v)
        setMarkup(registerMarkup(v, m_registerName));
}

QList<MemoryMarkup> RegisterMemoryView::registerMarkup(quint64 a, const QByteArray &regName)
{
    QList<MemoryMarkup> result;
    result.push_back(MemoryMarkup(a, 1, QColor(Qt::blue).lighter(),
        tr("Register \"%1\"").arg(QString::fromUtf8(regName))));
    return result;
}

} // namespace Internal
} // namespace Debugger
