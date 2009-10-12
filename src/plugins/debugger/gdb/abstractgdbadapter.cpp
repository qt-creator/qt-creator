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

#include "abstractgdbadapter.h"

#include <utils/qtcassert.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>

namespace Debugger {
namespace Internal {

AbstractGdbAdapter::AbstractGdbAdapter(GdbEngine *engine, QObject *parent)
        : QObject(parent), m_engine(engine)
{
}

AbstractGdbAdapter::~AbstractGdbAdapter()
{
    disconnect();
}

// This cannot be in the c'tor, as it would not connect the "virtual" slots
void AbstractGdbAdapter::commonInit()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(handleGdbError(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(started()),
        this, SLOT(handleGdbStarted()));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)),
        this, SLOT(handleGdbFinished(int, QProcess::ExitStatus)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()),
        this, SIGNAL(readyReadStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()),
        this, SIGNAL(readyReadStandardError()));
}

QByteArray AbstractGdbAdapter::readAllStandardOutput()
{
    return m_gdbProc.readAllStandardOutput();
}

QByteArray AbstractGdbAdapter::readAllStandardError()
{
    return m_gdbProc.readAllStandardError();
}

void AbstractGdbAdapter::write(const QByteArray &data)
{
    m_gdbProc.write(data);
}

bool AbstractGdbAdapter::isTrkAdapter() const
{
    return false;
}

QString AbstractGdbAdapter::msgGdbStopFailed(const QString &why)
{
    return tr("The Gdb process could not be stopped:\n%1").arg(why);
}

QString AbstractGdbAdapter::msgInferiorStopFailed(const QString &why)
{
    return tr("Inferior process could not be stopped:\n%1").arg(why);
}

QString AbstractGdbAdapter::msgInferiorStarted()
{
    return tr("Inferior started.");
}

QString AbstractGdbAdapter::msgInferiorRunning()
{
    return tr("Inferior running.");
}

QString AbstractGdbAdapter::msgAttachedToStoppedInferior()
{
    return tr("Attached to stopped inferior.");
}

QString AbstractGdbAdapter::msgConnectRemoteServerFailed(const QString &why)
{
    return tr("Connecting to remote server failed:\n%1").arg(why);
}

} // namespace Internal
} // namespace Debugger
