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

#ifndef QLOGGER_H
#define QLOGGER_H

#include <QString>
#include <QStringList>


#define qLogger(str) qDebug() << QString("GLOBAL %1, %2").arg(__FUNC__).arg(#str);
#define qLogger(module, str) qDebug() << QString("%1, %2 %3").arg(__FUNC__).arg(#str).arg(#module);

class QFile;
class QTime;

class QLogger {
public:
    static void enable() {
        setEnabled(true);
    }
    static void disable() {
        setEnabled(false);
    }
    static void setLevel(int level);
    static void setModul(const QString &module);
    static void setFilename(const QString &filename);
    static void setEnabled(bool enabled);
    static void setFlush(int msec);
    static void flush();
    static void setSilent(bool silent);
    ~QLogger();

private:
    QLogger();

    static QLogger* instance();
    static QLogger* m_instance;
    void output(const char *msg);
    static void loggerMessageOutput(QtMsgType type, const char *msg);

    int m_level;
    QString m_modul;
    QString m_fileName;
    QFile* m_file;
    bool m_enabled;
    bool m_silent;
    int m_flushTime;
    int m_lastFlush;
    QStringList m_buffer;
    QTime *m_timer;

};

#endif // QLOGGER_H
