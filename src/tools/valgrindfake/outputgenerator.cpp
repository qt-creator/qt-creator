/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "outputgenerator.h"

#include <QAbstractSocket>
#include <QIODevice>
#include <QTextStream>
#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include <unistd.h>

using namespace Valgrind::Fake;

OutputGenerator::OutputGenerator(QAbstractSocket *output, QIODevice *input) :
    m_output(output),
    m_input(input),
    m_finished(false),
    m_crash(false),
    m_garbage(false),
    m_wait(0)
{
    Q_ASSERT(input->isOpen());
    Q_ASSERT(input->isReadable());

    m_timer.setSingleShot(true);
    m_timer.start();

    connect(&m_timer, SIGNAL(timeout()),
            this, SLOT(writeOutput()));
}

void OutputGenerator::setCrashRandomly(bool enable)
{
    m_crash = enable;
}

void OutputGenerator::setOutputGarbage(bool enable)
{
    m_garbage = enable;
}

void OutputGenerator::setWait(uint seconds)
{
    m_wait = seconds;
}

#include <iostream>

static bool blockingWrite(QIODevice *dev, const QByteArray &ba)
{
    const qint64 toWrite = ba.size();
    qint64 written = 0;
    while (written < ba.size()) {
        const qint64 n = dev->write(ba.constData() + written, toWrite - written);
        if (n < 0)
            return false;
        written += n;
    }

    return true;
}

void OutputGenerator::produceRuntimeError()
{
    if (m_crash) {
        std::cerr << "Goodbye, cruel world" << std::endl;
        int zero = 0; // hide the error at compile-time to avoid a compiler warning
        int i = 1 / zero;
        Q_UNUSED(i);
        Q_ASSERT(false);
    }
    else if (m_garbage) {
        std::cerr << "Writing garbage" << std::endl;
        blockingWrite(m_output, "<</GARBAGE = '\"''asdfaqre");
        m_output->flush();
    } else if (m_wait) {
        qDebug() << "waiting in fake valgrind for " << m_wait << " seconds..." << endl;
        sleep(m_wait);
    }
}


void OutputGenerator::writeOutput()
{
    m_timer.setInterval(qrand() % 1000);

    int lines = 0;
    while (!m_input->atEnd()) {
        qint64 lastPos = m_input->pos();
        QByteArray line = m_input->readLine();
        if (lines > 0 && !m_finished && line.contains("<error>")) {
            if ((m_crash || m_garbage || m_wait) && qrand() % 10 == 1) {
                produceRuntimeError();
                m_timer.start();
                return;
            }

            m_input->seek(lastPos);
            m_timer.start();
            return;
        } else {
            if (!m_finished && line.contains("<state>FINISHED</state>")) {
                m_finished = true;
                if (m_crash || m_garbage || m_wait) {
                    produceRuntimeError();
                    m_timer.start();
                    return;
                }
            }

            if (!blockingWrite(m_output, line)) {
                std::cerr << "Writing to socket failed: " << qPrintable(m_output->errorString()) << std::endl;
                emit finished();
                return;
            }
            m_output->flush();
            ++lines;
        }
    }

    Q_ASSERT(m_input->atEnd());

    while (m_output->bytesToWrite() > 0)
        m_output->waitForBytesWritten(-1);
    emit finished();
}
