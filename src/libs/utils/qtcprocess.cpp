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

#include "qtcprocess.h"
#include "stringutils.h"

#include <QDir>
#include <QDebug>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

using namespace Utils;

/*!
    \class Utils::QtcProcess

    \brief This class provides functionality for dealing with shell-quoted process arguments.
*/

#ifdef Q_OS_WIN

inline static bool isMetaChar(ushort c)
{
    static const uchar iqm[] = {
        0x00, 0x00, 0x00, 0x00, 0x40, 0x03, 0x00, 0x50,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10
    }; // &()<>|

    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

static void envExpand(QString &args, const Environment *env, const QString *pwd)
{
    static const QString cdName = QLatin1String("CD");
    int off = 0;
  next:
    for (int prev = -1, that;
         (that = args.indexOf(QLatin1Char('%'), off)) >= 0;
         prev = that, off = that + 1) {
        if (prev >= 0) {
            const QString var = args.mid(prev + 1, that - prev - 1).toUpper();
            const QString val = (var == cdName && pwd && !pwd->isEmpty())
                                    ? QDir::toNativeSeparators(*pwd) : env->value(var);
            if (!val.isEmpty()) { // Empty values are impossible, so this is an existence check
                args.replace(prev, that - prev + 1, val);
                off = prev + val.length();
                goto next;
            }
        }
    }
}

QString QtcProcess::prepareArgs(const QString &_args, SplitError *err,
                                const Environment *env, const QString *pwd)
{
    QString args(_args);

    if (env) {
        envExpand(args, env, pwd);
    } else {
        if (args.indexOf(QLatin1Char('%')) >= 0) {
            if (err)
                *err = FoundMeta;
            return QString();
        }
    }

    if (!args.isEmpty() && args.unicode()[0].unicode() == '@')
        args.remove(0, 1);

    for (int p = 0; p < args.length(); p++) {
        ushort c = args.unicode()[p].unicode();
        if (c == '^') {
            args.remove(p, 1);
        } else if (c == '"') {
            do {
                if (++p == args.length())
                    break; // For cmd, this is no error.
            } while (args.unicode()[p].unicode() != '"');
        } else if (isMetaChar(c)) {
            if (err)
                *err = FoundMeta;
            return QString();
        }
    }

    if (err)
        *err = SplitOk;
    return args;
}

inline static bool isWhiteSpace(ushort c)
{
    return c == ' ' || c == '\t';
}

static QStringList doSplitArgs(const QString &args, QtcProcess::SplitError *err)
{
    QStringList ret;

    if (err)
        *err = QtcProcess::SplitOk;

    int p = 0;
    const int length = args.length();
    forever {
        forever {
            if (p == length)
                return ret;
            if (!isWhiteSpace(args.unicode()[p].unicode()))
                break;
            ++p;
        }

        QString arg;
        bool inquote = false;
        forever {
            bool copy = true; // copy this char
            int bslashes = 0; // number of preceding backslashes to insert
            while (p < length && args.unicode()[p] == QLatin1Char('\\')) {
                ++p;
                ++bslashes;
            }
            if (p < length && args.unicode()[p] == QLatin1Char('"')) {
                if (!(bslashes & 1)) {
                    // Even number of backslashes, so the quote is not escaped.
                    if (inquote) {
                        if (p + 1 < length && args.unicode()[p + 1] == QLatin1Char('"')) {
                            // This is not documented on MSDN.
                            // Two consecutive quotes make a literal quote. Brain damage:
                            // this still closes the quoting, so a 3rd quote is required,
                            // which makes the runtime's quoting run out of sync with the
                            // shell's one unless the 2nd quote is escaped.
                            ++p;
                        } else {
                            // Closing quote
                            copy = false;
                        }
                        inquote = false;
                    } else {
                        // Opening quote
                        copy = false;
                        inquote = true;
                    }
                }
                bslashes >>= 1;
            }

            while (--bslashes >= 0)
                arg.append(QLatin1Char('\\'));

            if (p == length || (!inquote && isWhiteSpace(args.unicode()[p].unicode()))) {
                ret.append(arg);
                if (inquote) {
                    if (err)
                        *err = QtcProcess::BadQuoting;
                    return QStringList();
                }
                break;
            }

            if (copy)
                arg.append(args.unicode()[p]);
            ++p;
        }
    }
    //not reached
}

/*!
    Splits \a args according to system shell word splitting and quoting rules.

    \section1 Unix

    The behavior is based on the POSIX shell and bash:
    \list
    \i Whitespace splits tokens
    \i The backslash quotes the following character
    \i A string enclosed in single quotes is not split. No shell meta
        characters are interpreted.
    \i A string enclosed in double quotes is not split. Within the string,
        the backslash quotes shell meta characters - if it is followed
        by a "meaningless" character, the backslash is output verbatim.
    \endlist
    If \a abortOnMeta is \c false, only the splitting and quoting rules apply,
    while other meta characters (substitutions, redirections, etc.) are ignored.
    If \a abortOnMeta is \c true, encounters of unhandled meta characters are
    treated as errors.

    \section1 Windows

    The behavior is defined by the Microsoft C runtime:
    \list
    \i Whitespace splits tokens
    \i A string enclosed in double quotes is not split
    \list
        \i 3N double quotes within a quoted string yield N literal quotes.
           This is not documented on MSDN.
    \endlist
    \i Backslashes have special semantics iff they are followed by a double quote:
    \list
        \i 2N backslashes + double quote => N backslashes and begin/end quoting
        \i 2N+1 backslashes + double quote => N backslashes + literal quote
    \endlist
    \endlist
    Qt and many other implementations comply with this standard, but many do not.

    If \a abortOnMeta is \c true, cmd shell semantics are applied before
    proceeding with word splitting:
    \list
    \i Cmd ignores \e all special chars between double quotes.
        Note that the quotes are \e not removed at this stage - the
        tokenization rules described above still apply.
    \i The \c circumflex is the escape char for everything including itself.
    \endlist
    As the quoting levels are independent from each other and have different
    semantics, you need a command line like \c{"foo "\^"" bar"} to get
    \c{foo " bar}.

    \param cmd the command to split
    \param abortOnMeta see above
    \param err if not NULL, a status code will be stored at the pointer
    target, see \l SplitError
    \param env if not NULL, perform variable substitution with the
    given environment.
   \return a list of unquoted words or an empty list if an error occurred
 */

QStringList QtcProcess::splitArgs(const QString &_args, bool abortOnMeta, SplitError *err,
                                  const Environment *env, const QString *pwd)
{
    if (abortOnMeta) {
        SplitError perr;
        if (!err)
            err = &perr;
        QString args = prepareArgs(_args, &perr, env, pwd);
        if (*err != SplitOk)
            return QStringList();
        return doSplitArgs(args, err);
    } else {
        QString args = _args;
        if (env)
            envExpand(args, env, pwd);
        return doSplitArgs(args, err);
    }
}

#else // Q_OS_WIN

inline static bool isQuoteMeta(QChar cUnicode)
{
    char c = cUnicode.toLatin1();
    return c == '\\' || c == '\'' || c == '"' || c == '$';
}

inline static bool isMeta(QChar cUnicode)
{
    static const uchar iqm[] = {
        0x00, 0x00, 0x00, 0x00, 0xdc, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x38
    }; // \'"$`<>|;&(){}*?#[]

    uint c = cUnicode.unicode();

    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

QStringList QtcProcess::splitArgs(const QString &args, bool abortOnMeta, SplitError *err,
                                  const Environment *env, const QString *pwd)
{
    static const QString pwdName = QLatin1String("PWD");
    QStringList ret;

    for (int pos = 0; ; ) {
        QChar c;
        do {
            if (pos >= args.length())
                goto okret;
            c = args.unicode()[pos++];
        } while (c.isSpace());
        QString cret;
        bool hadWord = false;
        if (c == QLatin1Char('~')) {
            if (pos >= args.length()
                || args.unicode()[pos].isSpace() || args.unicode()[pos] == QLatin1Char('/')) {
                cret = QDir::homePath();
                hadWord = true;
                goto getc;
            } else if (abortOnMeta) {
                goto metaerr;
            }
        }
        do {
            if (c == QLatin1Char('\'')) {
                int spos = pos;
                do {
                    if (pos >= args.length())
                        goto quoteerr;
                    c = args.unicode()[pos++];
                } while (c != QLatin1Char('\''));
                cret += args.mid(spos, pos-spos-1);
                hadWord = true;
            } else if (c == QLatin1Char('"')) {
                for (;;) {
                    if (pos >= args.length())
                        goto quoteerr;
                    c = args.unicode()[pos++];
                  nextq:
                    if (c == QLatin1Char('"'))
                        break;
                    if (c == QLatin1Char('\\')) {
                        if (pos >= args.length())
                            goto quoteerr;
                        c = args.unicode()[pos++];
                        if (c != QLatin1Char('"') &&
                            c != QLatin1Char('\\') &&
                            !(abortOnMeta &&
                              (c == QLatin1Char('$') ||
                               c == QLatin1Char('`'))))
                            cret += QLatin1Char('\\');
                    } else if (c == QLatin1Char('$') && env) {
                        if (pos >= args.length())
                            goto quoteerr;
                        c = args.unicode()[pos++];
                        bool braced = false;
                        if (c == QLatin1Char('{')) {
                            if (pos >= args.length())
                                goto quoteerr;
                            c = args.unicode()[pos++];
                            braced = true;
                        }
                        QString var;
                        while (c.isLetterOrNumber() || c == QLatin1Char('_')) {
                            var += c;
                            if (pos >= args.length())
                                goto quoteerr;
                            c = args.unicode()[pos++];
                        }
                        if (var == pwdName && pwd && !pwd->isEmpty()) {
                            cret += *pwd;
                        } else {
                            Environment::const_iterator vit = env->constFind(var);
                            if (vit == env->constEnd()) {
                                if (abortOnMeta)
                                    goto metaerr; // Assume this is a shell builtin
                            } else {
                                cret += *vit;
                            }
                        }
                        if (!braced)
                            goto nextq;
                        if (c != QLatin1Char('}')) {
                            if (abortOnMeta)
                                goto metaerr; // Assume this is a complex expansion
                            goto quoteerr; // Otherwise it's just garbage
                        }
                        continue;
                    } else if (abortOnMeta &&
                               (c == QLatin1Char('$') ||
                                c == QLatin1Char('`'))) {
                        goto metaerr;
                    }
                    cret += c;
                }
                hadWord = true;
            } else if (c == QLatin1Char('$') && env) {
                if (pos >= args.length())
                    goto quoteerr; // Bash just takes it verbatim, but whatever
                c = args.unicode()[pos++];
                bool braced = false;
                if (c == QLatin1Char('{')) {
                    if (pos >= args.length())
                        goto quoteerr;
                    c = args.unicode()[pos++];
                    braced = true;
                }
                QString var;
                while (c.isLetterOrNumber() || c == QLatin1Char('_')) {
                    var += c;
                    if (pos >= args.length()) {
                        if (braced)
                            goto quoteerr;
                        c = QLatin1Char(' ');
                        break;
                    }
                    c = args.unicode()[pos++];
                }
                QString val;
                if (var == pwdName && pwd && !pwd->isEmpty()) {
                    val = *pwd;
                } else {
                    Environment::const_iterator vit = env->constFind(var);
                    if (vit == env->constEnd()) {
                        if (abortOnMeta)
                            goto metaerr; // Assume this is a shell builtin
                    } else {
                        val = *vit;
                    }
                }
                for (int i = 0; i < val.length(); i++) {
                    QChar cc = val.unicode()[i];
                    if (cc == 9 || cc == 10 || cc == 32) {
                        if (hadWord) {
                            ret += cret;
                            cret.clear();
                            hadWord = false;
                        }
                    } else {
                        cret += cc;
                        hadWord = true;
                    }
                }
                if (!braced)
                    goto nextc;
                if (c != QLatin1Char('}')) {
                    if (abortOnMeta)
                        goto metaerr; // Assume this is a complex expansion
                    goto quoteerr; // Otherwise it's just garbage
                }
            } else {
                if (c == QLatin1Char('\\')) {
                    if (pos >= args.length())
                        goto quoteerr;
                    c = args.unicode()[pos++];
                } else if (abortOnMeta && isMeta(c)) {
                    goto metaerr;
                }
                cret += c;
                hadWord = true;
            }
          getc:
            if (pos >= args.length())
                break;
            c = args.unicode()[pos++];
          nextc: ;
        } while (!c.isSpace());
        if (hadWord)
            ret += cret;
    }

  okret:
    if (err)
        *err = SplitOk;
    return ret;

  quoteerr:
    if (err)
        *err = BadQuoting;
    return QStringList();

  metaerr:
    if (err)
        *err = FoundMeta;
    return QStringList();
}

#endif // Q_OS_WIN

inline static bool isSpecialCharUnix(ushort c)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

inline static bool hasSpecialCharsUnix(const QString &arg)
{
    for (int x = arg.length() - 1; x >= 0; --x)
        if (isSpecialCharUnix(arg.unicode()[x].unicode()))
            return true;
    return false;
}

QString QtcProcess::quoteArgUnix(const QString &arg)
{
    if (!arg.length())
        return QString::fromLatin1("''");

    QString ret(arg);
    if (hasSpecialCharsUnix(ret)) {
        ret.replace(QLatin1Char('\''), QLatin1String("'\\''"));
        ret.prepend(QLatin1Char('\''));
        ret.append(QLatin1Char('\''));
    }
    return ret;
}

void QtcProcess::addArgUnix(QString *args, const QString &arg)
{
    if (!args->isEmpty())
        *args += QLatin1Char(' ');
    *args += quoteArgUnix(arg);
}

QString QtcProcess::joinArgsUnix(const QStringList &args)
{
    QString ret;
    foreach (const QString &arg, args)
        addArgUnix(&ret, arg);
    return ret;
}

#ifdef Q_OS_WIN
inline static bool isSpecialChar(ushort c)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars "&()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x45, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };

    return (c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7)));
}

inline static bool hasSpecialChars(const QString &arg)
{
    for (int x = arg.length() - 1; x >= 0; --x)
        if (isSpecialChar(arg.unicode()[x].unicode()))
            return true;
    return false;
}

QString QtcProcess::quoteArg(const QString &arg)
{
    if (!arg.length())
        return QString::fromLatin1("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret)) {
        // Quotes are escaped and their preceding backslashes are doubled.
        // It's impossible to escape anything inside a quoted string on cmd
        // level, so the outer quoting must be "suspended".
        ret.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\"\\1\\1\\^\"\""));
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    // FIXME: Without this, quoting is not foolproof. But it needs support in the process setup, etc.
    //ret.replace('%', QLatin1String("%PERCENT_SIGN%"));
    return ret;
}

void QtcProcess::addArg(QString *args, const QString &arg)
{
    if (!args->isEmpty())
        *args += QLatin1Char(' ');
    *args += quoteArg(arg);
}

QString QtcProcess::joinArgs(const QStringList &args)
{
    QString ret;
    foreach (const QString &arg, args)
        addArg(&ret, arg);
    return ret;
}
#endif

void QtcProcess::addArgs(QString *args, const QString &inArgs)
{
    if (!inArgs.isEmpty()) {
        if (!args->isEmpty())
            *args += QLatin1Char(' ');
        *args += inArgs;
    }
}

void QtcProcess::addArgs(QString *args, const QStringList &inArgs)
{
    foreach (const QString &arg, inArgs)
        addArg(args, arg);
}

#ifdef Q_OS_WIN
void QtcProcess::prepareCommand(const QString &command, const QString &arguments,
                                QString *outCmd, QString *outArgs,
                                const Environment *env, const QString *pwd)
{
    QtcProcess::SplitError err;
    *outArgs = QtcProcess::prepareArgs(arguments, &err, env, pwd);
    if (err == QtcProcess::SplitOk) {
        *outCmd = command;
    } else {
        *outCmd = QString::fromLatin1(qgetenv("COMSPEC"));
        *outArgs = QLatin1String("/v:off /s /c \"")
                + quoteArg(QDir::toNativeSeparators(command)) + QLatin1Char(' ') + arguments
                + QLatin1Char('"');
    }
}
#else
bool QtcProcess::prepareCommand(const QString &command, const QString &arguments,
                                QString *outCmd, QStringList *outArgs,
                                const Environment *env, const QString *pwd)
{
    QtcProcess::SplitError err;
    *outArgs = QtcProcess::prepareArgs(arguments, &err, env, pwd);
    if (err == QtcProcess::SplitOk) {
        *outCmd = command;
    } else {
        if (err != QtcProcess::FoundMeta)
            return false;
        *outCmd = QLatin1String("/bin/sh");
        *outArgs << QLatin1String("-c") << (quoteArg(command) + QLatin1Char(' ') + arguments);
    }
    return true;
}
#endif

void QtcProcess::start()
{
    Environment env;
    if (m_haveEnv) {
        if (m_environment.size() == 0)
            qWarning("QtcProcess::start: Empty environment set when running '%s'.", qPrintable(m_command));
        env = m_environment;

        // If the process environemnt has no libraryPath,
        // Qt will copy creator's libraryPath into the process environment.
        // That's brain dead, and we work around it
#if defined(Q_OS_UNIX)
#  if defined(Q_OS_MAC)
        static const char libraryPathC[] = "DYLD_LIBRARY_PATH";
#  else
        static const char libraryPathC[] = "LD_LIBRARY_PATH";
#  endif
        const QString libraryPath = QLatin1String(libraryPathC);
        if (env.constFind(libraryPath) == env.constEnd())
            env.set(libraryPath, QString());
#endif
        QProcess::setEnvironment(env.toStringList());
    } else {
        env = Environment::systemEnvironment();
    }

    const QString &workDir = workingDirectory();
    QString command;
#ifdef Q_OS_WIN
    QString arguments;
    QStringList argList;
    prepareCommand(m_command, m_arguments, &command, &arguments, &env, &workDir);
    setNativeArguments(arguments);
    if (m_useCtrlCStub) {
        argList << QDir::toNativeSeparators(command);
        command = QCoreApplication::applicationDirPath() + QLatin1String("/qtcreator_ctrlc_stub.exe");
    }
    QProcess::start(command, argList);
#else
    QStringList arguments;
    if (!prepareCommand(m_command, m_arguments, &command, &arguments, &env, &workDir)) {
        setErrorString(tr("Error in command line."));
        // Should be FailedToStart, but we cannot set the process error from the outside,
        // so it would be inconsistent.
        emit error(QProcess::UnknownError);
        return;
    }
    QProcess::start(command, arguments);
#endif
}

#ifdef Q_OS_WIN
BOOL CALLBACK sendShutDownMessageToAllWindowsOfProcess_enumWnd(HWND hwnd, LPARAM lParam)
{
    static UINT uiShutDownMessage = RegisterWindowMessage(L"qtcctrlcstub_shutdown");
    DWORD dwProcessID;
    GetWindowThreadProcessId(hwnd, &dwProcessID);
    if ((DWORD)lParam == dwProcessID) {
        SendNotifyMessage(hwnd, uiShutDownMessage, 0, 0);
        return FALSE;
    }
    return TRUE;
}
#endif

void QtcProcess::terminate()
{
#ifdef Q_OS_WIN
    if (m_useCtrlCStub)
        EnumWindows(sendShutDownMessageToAllWindowsOfProcess_enumWnd, pid()->dwProcessId);
    else
#endif
    QProcess::terminate();
}

#ifdef Q_OS_WIN

// This function assumes that the resulting string will be quoted.
// That's irrelevant if it does not contain quotes itself.
static int quoteArgInternal(QString &ret, int bslashes)
{
    // Quotes are escaped and their preceding backslashes are doubled.
    // It's impossible to escape anything inside a quoted string on cmd
    // level, so the outer quoting must be "suspended".
    const QChar bs(QLatin1Char('\\')), dq(QLatin1Char('"'));
    for (int p = 0; p < ret.length(); p++) {
        if (ret.at(p) == bs) {
            bslashes++;
        } else {
            if (ret.at(p) == dq) {
                if (bslashes) {
                    ret.insert(p, QString(bslashes, bs));
                    p += bslashes;
                }
                ret.insert(p, QLatin1String("\"\\^\""));
                p += 4;
            }
            bslashes = 0;
        }
    }
    return bslashes;
}

#else

// The main state of the Unix shell parser
enum MxQuoting { MxBasic, MxSingleQuote, MxDoubleQuote, MxParen, MxSubst, MxGroup, MxMath };
typedef struct {
    MxQuoting current;
    // Bizarrely enough, double quoting has an impact on the behavior of some
    // complex expressions within the quoted string.
    bool dquote;
} MxState;
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(MxState, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

// Pushed state for the case where a $(()) expansion turns out bogus
typedef struct {
    QString str;
    int pos, varPos;
} MxSave;
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(MxSave, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

#include <QStack>

#endif

// TODO: This documentation is relevant for end-users. Where to put it?
/**
 * Perform safe macro expansion (substitution) on a string for use
 * in shell commands.
 *
 * \section Unix notes
 *
 * Explicitly supported shell constructs:
 *   \\ '' "" {} () $(()) ${} $() ``
 *
 * Implicitly supported shell constructs:
 *   (())
 *
 * Unsupported shell constructs that will cause problems:
 * \list
 * \li Shortened \c{case $v in pat)} syntax. Use \c{case $v in (pat)} instead.
 * \li Bash-style \c{$""} and \c{$''} string quoting syntax.
 * \endlist
 *
 * The rest of the shell (incl. bash) syntax is simply ignored,
 * as it is not expected to cause problems.
 *
 * Security considerations:
 * \list
 * \li Backslash-escaping an expando is treated as a quoting error
 * \li Do not put expandos into double quoted substitutions:
 *     \badcode
 *       "${VAR:-%{macro}}"
 *     \endcode
 * \li Do not put expandos into command line arguments which are nested
 *     shell commands:
 *     \badcode
 *       sh -c 'foo \%{file}'
 *     \endcode
 *     \goodcode
 *       file=\%{file} sh -c 'foo "$file"'
 *     \endcode
 * \endlist
 *
 * \section Windows notes
 *
 * All quoting syntax supported by splitArgs() is supported here as well.
 * Additionally, command grouping via parentheses is recognized - note
 * however, that the parser is much stricter about unquoted parentheses
 * than cmd itself.
 * The rest of the cmd syntax is simply ignored, as it is not expected
 * to cause problems.
 *
 * Security considerations:
 * \list
 * \li Circumflex-escaping an expando is treated as a quoting error
 * \li Closing double quotes right before expandos and opening double quotes
 *     right after expandos are treated as quoting errors
 * \li Do not put expandos into nested commands:
 *     \badcode
 *       for /f "usebackq" \%v in (`foo \%{file}`) do \@echo \%v
 *     \endcode
 * \li A macro's value must not contain anything which may be interpreted
 *     as an environment variable expansion. A solution is replacing any
 *     percent signs with a fixed string like \c{\%PERCENT_SIGN\%} and
 *     injecting \c{PERCENT_SIGN=\%} into the shell's environment.
 * \li Enabling delayed environment variable expansion (cmd /v:on) should have
 *     no security implications, but may still wreak havoc due to the
 *     need for doubling circumflexes if any exclamation marks are present,
 *     and the need to circumflex-escape the exclamation marks themselves.
 * \endlist
 *
 * \param cmd pointer to the string in which macros are expanded in-place
 * \param mx pointer to a macro expander instance
 * \return false if the string could not be parsed and therefore no safe
 *   substitution was possible
 */
bool QtcProcess::expandMacros(QString *cmd, AbstractMacroExpander *mx)
{
    QString str = *cmd;
    if (str.isEmpty())
        return true;

    QString rsts;
    int varLen;
    int varPos = 0;
    if (!(varLen = mx->findMacro(str, &varPos, &rsts)))
        return true;

    int pos = 0;

#ifdef Q_OS_WIN
    enum { // cmd.exe parsing state
        ShellBasic, // initial state
        ShellQuoted, // double-quoted state => *no* other meta chars are interpreted
        ShellEscaped // circumflex-escaped state => next char is not interpreted
    } shellState = ShellBasic;
    enum { // CommandLineToArgv() parsing state and some more
        CrtBasic, // initial state
        CrtNeedWord, // after empty expando; insert empty argument if whitespace follows
        CrtInWord, // in non-whitespace
        CrtClosed, // previous char closed the double-quoting
        CrtHadQuote, // closed double-quoting after an expando
        // The remaining two need to be numerically higher
        CrtQuoted, // double-quoted state => spaces don't split tokens
        CrtNeedQuote // expando opened quote; close if no expando follows
    } crtState = CrtBasic;
    int bslashes = 0; // previous chars were manual backslashes
    int rbslashes = 0; // trailing backslashes in replacement

    forever {
        if (pos == varPos) {
            if (shellState == ShellEscaped)
                return false; // Circumflex'd quoted expando would be Bad (TM).
            if ((shellState == ShellQuoted) != (crtState == CrtQuoted))
                return false; // CRT quoting out of sync with shell quoting. Ahoy to Redmond.
            rbslashes += bslashes;
            bslashes = 0;
            if (crtState < CrtQuoted) {
                if (rsts.isEmpty()) {
                    if (crtState == CrtBasic) {
                        // Outside any quoting and the string is empty, so put
                        // a pair of quotes. Delaying that is just pedantry.
                        crtState = CrtNeedWord;
                    }
                } else {
                    if (hasSpecialChars(rsts)) {
                        if (crtState == CrtClosed) {
                            // Quoted expando right after closing quote. Can't do that.
                            return false;
                        }
                        int tbslashes = quoteArgInternal(rsts, 0);
                        rsts.prepend(QLatin1Char('"'));
                        if (rbslashes)
                            rsts.prepend(QString(rbslashes, QLatin1Char('\\')));
                        crtState = CrtNeedQuote;
                        rbslashes = tbslashes;
                    } else {
                        crtState = CrtInWord; // We know that this string contains no spaces.
                        // We know that this string contains no quotes,
                        // so the function won't make a mess.
                        rbslashes = quoteArgInternal(rsts, rbslashes);
                    }
                }
            } else {
                rbslashes = quoteArgInternal(rsts, rbslashes);
            }
            str.replace(pos, varLen, rsts);
            pos += rsts.length();
            varPos = pos;
            if (!(varLen = mx->findMacro(str, &varPos, &rsts))) {
                // Don't leave immediately, as we may be in CrtNeedWord state which could
                // be still resolved, or we may have inserted trailing backslashes.
                varPos = INT_MAX;
            }
            continue;
        }
        if (crtState == CrtNeedQuote) {
            if (rbslashes) {
                str.insert(pos, QString(rbslashes, QLatin1Char('\\')));
                pos += rbslashes;
                varPos += rbslashes;
                rbslashes = 0;
            }
            str.insert(pos, QLatin1Char('"'));
            pos++;
            varPos++;
            crtState = CrtHadQuote;
        }
        ushort cc = str.unicode()[pos].unicode();
        if (shellState == ShellBasic && cc == '^') {
            shellState = ShellEscaped;
        } else {
            if (!cc || cc == ' ' || cc == '\t') {
                if (crtState < CrtQuoted) {
                    if (crtState == CrtNeedWord) {
                        str.insert(pos, QLatin1String("\"\""));
                        pos += 2;
                        varPos += 2;
                    }
                    crtState = CrtBasic;
                }
                if (!cc)
                    break;
                bslashes = 0;
                rbslashes = 0;
            } else {
                if (cc == '\\') {
                    bslashes++;
                    if (crtState < CrtQuoted)
                        crtState = CrtInWord;
                } else {
                    if (cc == '"') {
                        if (shellState != ShellEscaped)
                            shellState = (shellState == ShellQuoted) ? ShellBasic : ShellQuoted;
                        if (rbslashes) {
                            // Offset -1: skip possible circumflex. We have at least
                            // one backslash, so a fixed offset is ok.
                            str.insert(pos - 1, QString(rbslashes, QLatin1Char('\\')));
                            pos += rbslashes;
                            varPos += rbslashes;
                        }
                        if (!(bslashes & 1)) {
                            // Even number of backslashes, so the quote is not escaped.
                            switch (crtState) {
                            case CrtQuoted:
                                // Closing quote
                                crtState = CrtClosed;
                                break;
                            case CrtClosed:
                                // Two consecutive quotes make a literal quote - and
                                // still close quoting. See QtcProcess::quoteArg().
                                crtState = CrtInWord;
                                break;
                            case CrtHadQuote:
                                // Opening quote right after quoted expando. Can't do that.
                                return false;
                            default:
                                // Opening quote
                                crtState = CrtQuoted;
                                break;
                            }
                        } else if (crtState < CrtQuoted) {
                            crtState = CrtInWord;
                        }
                    } else if (crtState < CrtQuoted) {
                        crtState = CrtInWord;
                    }
                    bslashes = 0;
                    rbslashes = 0;
                }
            }
            if (varPos == INT_MAX && !rbslashes)
                break;
            if (shellState == ShellEscaped)
                shellState = ShellBasic;
        }
        pos++;
    }
#else
    MxState state = { MxBasic, false };
    QStack<MxState> sstack;
    QStack<MxSave> ostack;

    while (pos < str.length()) {
        if (pos == varPos) {
            // Our expansion rules trigger in any context
            if (state.dquote) {
                // We are within a double-quoted string. Escape relevant meta characters.
                rsts.replace(QRegExp(QLatin1String("([$`\"\\\\])")), QLatin1String("\\\\1"));
            } else if (state.current == MxSingleQuote) {
                // We are within a single-quoted string. "Suspend" single-quoting and put a
                // single escaped quote for each single quote inside the string.
                rsts.replace(QLatin1Char('\''), QLatin1String("'\\''"));
            } else if (rsts.isEmpty() || hasSpecialCharsUnix(rsts)) {
                // String contains "quote-worthy" characters. Use single quoting - but
                // that choice is arbitrary.
                rsts.replace(QLatin1Char('\''), QLatin1String("'\\''"));
                rsts.prepend(QLatin1Char('\''));
                rsts.append(QLatin1Char('\''));
            } // Else just use the string verbatim.
            str.replace(pos, varLen, rsts);
            pos += rsts.length();
            varPos = pos;
            if (!(varLen = mx->findMacro(str, &varPos, &rsts)))
                break;
            continue;
        }
        ushort cc = str.unicode()[pos].unicode();
        if (state.current == MxSingleQuote) {
            // Single quoted context - only the single quote has any special meaning.
            if (cc == '\'')
                state = sstack.pop();
        } else if (cc == '\\') {
            // In any other context, the backslash starts an escape.
            pos += 2;
            if (varPos < pos)
                return false; // Backslash'd quoted expando would be Bad (TM).
            continue;
        } else if (cc == '$') {
            cc = str.unicode()[++pos].unicode();
            if (cc == '(') {
                sstack.push(state);
                if (str.unicode()[pos + 1].unicode() == '(') {
                    // $(( starts a math expression. This may also be a $( ( in fact,
                    // so we push the current string and offset on a stack so we can retry.
                    MxSave sav = { str, pos + 2, varPos };
                    ostack.push(sav);
                    state.current = MxMath;
                    pos += 2;
                    continue;
                } else {
                    // $( starts a command substitution. This actually "opens a new context"
                    // which overrides surrounding double quoting.
                    state.current = MxParen;
                    state.dquote = false;
                }
            } else if (cc == '{') {
                // ${ starts a "braced" variable substitution.
                sstack.push(state);
                state.current = MxSubst;
            } // Else assume that a "bare" variable substitution has started
        } else if (cc == '`') {
            // Backticks are evil, as every shell interprets escapes within them differently,
            // which is a danger for the quoting of our own expansions.
            // So we just apply *our* rules (which match bash) and transform it into a POSIX
            // command substitution which has clear semantics.
            str.replace(pos, 1, QLatin1String("$( " )); // add space -> avoid creating $((
            varPos += 2;
            int pos2 = pos += 3;
            forever {
                if (pos2 >= str.length())
                    return false; // Syntax error - unterminated backtick expression.
                cc = str.unicode()[pos2].unicode();
                if (cc == '`')
                    break;
                if (cc == '\\') {
                    cc = str.unicode()[++pos2].unicode();
                    if (cc == '$' || cc == '`' || cc == '\\' ||
                        (cc == '"' && state.dquote))
                    {
                        str.remove(pos2 - 1, 1);
                        if (varPos >= pos2)
                            varPos--;
                        continue;
                    }
                }
                pos2++;
            }
            str[pos2] = QLatin1Char(')');
            sstack.push(state);
            state.current = MxParen;
            state.dquote = false;
            continue;
        } else if (state.current == MxDoubleQuote) {
            // (Truly) double quoted context - only remaining special char is the closing quote.
            if (cc == '"')
                state = sstack.pop();
        } else if (cc == '\'') {
            // Start single quote if we are not in "inherited" double quoted context.
            if (!state.dquote) {
                sstack.push(state);
                state.current = MxSingleQuote;
            }
        } else if (cc == '"') {
            // Same for double quoting.
            if (!state.dquote) {
                sstack.push(state);
                state.current = MxDoubleQuote;
                state.dquote = true;
            }
        } else if (state.current == MxSubst) {
            // "Braced" substitution context - only remaining special char is the closing brace.
            if (cc == '}')
                state = sstack.pop();
        } else if (cc == ')') {
            if (state.current == MxMath) {
                if (str.unicode()[pos + 1].unicode() == ')') {
                    state = sstack.pop();
                    pos += 2;
                } else {
                    // False hit: the $(( was a $( ( in fact.
                    // ash does not care (and will complain), but bash actually parses it.
                    varPos = ostack.top().varPos;
                    pos = ostack.top().pos;
                    str = ostack.top().str;
                    ostack.pop();
                    state.current = MxParen;
                    state.dquote = false;
                    sstack.push(state);
                }
                continue;
            } else if (state.current == MxParen) {
                state = sstack.pop();
            } else {
                break; // Syntax error - excess closing parenthesis.
            }
        } else if (cc == '}') {
            if (state.current == MxGroup)
                state = sstack.pop();
            else
                break; // Syntax error - excess closing brace.
        } else if (cc == '(') {
            // Context-saving command grouping.
            sstack.push(state);
            state.current = MxParen;
        } else if (cc == '{') {
            // Plain command grouping.
            sstack.push(state);
            state.current = MxGroup;
        }
        pos++;
    }
    // FIXME? May complain if (!sstack.empty()), but we don't really care anyway.
#endif

    *cmd = str;
    return true;
}

QString QtcProcess::expandMacros(const QString &str, AbstractMacroExpander *mx)
{
    QString ret = str;
    expandMacros(&ret, mx);
    return ret;
}

bool QtcProcess::ArgIterator::next()
{
    // We delay the setting of m_prev so we can still delete the last argument
    // after we find that there are no more arguments. It's a bit of a hack ...
    int prev = m_pos;

    m_simple = true;
    m_value.clear();

#ifdef Q_OS_WIN
    enum { // cmd.exe parsing state
        ShellBasic, // initial state
        ShellQuoted, // double-quoted state => *no* other meta chars are interpreted
        ShellEscaped // circumflex-escaped state => next char is not interpreted
    } shellState = ShellBasic;
    enum { // CommandLineToArgv() parsing state and some more
        CrtBasic, // initial state
        CrtInWord, // in non-whitespace
        CrtClosed, // previous char closed the double-quoting
        CrtQuoted // double-quoted state => spaces don't split tokens
    } crtState = CrtBasic;
    enum { NoVar, NewVar, FullVar } varState = NoVar; // inside a potential env variable expansion
    int bslashes = 0; // number of preceding backslashes

    for (;; m_pos++) {
        ushort cc = m_pos < m_str->length() ? m_str->unicode()[m_pos].unicode() : 0;
        if (shellState == ShellBasic && cc == '^') {
            varState = NoVar;
            shellState = ShellEscaped;
        } else if ((shellState == ShellBasic && isMetaChar(cc)) || !cc) { // A "bit" simplistic ...
            // We ignore crtQuote state here. Whatever ...
          doReturn:
            if (m_simple)
                while (--bslashes >= 0)
                    m_value += QLatin1Char('\\');
            else
                m_value.clear();
            if (crtState != CrtBasic) {
                m_prev = prev;
                return true;
            }
            return false;
        } else {
            if (crtState != CrtQuoted && (cc == ' ' || cc == '\t')) {
                if (crtState != CrtBasic) {
                    // We'll lose shellQuote state here. Whatever ...
                    goto doReturn;
                }
            } else {
                if (cc == '\\') {
                    bslashes++;
                    if (crtState != CrtQuoted)
                        crtState = CrtInWord;
                    varState = NoVar;
                } else {
                    if (cc == '"') {
                        varState = NoVar;
                        if (shellState != ShellEscaped)
                            shellState = (shellState == ShellQuoted) ? ShellBasic : ShellQuoted;
                        int obslashes = bslashes;
                        bslashes >>= 1;
                        if (!(obslashes & 1)) {
                            // Even number of backslashes, so the quote is not escaped.
                            switch (crtState) {
                            case CrtQuoted:
                                // Closing quote
                                crtState = CrtClosed;
                                continue;
                            case CrtClosed:
                                // Two consecutive quotes make a literal quote - and
                                // still close quoting. See quoteArg().
                                crtState = CrtInWord;
                                break;
                            default:
                                // Opening quote
                                crtState = CrtQuoted;
                                continue;
                            }
                        } else if (crtState != CrtQuoted) {
                            crtState = CrtInWord;
                        }
                    } else {
                        if (cc == '%') {
                            if (varState == FullVar) {
                                m_simple = false;
                                varState = NoVar;
                            } else {
                                varState = NewVar;
                            }
                        } else if (varState != NoVar) {
                            // This check doesn't really reflect cmd reality, but it is an
                            // approximation of what would be sane.
                            varState = (cc == '_' || cc == '-' || cc == '.'
                                     || QChar(cc).isLetterOrNumber()) ? FullVar : NoVar;

                        }
                        if (crtState != CrtQuoted)
                            crtState = CrtInWord;
                    }
                    for (; bslashes > 0; bslashes--)
                        m_value += QLatin1Char('\\');
                    m_value += QChar(cc);
                }
            }
            if (shellState == ShellEscaped)
                shellState = ShellBasic;
        }
    }
#else
    MxState state = { MxBasic, false };
    QStack<MxState> sstack;
    QStack<int> ostack;
    bool hadWord = false;

    for (; m_pos < m_str->length(); m_pos++) {
        ushort cc = m_str->unicode()[m_pos].unicode();
        if (state.current == MxSingleQuote) {
            if (cc == '\'') {
                state = sstack.pop();
                continue;
            }
        } else if (cc == '\\') {
            if (++m_pos >= m_str->length())
                break;
            cc = m_str->unicode()[m_pos].unicode();
            if (state.dquote && cc != '"' && cc != '\\' && cc != '$' && cc != '`')
                m_value += QLatin1Char('\\');
        } else if (cc == '$') {
            if (++m_pos >= m_str->length())
                break;
            cc = m_str->unicode()[m_pos].unicode();
            if (cc == '(') {
                sstack.push(state);
                if (++m_pos >= m_str->length())
                    break;
                if (m_str->unicode()[m_pos].unicode() == '(') {
                    ostack.push(m_pos);
                    state.current = MxMath;
                } else {
                    state.dquote = false;
                    state.current = MxParen;
                    // m_pos too far by one now - whatever.
                }
            } else if (cc == '{') {
                sstack.push(state);
                state.current = MxSubst;
            } else {
                // m_pos too far by one now - whatever.
            }
            m_simple = false;
            hadWord = true;
            continue;
        } else if (cc == '`') {
            forever {
                if (++m_pos >= m_str->length()) {
                    m_simple = false;
                    m_prev = prev;
                    return true;
                }
                cc = m_str->unicode()[m_pos].unicode();
                if (cc == '`')
                    break;
                if (cc == '\\')
                    m_pos++; // m_pos may be too far by one now - whatever.
            }
            m_simple = false;
            hadWord = true;
            continue;
        } else if (state.current == MxDoubleQuote) {
            if (cc == '"') {
                state = sstack.pop();
                continue;
            }
        } else if (cc == '\'') {
            if (!state.dquote) {
                sstack.push(state);
                state.current = MxSingleQuote;
                hadWord = true;
                continue;
            }
        } else if (cc == '"') {
            if (!state.dquote) {
                sstack.push(state);
                state.dquote = true;
                state.current = MxDoubleQuote;
                hadWord = true;
                continue;
            }
        } else if (state.current == MxSubst) {
            if (cc == '}')
                state = sstack.pop();
            continue; // Not simple anyway
        } else if (cc == ')') {
            if (state.current == MxMath) {
                if (++m_pos >= m_str->length())
                    break;
                if (m_str->unicode()[m_pos].unicode() == ')') {
                    ostack.pop();
                    state = sstack.pop();
                } else {
                    // false hit: the $(( was a $( ( in fact.
                    // ash does not care, but bash does.
                    m_pos = ostack.pop();
                    state.current = MxParen;
                    state.dquote = false;
                    sstack.push(state);
                }
                continue;
            } else if (state.current == MxParen) {
                state = sstack.pop();
                continue;
            } else {
                break;
            }
#if 0 // MxGroup is impossible, see below.
        } else if (cc == '}') {
            if (state.current == MxGroup) {
                state = sstack.pop();
                continue;
            }
#endif
        } else if (cc == '(') {
            sstack.push(state);
            state.current = MxParen;
            m_simple = false;
            hadWord = true;
#if 0 // Should match only at the beginning of a command, which we never have currently.
        } else if (cc == '{') {
            sstack.push(state);
            state.current = MxGroup;
            m_simple = false;
            hadWord = true;
            continue;
#endif
        } else if (cc == '<' || cc == '>' || cc == '&' || cc == '|' || cc == ';') {
            if (sstack.isEmpty())
                break;
        } else if (cc == ' ' || cc == '\t') {
            if (!hadWord)
                continue;
            if (sstack.isEmpty())
                break;
        }
        m_value += QChar(cc);
        hadWord = true;
    }
    // TODO: Possibly complain here if (!sstack.empty())
    if (!m_simple)
        m_value.clear();
    if (hadWord) {
        m_prev = prev;
        return true;
    }
    return false;
#endif
}

void QtcProcess::ArgIterator::deleteArg()
{
    if (!m_prev)
        while (m_pos < m_str->length() && m_str->at(m_pos).isSpace())
            m_pos++;
    m_str->remove(m_prev, m_pos - m_prev);
    m_pos = m_prev;
}

void QtcProcess::ArgIterator::appendArg(const QString &str)
{
    const QString qstr = quoteArg(str);
    if (!m_pos)
        m_str->insert(0, qstr + QLatin1Char(' '));
    else
        m_str->insert(m_pos, QLatin1Char(' ') + qstr);
    m_pos += qstr.length() + 1;
}
