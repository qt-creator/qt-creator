/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
