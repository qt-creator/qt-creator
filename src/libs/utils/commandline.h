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

#pragma once

#include "utils_global.h"

#include "fileutils.h"
#include "hostosinfo.h"

#include <QStringList>

namespace Utils {

class AbstractMacroExpander;
class CommandLine;
class Environment;

class QTCREATOR_UTILS_EXPORT ProcessArgs
{
public:
    static ProcessArgs createWindowsArgs(const QString &args);
    static ProcessArgs createUnixArgs(const QStringList &args);

    QString toWindowsArgs() const;
    QStringList toUnixArgs() const;
    QString toString() const;

    enum SplitError {
        SplitOk = 0, //! All went just fine
        BadQuoting, //! Command contains quoting errors
        FoundMeta //! Command contains complex shell constructs
    };

    //! Quote a single argument for usage in a unix shell command
    static QString quoteArgUnix(const QString &arg);
    //! Quote a single argument for usage in a shell command
    static QString quoteArg(const QString &arg, OsType osType = HostOsInfo::hostOs());
    //! Quote a single argument and append it to a shell command
    static void addArg(QString *args, const QString &arg, OsType osType = HostOsInfo::hostOs());
    //! Join an argument list into a shell command
    static QString joinArgs(const QStringList &args, OsType osType = HostOsInfo::hostOs());
    //! Prepare argument of a shell command for feeding into QProcess
    static ProcessArgs prepareArgs(const QString &args, SplitError *err, OsType osType,
                                   const Environment *env = nullptr, const FilePath *pwd = nullptr,
                                   bool abortOnMeta = true);
    //! Prepare a shell command for feeding into QProcess
    static bool prepareCommand(const CommandLine &cmdLine, QString *outCmd, ProcessArgs *outArgs,
                               const Environment *env = nullptr, const FilePath *pwd = nullptr);
    //! Quote and append each argument to a shell command
    static void addArgs(QString *args, const QStringList &inArgs);
    //! Append already quoted arguments to a shell command
    static void addArgs(QString *args, const QString &inArgs);
    //! Split a shell command into separate arguments.
    static QStringList splitArgs(const QString &cmd, OsType osType = HostOsInfo::hostOs(),
                                 bool abortOnMeta = false, SplitError *err = nullptr,
                                 const Environment *env = nullptr, const QString *pwd = nullptr);
    //! Safely replace the expandos in a shell command
    static bool expandMacros(QString *cmd, AbstractMacroExpander *mx,
                             OsType osType = HostOsInfo::hostOs());

    /*! Iterate over arguments from a command line.
     *  Assumes that the name of the actual command is *not* part of the line.
     *  Terminates after the first command if the command line is complex.
     */
    class QTCREATOR_UTILS_EXPORT ArgIterator {
    public:
        ArgIterator(QString *str, OsType osType = HostOsInfo::hostOs())
            : m_str(str), m_osType(osType)
        {}
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
        int m_pos = 0;
        int m_prev = -1;
        bool m_simple;
        OsType m_osType;
    };

    class QTCREATOR_UTILS_EXPORT ConstArgIterator {
    public:
        ConstArgIterator(const QString &str, OsType osType = HostOsInfo::hostOs())
            : m_str(str), m_ait(&m_str, osType)
        {}
        bool next() { return m_ait.next(); }
        bool isSimple() const { return m_ait.isSimple(); }
        QString value() const { return m_ait.value(); }
    private:
        QString m_str;
        ArgIterator m_ait;
    };

private:
    QString m_windowsArgs;
    QStringList m_unixArgs;
    bool m_isWindows;
};

class QTCREATOR_UTILS_EXPORT CommandLine
{
public:
    enum RawType { Raw };

    CommandLine();
    explicit CommandLine(const QString &executable);
    explicit CommandLine(const FilePath &executable);
    CommandLine(const QString &exe, const QStringList &args);
    CommandLine(const FilePath &exe, const QStringList &args);
    CommandLine(const FilePath &exe, const QString &unparsedArgs, RawType);

    void addArg(const QString &arg, OsType osType = HostOsInfo::hostOs());
    void addArgs(const QStringList &inArgs, OsType osType = HostOsInfo::hostOs());
    void addArgs(const CommandLine &cmd, OsType osType = HostOsInfo::hostOs());

    void addArgs(const QString &inArgs, RawType);

    QString toUserOutput() const;

    FilePath executable() const { return m_executable; }
    void setExecutable(const FilePath &executable) { m_executable = executable; }

    QString arguments() const { return m_arguments; }
    void setArguments(const QString &args) { m_arguments = args; }

    QStringList splitArguments(OsType osType = HostOsInfo::hostOs()) const;

    bool isEmpty() const { return m_executable.isEmpty(); }

    friend bool operator==(const CommandLine &first, const CommandLine &second) {
        return first.m_executable == second.m_executable && first.m_arguments == second.m_arguments;
    }

private:
    FilePath m_executable;
    QString m_arguments;
};

} // namespace Utils

Q_DECLARE_METATYPE(Utils::CommandLine)
