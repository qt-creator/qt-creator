// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputgenerator.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDebug>
#include <QIODevice>
#include <QRandomGenerator>
#include <QStringList>
#include <QTextStream>

// Yes, this is ugly. But please don't introduce a libUtils dependency
// just to get rid of a single function.
#ifdef Q_OS_WIN
#include <windows.h>
void doSleep(int msec) { ::Sleep(msec); }
#else
#include <time.h>
#include <unistd.h>
void doSleep(int msec)
{
    struct timespec ts = {msec / 1000, (msec % 1000) * 1000000};
    ::nanosleep(&ts, nullptr);
}
#endif

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

    connect(&m_timer, &QTimer::timeout,
            this, &OutputGenerator::writeOutput);
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
#ifndef __clang_analyzer__
        int zero = 0; // hide the error at compile-time to avoid a compiler warning
        int i = 1 / zero;
        Q_UNUSED(i)
#endif
        Q_ASSERT(false);
    } else if (m_garbage) {
        std::cerr << "Writing garbage" << std::endl;
        blockingWrite(m_output, "<</GARBAGE = '\"''asdfaqre");
        m_output->flush();
    } else if (m_wait) {
        qDebug() << "waiting in fake valgrind for " << m_wait << " seconds..." << '\n';
        doSleep(1000 * m_wait);
    }
}


void OutputGenerator::writeOutput()
{
    m_timer.setInterval(QRandomGenerator::global()->generate() % 1000);

    int lines = 0;
    while (!m_input->atEnd()) {
        qint64 lastPos = m_input->pos();
        QByteArray line = m_input->readLine();
        if (lines > 0 && !m_finished && line.contains("<error>")) {
            if ((m_crash || m_garbage || m_wait) && QRandomGenerator::global()->generate() % 10 == 1) {
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
