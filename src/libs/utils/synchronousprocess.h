/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SYNCHRONOUSPROCESS_H
#define SYNCHRONOUSPROCESS_H

#include "utils_global.h"

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QTextCodec;
class QDebug;
class QByteArray;
QT_END_NAMESPACE

namespace Core {
namespace Utils {

struct SynchronousProcessPrivate;

/* Result of SynchronousProcess execution */
struct QWORKBENCH_UTILS_EXPORT SynchronousProcessResponse
{
    enum Result {
        // Finished with return code 0
        Finished,
        // Finished with return code != 0
        FinishedError,
        // Process terminated abnormally (kill)
        TerminatedAbnormally,
        // Executable could not be started
        StartFailed,
        // Hang, no output after time out
        Hang };

    SynchronousProcessResponse();
    void clear();

    Result result;
    int exitCode;
    QString stdOut;
    QString stdErr;
};

QWORKBENCH_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse &);

/* SynchronousProcess: Runs a synchronous process in its own event loop
 * that blocks only user input events. Thus, it allows for the gui to
 * repaint and append output to log windows.
 *
 * The stdOut(), stdErr() signals are emitted unbuffered as the process
 * writes them.
 *
 * The stdOutBuffered(), stdErrBuffered() signals are emitted with complete
 * lines based on the '\n' marker if they are enabled using
 * stdOutBufferedSignalsEnabled()/setStdErrBufferedSignalsEnabled().
 * They would typically be used for log windows. */

class QWORKBENCH_UTILS_EXPORT SynchronousProcess : public QObject
{
    Q_OBJECT
public:
    SynchronousProcess();
    virtual ~SynchronousProcess();

    /* Timeout for hanging processes (no reaction on stderr/stdout)*/
    void setTimeout(int timeoutMS);
    int timeout() const;

    void setStdOutCodec(QTextCodec *c);
    QTextCodec *stdOutCodec() const;

    bool stdOutBufferedSignalsEnabled() const;
    void setStdOutBufferedSignalsEnabled(bool);

    bool stdErrBufferedSignalsEnabled() const;
    void setStdErrBufferedSignalsEnabled(bool);

    QStringList environment() const;
    void setEnvironment(const QStringList &);

    SynchronousProcessResponse run(const QString &binary, const QStringList &args);

signals:
    void stdOut(const QByteArray &data, bool firstTime);
    void stdErr(const QByteArray &data, bool firstTime);

    void stdOutBuffered(const QString &data, bool firstTime);
    void stdErrBuffered(const QString &data, bool firstTime);

private slots:
    void slotTimeout();
    void finished(int exitCode, QProcess::ExitStatus e);
    void error(QProcess::ProcessError);
    void stdOutReady();
    void stdErrReady();

private:
    void processStdOut(bool emitSignals);
    void processStdErr(bool emitSignals);
    static QString convertStdErr(const QByteArray &);
    QString convertStdOut(const QByteArray &) const;

    SynchronousProcessPrivate *m_d;
};

} // namespace Utils
} // namespace Core

#endif
