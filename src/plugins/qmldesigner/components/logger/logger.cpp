/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "logger.h"

#include <qapplication.h>
#include <stdio.h>
#include <stdlib.h>

#include <QTime>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

QLogger* QLogger::m_instance;

void QLogger::loggerMessageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
     case QtDebugMsg:
         if (!QLogger::instance()->m_silent)
           QLogger::instance()->output(msg);
         break;
     case QtWarningMsg:
         fprintf(stderr, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         abort();
    }
}

void QLogger::setLevel(int level)
{
    instance()->m_level = level;
}

void QLogger::setSilent(bool silent)
{
    instance()->m_silent = silent;
}

void QLogger::setModul(const QString &module)
{
    instance()->m_modul = module;
}

void QLogger::setFilename(const QString &filename)
{
    instance()->m_fileName = filename;
    if (instance()->m_file) {
        instance()->m_file->close();
        delete instance()->m_file;
    }
    instance()->m_file = new QFile(filename);
    instance()->m_fileName = filename;
    instance()->m_file->open(QIODevice::WriteOnly);
}

void QLogger::setEnabled(bool enabled)
{
    instance()->m_enabled = enabled;
}

void QLogger::setFlush(int msec)
{
    instance()->m_flushTime = msec;
}

void QLogger::flush()
{
    instance()->m_lastFlush = QTime::currentTime().elapsed();
    if (instance()->m_file) {
        foreach (QString s, instance()->m_buffer) {
            s += QLatin1Char('\n');
            instance()->m_file->write (s.toAscii());
        }
        instance()->m_file->flush();
    } else {
        foreach ( QString s, instance()->m_buffer) {
            s += QLatin1Char('\n');
#ifdef Q_OS_WIN
            OutputDebugStringW((TCHAR*)s.utf16());
#else
            fprintf(stderr, "Debug: %s\n", s.toAscii().constData());
#endif
        }
    }
    instance()->m_buffer.clear();
}

QLogger::QLogger() : m_level(0), m_modul(), m_fileName(), m_file(0), m_enabled(true), m_silent(false), m_flushTime(1000)
{
   qInstallMsgHandler(loggerMessageOutput);
   m_timer = new QTime();
   m_timer->start();
   m_lastFlush = m_timer->elapsed();
}

QLogger::~QLogger()
{
    flush();
    if (m_file) {
        m_file->close();
        delete m_file;
    }
    delete m_timer;
}

QLogger* QLogger::instance()
{
    if (!m_instance)
        m_instance = new QLogger();
    return m_instance;
}

void QLogger::output(const char *msg)
{

    QString s = QString::fromAscii(msg);
    if (m_enabled && (m_modul.isEmpty() || s.contains(m_modul, Qt::CaseInsensitive))) {
        int level = 0;
        if (s.contains("LEVEL=1")) {
            s = s.remove("LEVEL=1");
            level = 1;
        } else if (s.contains("LEVEL=2")) {
            s = s.remove("LEVEL=2");
            level = 2;
        } else if (s.contains("LEVEL=3")) {
            s = s.remove("LEVEL=3");
            level = 3;
        }
        if (m_level >= level)
            m_buffer.append(s);
    }
    int time = m_timer->elapsed();
    if (time > m_lastFlush + m_flushTime)
        flush();
}
