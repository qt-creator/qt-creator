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

#ifndef QTCPROCESS_H
#define QTCPROCESS_H

#include <QProcess>

#include "utils_global.h"

#include "environment.h"

namespace Utils {
class AbstractMacroExpander;

class QTCREATOR_UTILS_EXPORT QtcProcess : public QProcess
{
    Q_OBJECT

public:
    QtcProcess(QObject *parent = 0)
      : QProcess(parent),
        m_haveEnv(false)
#ifdef Q_OS_WIN
      , m_useCtrlCStub(false)
#endif
        {}
    void setEnvironment(const Environment &env)
        { m_environment = env; m_haveEnv = true; }
    void setCommand(const QString &command, const QString &arguments)
        { m_command = command; m_arguments = arguments; }
#ifdef Q_OS_WIN
    void setUseCtrlCStub(bool enabled) { m_useCtrlCStub = enabled; }
#endif
    void start();
    void terminate();

    enum SplitError {
        SplitOk = 0, //! All went just fine
        BadQuoting, //! Command contains quoting errors
        FoundMeta //! Command contains complex shell constructs
    };

    //! Quote a single argument for usage in a unix shell command
    static QString quoteArgUnix(const QString &arg);
    //! Quote a single argument and append it to a unix shell command
    static void addArgUnix(QString *args, const QString &arg);
    //! Join an argument list into a unix shell command
    static QString joinArgsUnix(const QStringList &args);
#ifdef Q_OS_WIN
    //! Quote a single argument for usage in a shell command
    static QString quoteArg(const QString &arg);
    //! Quote a single argument and append it to a shell command
    static void addArg(QString *args, const QString &arg);
    //! Join an argument list into a shell command
    static QString joinArgs(const QStringList &args);
    //! Prepare argument of a shell command for feeding into QProcess
    static QString prepareArgs(const QString &cmd, SplitError *err,
                               const Environment *env = 0, const QString *pwd = 0);
    //! Prepare a shell command for feeding into QProcess
    static void prepareCommand(const QString &command, const QString &arguments,
                               QString *outCmd, QString *outArgs,
                               const Environment *env = 0, const QString *pwd = 0);
#else
    static QString quoteArg(const QString &arg) { return quoteArgUnix(arg); }
    static void addArg(QString *args, const QString &arg) { addArgUnix(args, arg); }
    static QString joinArgs(const QStringList &args) { return joinArgsUnix(args); }
    static QStringList prepareArgs(const QString &cmd, SplitError *err,
                                   const Environment *env = 0, const QString *pwd = 0)
        { return splitArgs(cmd, true, err, env, pwd); }
    static bool prepareCommand(const QString &command, const QString &arguments,
                               QString *outCmd, QStringList *outArgs,
                               const Environment *env = 0, const QString *pwd = 0);
#endif
    //! Quote and append each argument to a shell command
    static void addArgs(QString *args, const QStringList &inArgs);
    //! Append already quoted arguments to a shell command
    static void addArgs(QString *args, const QString &inArgs);
    //! Split a shell command into separate arguments. ArgIterator is usually a better choice.
    static QStringList splitArgs(const QString &cmd, bool abortOnMeta = false, SplitError *err = 0,
                                 const Environment *env = 0, const QString *pwd = 0);
    //! Safely replace the expandos in a shell command
    static bool expandMacros(QString *cmd, AbstractMacroExpander *mx);
    static QString expandMacros(const QString &str, AbstractMacroExpander *mx);

    /*! Iterate over arguments from a command line.
     *  Assumes that the name of the actual command is *not* part of the line.
     *  Terminates after the first command if the command line is complex.
     */
    class QTCREATOR_UTILS_EXPORT ArgIterator {
    public:
        ArgIterator(QString *str) : m_str(str), m_pos(0), m_prev(-1) {}
        //! Get the next argument. Returns false on encountering end of first command.
        bool next();
        //! True iff the argument is a plain string, possibly after unquoting.
        bool isSimple() const { return m_simple; }
        //! Return the string value of the current argument if it is simple, otherwise empty.
        QString value() const { return m_value; }
        //! Delete the last argument fetched via next() from the command line.
        void deleteArg();
        //! Insert argument into the command line after the last one fetched via next().
        //! This may be used before the first call to next() to insert at the front.
        void appendArg(const QString &str);
    private:
        QString *m_str, m_value;
        int m_pos, m_prev;
        bool m_simple;
    };

    class QTCREATOR_UTILS_EXPORT ConstArgIterator {
    public:
        ConstArgIterator(const QString &str) : m_str(str), m_ait(&m_str) {}
        bool next() { return m_ait.next(); }
        bool isSimple() const { return m_ait.isSimple(); }
        QString value() const { return m_ait.value(); }
    private:
        QString m_str;
        ArgIterator m_ait;
    };

private:
    QString m_command;
    QString m_arguments;
    Environment m_environment;
    bool m_haveEnv;
#ifdef Q_OS_WIN
    bool m_useCtrlCStub;
#endif
};

}

#endif // QTCPROCESS_H
