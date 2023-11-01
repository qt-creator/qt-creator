// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "hostosinfo.h"

#include <QPair>
#include <QStringList>

namespace Utils {

class AbstractMacroExpander;
class CommandLine;
class Environment;
class MacroExpander;

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
    static QStringList splitArgs(const QString &cmd, OsType osType,
                                 bool abortOnMeta = false, SplitError *err = nullptr,
                                 const Environment *env = nullptr, const QString *pwd = nullptr);
    //! Safely replace the expandos in a shell command
    static bool expandMacros(QString *cmd, AbstractMacroExpander *mx,
                             OsType osType = HostOsInfo::hostOs());

    /*! Iterate over arguments from a command line.
     *  Assumes that the name of the actual command is *not* part of the line.
     *  Terminates after the first command if the command line is complex.
     */
    class QTCREATOR_UTILS_EXPORT ArgIterator
    {
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

    class QTCREATOR_UTILS_EXPORT ConstArgIterator
    {
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
    ~CommandLine();

    explicit CommandLine(const FilePath &executable);
    CommandLine(const FilePath &exe, const QStringList &args);
    CommandLine(const FilePath &exe, const QStringList &args, OsType osType);
    CommandLine(const FilePath &exe, const QString &unparsedArgs, RawType);

    static CommandLine fromUserInput(const QString &cmdline, MacroExpander *expander = nullptr);

    void addArg(const QString &arg);
    void addArg(const QString &arg, OsType osType);
    void addMaskedArg(const QString &arg);
    void addMaskedArg(const QString &arg, OsType osType);
    void addArgs(const QStringList &inArgs);
    void addArgs(const QStringList &inArgs, OsType osType);
    void addArgs(const QString &inArgs, RawType);
    CommandLine &operator<<(const QString &arg);
    CommandLine &operator<<(const QStringList &arg);

    void prependArgs(const QStringList &inArgs);
    void prependArgs(const QString &inArgs, RawType);

    void addCommandLineAsArgs(const CommandLine &cmd);
    void addCommandLineAsArgs(const CommandLine &cmd, RawType);

    void addCommandLineAsSingleArg(const CommandLine &cmd);
    void addCommandLineWithAnd(const CommandLine &cmd);

    QString toUserOutput() const;
    QString displayName() const;

    FilePath executable() const { return m_executable; }
    void setExecutable(const FilePath &executable) { m_executable = executable; }

    QString arguments() const { return m_arguments; }
    void setArguments(const QString &args) { m_arguments = args; }

    QStringList splitArguments() const;

    bool isEmpty() const { return m_executable.isEmpty(); }

private:
    friend QTCREATOR_UTILS_EXPORT bool operator==(const CommandLine &first, const CommandLine &second);
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug dbg, const CommandLine &cmd);

    FilePath m_executable;
    QString m_arguments;
    QList<QPair<int, int>> m_masked;
};

} // Utils

Q_DECLARE_METATYPE(Utils::CommandLine)
