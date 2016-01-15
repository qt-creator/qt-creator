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
