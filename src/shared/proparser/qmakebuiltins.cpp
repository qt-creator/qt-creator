/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qmakeevaluator.h"

#include "qmakeevaluator_p.h"
#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "ioutils.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRegExp>
#include <QSet>
#include <QStringList>
#include <QTextStream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/utsname.h>
#else
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_PCLOSE pclose
#endif

using namespace ProFileEvaluatorInternal;

QT_BEGIN_NAMESPACE

using namespace ProStringConstants;


#define fL1S(s) QString::fromLatin1(s)

enum ExpandFunc {
    E_INVALID = 0, E_MEMBER, E_FIRST, E_LAST, E_SIZE, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
    E_SPRINTF, E_FORMAT_NUMBER, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
    E_FIND, E_SYSTEM, E_UNIQUE, E_REVERSE, E_QUOTE, E_ESCAPE_EXPAND,
    E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE, E_VAL_ESCAPE,
    E_REPLACE, E_SORT_DEPENDS, E_RESOLVE_DEPENDS, E_ENUMERATE_VARS,
    E_SHADOWED, E_ABSOLUTE_PATH, E_RELATIVE_PATH, E_CLEAN_PATH
};

enum TestFunc {
    T_INVALID = 0, T_REQUIRES, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
    T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
    T_RETURN, T_BREAK, T_NEXT, T_DEFINED, T_CONTAINS, T_INFILE,
    T_COUNT, T_ISEMPTY, T_INCLUDE, T_LOAD, T_DEBUG, T_MESSAGE, T_IF
};

void QMakeEvaluator::initFunctionStatics()
{
    static const struct {
        const char * const name;
        const ExpandFunc func;
    } expandInits[] = {
        { "member", E_MEMBER },
        { "first", E_FIRST },
        { "last", E_LAST },
        { "size", E_SIZE },
        { "cat", E_CAT },
        { "fromfile", E_FROMFILE },
        { "eval", E_EVAL },
        { "list", E_LIST },
        { "sprintf", E_SPRINTF },
        { "format_number", E_FORMAT_NUMBER },
        { "join", E_JOIN },
        { "split", E_SPLIT },
        { "basename", E_BASENAME },
        { "dirname", E_DIRNAME },
        { "section", E_SECTION },
        { "find", E_FIND },
        { "system", E_SYSTEM },
        { "unique", E_UNIQUE },
        { "reverse", E_REVERSE },
        { "quote", E_QUOTE },
        { "escape_expand", E_ESCAPE_EXPAND },
        { "upper", E_UPPER },
        { "lower", E_LOWER },
        { "re_escape", E_RE_ESCAPE },
        { "val_escape", E_VAL_ESCAPE },
        { "files", E_FILES },
        { "prompt", E_PROMPT },
        { "replace", E_REPLACE },
        { "sort_depends", E_SORT_DEPENDS },
        { "resolve_depends", E_RESOLVE_DEPENDS },
        { "enumerate_vars", E_ENUMERATE_VARS },
        { "shadowed", E_SHADOWED },
        { "absolute_path", E_ABSOLUTE_PATH },
        { "relative_path", E_RELATIVE_PATH },
        { "clean_path", E_CLEAN_PATH },
    };
    for (unsigned i = 0; i < sizeof(expandInits)/sizeof(expandInits[0]); ++i)
        statics.expands.insert(ProString(expandInits[i].name), expandInits[i].func);

    static const struct {
        const char * const name;
        const TestFunc func;
    } testInits[] = {
        { "requires", T_REQUIRES },
        { "greaterThan", T_GREATERTHAN },
        { "lessThan", T_LESSTHAN },
        { "equals", T_EQUALS },
        { "isEqual", T_EQUALS },
        { "exists", T_EXISTS },
        { "export", T_EXPORT },
        { "clear", T_CLEAR },
        { "unset", T_UNSET },
        { "eval", T_EVAL },
        { "CONFIG", T_CONFIG },
        { "if", T_IF },
        { "isActiveConfig", T_CONFIG },
        { "system", T_SYSTEM },
        { "return", T_RETURN },
        { "break", T_BREAK },
        { "next", T_NEXT },
        { "defined", T_DEFINED },
        { "contains", T_CONTAINS },
        { "infile", T_INFILE },
        { "count", T_COUNT },
        { "isEmpty", T_ISEMPTY },
        { "load", T_LOAD },
        { "include", T_INCLUDE },
        { "debug", T_DEBUG },
        { "message", T_MESSAGE },
        { "warning", T_MESSAGE },
        { "error", T_MESSAGE },
    };
    for (unsigned i = 0; i < sizeof(testInits)/sizeof(testInits[0]); ++i)
        statics.functions.insert(ProString(testInits[i].name), testInits[i].func);
}

static bool isTrue(const ProString &_str, QString &tmp)
{
    const QString &str = _str.toQString(tmp);
    return !str.compare(statics.strtrue, Qt::CaseInsensitive) || str.toInt();
}

static QString
quoteValue(const ProString &val)
{
    QString ret;
    ret.reserve(val.size());
    const QChar *chars = val.constData();
    bool quote = val.isEmpty();
    bool escaping = false;
    for (int i = 0, l = val.size(); i < l; i++) {
        QChar c = chars[i];
        ushort uc = c.unicode();
        if (uc < 32) {
            if (!escaping) {
                escaping = true;
                ret += QLatin1String("$$escape_expand(");
            }
            switch (uc) {
            case '\r':
                ret += QLatin1String("\\\\r");
                break;
            case '\n':
                ret += QLatin1String("\\\\n");
                break;
            case '\t':
                ret += QLatin1String("\\\\t");
                break;
            default:
                ret += QString::fromLatin1("\\\\x%1").arg(uc, 2, 16, QLatin1Char('0'));
                break;
            }
        } else {
            if (escaping) {
                escaping = false;
                ret += QLatin1Char(')');
            }
            switch (uc) {
            case '\\':
                ret += QLatin1String("\\\\");
                break;
            case '"':
                ret += QLatin1String("\\\"");
                break;
            case '\'':
                ret += QLatin1String("\\'");
                break;
            case '$':
                ret += QLatin1String("\\$");
                break;
            case '#':
                ret += QLatin1String("$${LITERAL_HASH}");
                break;
            case 32:
                quote = true;
                // fallthrough
            default:
                ret += c;
                break;
            }
        }
    }
    if (escaping)
        ret += QLatin1Char(')');
    if (quote) {
        ret.prepend(QLatin1Char('"'));
        ret.append(QLatin1Char('"'));
    }
    return ret;
}

#ifndef QT_BOOTSTRAPPED
void QMakeEvaluator::runProcess(QProcess *proc, const QString &command,
                                           QProcess::ProcessChannel chan) const
{
    proc->setWorkingDirectory(currentDirectory());
    if (!m_option->environment.isEmpty())
        proc->setProcessEnvironment(m_option->environment);
# ifdef Q_OS_WIN
    proc->setNativeArguments(QLatin1String("/v:off /s /c \"") + command + QLatin1Char('"'));
    proc->start(m_option->getEnv(QLatin1String("COMSPEC")), QStringList());
# else
    proc->start(QLatin1String("/bin/sh"), QStringList() << QLatin1String("-c") << command);
# endif
    proc->waitForFinished(-1);
    proc->setReadChannel(chan);
    QByteArray errout = proc->readAll();
    if (errout.endsWith('\n'))
        errout.chop(1);
    m_handler->message(QMakeHandler::EvalError, QString::fromLocal8Bit(errout));
}
#endif

void QMakeEvaluator::populateDeps(
        const ProStringList &deps, const ProString &prefix,
        QHash<ProString, QSet<ProString> > &dependencies, ProValueMap &dependees,
        ProStringList &rootSet) const
{
    foreach (const ProString &item, deps)
        if (!dependencies.contains(item)) {
            QSet<ProString> &dset = dependencies[item]; // Always create entry
            ProStringList depends = values(ProString(prefix + item + QString::fromLatin1(".depends")));
            if (depends.isEmpty()) {
                rootSet << item;
            } else {
                foreach (const ProString &dep, depends) {
                    dset.insert(dep);
                    dependees[dep] << item;
                }
                populateDeps(depends, prefix, dependencies, dependees, rootSet);
            }
        }
}

ProStringList QMakeEvaluator::evaluateExpandFunction(
        const ProString &func, const ProStringList &args)
{
    ExpandFunc func_t = ExpandFunc(statics.expands.value(func));
    if (func_t == 0) {
        const QString &fn = func.toQString(m_tmp1);
        const QString &lfn = fn.toLower();
        if (!fn.isSharedWith(lfn)) {
            func_t = ExpandFunc(statics.expands.value(ProString(lfn)));
            if (func_t)
                deprecationWarning(fL1S("Using uppercased builtin functions is deprecated."));
        }
    }

    ProStringList ret;

    switch (func_t) {
    case E_BASENAME:
    case E_DIRNAME:
    case E_SECTION: {
        bool regexp = false;
        QString sep;
        ProString var;
        int beg = 0;
        int end = -1;
        if (func_t == E_SECTION) {
            if (args.count() != 3 && args.count() != 4) {
                evalError(fL1S("%1(var) section(var, sep, begin, end) requires"
                               " three or four arguments.").arg(func.toQString(m_tmp1)));
            } else {
                var = args[0];
                sep = args.at(1).toQString();
                beg = args.at(2).toQString(m_tmp2).toInt();
                if (args.count() == 4)
                    end = args.at(3).toQString(m_tmp2).toInt();
            }
        } else {
            if (args.count() != 1) {
                evalError(fL1S("%1(var) requires one argument.").arg(func.toQString(m_tmp1)));
            } else {
                var = args[0];
                regexp = true;
                sep = QLatin1String("[\\\\/]");
                if (func_t == E_DIRNAME)
                    end = -2;
                else
                    beg = -1;
            }
        }
        if (!var.isEmpty()) {
            if (regexp) {
                QRegExp sepRx(sep);
                foreach (const ProString &str, values(map(var))) {
                    const QString &rstr = str.toQString(m_tmp1).section(sepRx, beg, end);
                    ret << (rstr.isSharedWith(m_tmp1) ? str : ProString(rstr, NoHash).setSource(str));
                }
            } else {
                foreach (const ProString &str, values(map(var))) {
                    const QString &rstr = str.toQString(m_tmp1).section(sep, beg, end);
                    ret << (rstr.isSharedWith(m_tmp1) ? str : ProString(rstr, NoHash).setSource(str));
                }
            }
        }
        break;
    }
    case E_SPRINTF:
        if (args.count() < 1) {
            evalError(fL1S("sprintf(format, ...) requires at least one argument."));
        } else {
            QString tmp = args.at(0).toQString(m_tmp1);
            for (int i = 1; i < args.count(); ++i)
                tmp = tmp.arg(args.at(i).toQString(m_tmp2));
            // Note: this depends on split_value_list() making a deep copy
            ret = split_value_list(tmp);
        }
        break;
    case E_FORMAT_NUMBER:
        if (args.count() > 2) {
            evalError(fL1S("format_number(number[, options...]) requires one or two arguments."));
        } else {
            int ibase = 10;
            int obase = 10;
            int width = 0;
            bool zeropad = false;
            bool leftalign = false;
            enum { DefaultSign, PadSign, AlwaysSign } sign = DefaultSign;
            if (args.count() >= 2) {
                foreach (const ProString &opt, split_value_list(args.at(1).toQString(m_tmp2))) {
                    opt.toQString(m_tmp3);
                    if (m_tmp3.startsWith(QLatin1String("ibase="))) {
                        ibase = m_tmp3.mid(6).toInt();
                    } else if (m_tmp3.startsWith(QLatin1String("obase="))) {
                        obase = m_tmp3.mid(6).toInt();
                    } else if (m_tmp3.startsWith(QLatin1String("width="))) {
                        width = m_tmp3.mid(6).toInt();
                    } else if (m_tmp3 == QLatin1String("zeropad")) {
                        zeropad = true;
                    } else if (m_tmp3 == QLatin1String("padsign")) {
                        sign = PadSign;
                    } else if (m_tmp3 == QLatin1String("alwayssign")) {
                        sign = AlwaysSign;
                    } else if (m_tmp3 == QLatin1String("leftalign")) {
                        leftalign = true;
                    } else {
                        evalError(fL1S("format_number(): invalid format option %1.").arg(m_tmp3));
                        goto formfail;
                    }
                }
            }
            args.at(0).toQString(m_tmp3);
            if (m_tmp3.contains(QLatin1Char('.'))) {
                evalError(fL1S("format_number(): floats are currently not supported."));
                break;
            }
            bool ok;
            qlonglong num = m_tmp3.toLongLong(&ok, ibase);
            if (!ok) {
                evalError(fL1S("format_number(): malformed number %2 for base %1.")
                          .arg(ibase).arg(m_tmp3));
                break;
            }
            QString outstr;
            if (num < 0) {
                num = -num;
                outstr = QLatin1Char('-');
            } else if (sign == AlwaysSign) {
                outstr = QLatin1Char('+');
            } else if (sign == PadSign) {
                outstr = QLatin1Char(' ');
            }
            QString numstr = QString::number(num, obase);
            int space = width - outstr.length() - numstr.length();
            if (space <= 0) {
                outstr += numstr;
            } else if (leftalign) {
                outstr += numstr + QString(space, QLatin1Char(' '));
            } else if (zeropad) {
                outstr += QString(space, QLatin1Char('0')) + numstr;
            } else {
                outstr.prepend(QString(space, QLatin1Char(' ')));
                outstr += numstr;
            }
            ret += ProString(outstr, NoHash);
        }
      formfail:
        break;
    case E_JOIN: {
        if (args.count() < 1 || args.count() > 4) {
            evalError(fL1S("join(var, glue, before, after) requires one to four arguments."));
        } else {
            QString glue;
            ProString before, after;
            if (args.count() >= 2)
                glue = args.at(1).toQString(m_tmp1);
            if (args.count() >= 3)
                before = args[2];
            if (args.count() == 4)
                after = args[3];
            const ProStringList &var = values(map(args.at(0)));
            if (!var.isEmpty()) {
                const ProFile *src = currentProFile();
                foreach (const ProString &v, var)
                    if (const ProFile *s = v.sourceFile()) {
                        src = s;
                        break;
                    }
                ret = split_value_list(before + var.join(glue) + after, src);
            }
        }
        break;
    }
    case E_SPLIT:
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("split(var, sep) requires one or two arguments."));
        } else {
            const QString &sep = (args.count() == 2) ? args.at(1).toQString(m_tmp1) : statics.field_sep;
            foreach (const ProString &var, values(map(args.at(0))))
                foreach (const QString &splt, var.toQString(m_tmp2).split(sep))
                    ret << (splt.isSharedWith(m_tmp2) ? var : ProString(splt, NoHash).setSource(var));
        }
        break;
    case E_MEMBER:
        if (args.count() < 1 || args.count() > 3) {
            evalError(fL1S("member(var, start, end) requires one to three arguments."));
        } else {
            bool ok = true;
            const ProStringList &var = values(map(args.at(0)));
            int start = 0, end = 0;
            if (args.count() >= 2) {
                const QString &start_str = args.at(1).toQString(m_tmp1);
                start = start_str.toInt(&ok);
                if (!ok) {
                    if (args.count() == 2) {
                        int dotdot = start_str.indexOf(statics.strDotDot);
                        if (dotdot != -1) {
                            start = start_str.left(dotdot).toInt(&ok);
                            if (ok)
                                end = start_str.mid(dotdot+2).toInt(&ok);
                        }
                    }
                    if (!ok)
                        evalError(fL1S("member() argument 2 (start) '%2' invalid.")
                                  .arg(start_str));
                } else {
                    end = start;
                    if (args.count() == 3)
                        end = args.at(2).toQString(m_tmp1).toInt(&ok);
                    if (!ok)
                        evalError(fL1S("member() argument 3 (end) '%2' invalid.")
                                  .arg(args.at(2).toQString(m_tmp1)));
                }
            }
            if (ok) {
                if (start < 0)
                    start += var.count();
                if (end < 0)
                    end += var.count();
                if (start < 0 || start >= var.count() || end < 0 || end >= var.count()) {
                    //nothing
                } else if (start < end) {
                    for (int i = start; i <= end && var.count() >= i; i++)
                        ret.append(var[i]);
                } else {
                    for (int i = start; i >= end && var.count() >= i && i >= 0; i--)
                        ret += var[i];
                }
            }
        }
        break;
    case E_FIRST:
    case E_LAST:
        if (args.count() != 1) {
            evalError(fL1S("%1(var) requires one argument.").arg(func.toQString(m_tmp1)));
        } else {
            const ProStringList &var = values(map(args.at(0)));
            if (!var.isEmpty()) {
                if (func_t == E_FIRST)
                    ret.append(var[0]);
                else
                    ret.append(var.last());
            }
        }
        break;
    case E_SIZE:
        if (args.count() != 1)
            evalError(fL1S("size(var) requires one argument."));
        else
            ret.append(ProString(QString::number(values(map(args.at(0))).size()), NoHash));
        break;
    case E_CAT:
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("cat(file, singleline=true) requires one or two arguments."));
        } else {
            const QString &file = args.at(0).toQString(m_tmp1);

            bool singleLine = true;
            if (args.count() > 1)
                singleLine = isTrue(args.at(1), m_tmp2);

            QFile qfile(resolvePath(m_option->expandEnvVars(file)));
            if (qfile.open(QIODevice::ReadOnly)) {
                QTextStream stream(&qfile);
                while (!stream.atEnd()) {
                    ret += split_value_list(stream.readLine().trimmed());
                    if (!singleLine)
                        ret += ProString("\n", NoHash);
                }
                qfile.close();
            }
        }
        break;
    case E_FROMFILE:
        if (args.count() != 2) {
            evalError(fL1S("fromfile(file, variable) requires two arguments."));
        } else {
            ProValueMap vars;
            QString fn = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));
            fn.detach();
            if (evaluateFileInto(fn, QMakeHandler::EvalAuxFile, &vars, LoadProOnly))
                ret = vars.value(map(args.at(1)));
        }
        break;
    case E_EVAL:
        if (args.count() != 1) {
            evalError(fL1S("eval(variable) requires one argument."));
        } else {
            ret += values(map(args.at(0)));
        }
        break;
    case E_LIST: {
        QString tmp;
        tmp.sprintf(".QMAKE_INTERNAL_TMP_variableName_%d", m_listCount++);
        ret = ProStringList(ProString(tmp, NoHash));
        ProStringList lst;
        foreach (const ProString &arg, args)
            lst += split_value_list(arg.toQString(m_tmp1), arg.sourceFile()); // Relies on deep copy
        m_valuemapStack.top()[ret.at(0)] = lst;
        break; }
    case E_FIND:
        if (args.count() != 2) {
            evalError(fL1S("find(var, str) requires two arguments."));
        } else {
            QRegExp regx(args.at(1).toQString());
            int t = 0;
            foreach (const ProString &val, values(map(args.at(0)))) {
                if (regx.indexIn(val.toQString(m_tmp[t])) != -1)
                    ret += val;
                t ^= 1;
            }
        }
        break;
    case E_SYSTEM:
        if (!m_skipLevel) {
            if (args.count() < 1 || args.count() > 2) {
                evalError(fL1S("system(execute) requires one or two arguments."));
            } else {
                bool singleLine = true;
                if (args.count() > 1)
                    singleLine = isTrue(args.at(1), m_tmp2);
                QByteArray output;
#ifndef QT_BOOTSTRAPPED
                QProcess proc;
                runProcess(&proc, args.at(0).toQString(m_tmp2), QProcess::StandardError);
                output = proc.readAllStandardOutput();
                output.replace('\t', ' ');
                if (singleLine)
                    output.replace('\n', ' ');
#else
                char buff[256];
                FILE *proc = QT_POPEN(QString(QLatin1String("cd ")
                                       + IoUtils::shellQuote(currentDirectory())
                                       + QLatin1String(" && ") + args[0]).toLocal8Bit(), "r");
                while (proc && !feof(proc)) {
                    int read_in = int(fread(buff, 1, 255, proc));
                    if (!read_in)
                        break;
                    for (int i = 0; i < read_in; i++) {
                        if ((singleLine && buff[i] == '\n') || buff[i] == '\t')
                            buff[i] = ' ';
                    }
                    output.append(buff, read_in);
                }
                if (proc)
                    QT_PCLOSE(proc);
#endif
                ret += split_value_list(QString::fromLocal8Bit(output));
            }
        }
        break;
    case E_UNIQUE:
        if (args.count() != 1) {
            evalError(fL1S("unique(var) requires one argument."));
        } else {
            ret = values(map(args.at(0)));
            ret.removeDuplicates();
        }
        break;
    case E_REVERSE:
        if (args.count() != 1) {
            evalError(fL1S("reverse(var) requires one argument."));
        } else {
            ProStringList var = values(args.at(0));
            for (int i = 0; i < var.size() / 2; i++)
                qSwap(var[i], var[var.size() - i - 1]);
            ret += var;
        }
        break;
    case E_QUOTE:
        ret += args;
        break;
    case E_ESCAPE_EXPAND:
        for (int i = 0; i < args.size(); ++i) {
            QString str = args.at(i).toQString();
            QChar *i_data = str.data();
            int i_len = str.length();
            for (int x = 0; x < i_len; ++x) {
                if (*(i_data+x) == QLatin1Char('\\') && x < i_len-1) {
                    if (*(i_data+x+1) == QLatin1Char('\\')) {
                        ++x;
                    } else {
                        struct {
                            char in, out;
                        } mapped_quotes[] = {
                            { 'n', '\n' },
                            { 't', '\t' },
                            { 'r', '\r' },
                            { 0, 0 }
                        };
                        for (int i = 0; mapped_quotes[i].in; ++i) {
                            if (*(i_data+x+1) == QLatin1Char(mapped_quotes[i].in)) {
                                *(i_data+x) = QLatin1Char(mapped_quotes[i].out);
                                if (x < i_len-2)
                                    memmove(i_data+x+1, i_data+x+2, (i_len-x-2)*sizeof(QChar));
                                --i_len;
                                break;
                            }
                        }
                    }
                }
            }
            ret.append(ProString(QString(i_data, i_len), NoHash).setSource(args.at(i)));
        }
        break;
    case E_RE_ESCAPE:
        for (int i = 0; i < args.size(); ++i) {
            const QString &rstr = QRegExp::escape(args.at(i).toQString(m_tmp1));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(i) : ProString(rstr, NoHash).setSource(args.at(i)));
        }
        break;
    case E_VAL_ESCAPE:
        if (args.count() != 1) {
            evalError(fL1S("val_escape(var) requires one argument."));
        } else {
            const ProStringList &vals = values(args.at(0));
            ret.reserve(vals.size());
            foreach (const ProString &str, vals)
                ret += ProString(quoteValue(str), NoHash);
        }
        break;
    case E_UPPER:
    case E_LOWER:
        for (int i = 0; i < args.count(); ++i) {
            QString rstr = args.at(i).toQString(m_tmp1);
            rstr = (func_t == E_UPPER) ? rstr.toUpper() : rstr.toLower();
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(i) : ProString(rstr, NoHash).setSource(args.at(i)));
        }
        break;
    case E_FILES:
        if (args.count() != 1 && args.count() != 2) {
            evalError(fL1S("files(pattern, recursive=false) requires one or two arguments."));
        } else {
            bool recursive = false;
            if (args.count() == 2)
                recursive = isTrue(args.at(1), m_tmp2);
            QStringList dirs;
            QString r = m_option->expandEnvVars(args.at(0).toQString(m_tmp1))
                        .replace(QLatin1Char('\\'), QLatin1Char('/'));
            QString pfx;
            if (IoUtils::isRelativePath(r)) {
                pfx = currentDirectory();
                if (!pfx.endsWith(QLatin1Char('/')))
                    pfx += QLatin1Char('/');
            }
            int slash = r.lastIndexOf(QLatin1Char('/'));
            if (slash != -1) {
                dirs.append(r.left(slash+1));
                r = r.mid(slash+1);
            } else {
                dirs.append(QString());
            }

            r.detach(); // Keep m_tmp out of QRegExp's cache
            QRegExp regex(r, Qt::CaseSensitive, QRegExp::Wildcard);
            for (int d = 0; d < dirs.count(); d++) {
                QString dir = dirs[d];
                QDir qdir(pfx + dir);
                for (int i = 0; i < (int)qdir.count(); ++i) {
                    if (qdir[i] == statics.strDot || qdir[i] == statics.strDotDot)
                        continue;
                    QString fname = dir + qdir[i];
                    if (IoUtils::fileType(pfx + fname) == IoUtils::FileIsDir) {
                        if (recursive)
                            dirs.append(fname + QLatin1Char('/'));
                    }
                    if (regex.exactMatch(qdir[i]))
                        ret += ProString(fname, NoHash).setSource(currentProFile());
                }
            }
        }
        break;
#ifdef PROEVALUATOR_FULL
    case E_PROMPT: {
        if (args.count() != 1) {
            evalError(fL1S("prompt(question) requires one argument."));
//        } else if (currentFileName() == QLatin1String("-")) {
//            evalError(fL1S("prompt(question) cannot be used when '-o -' is used"));
        } else {
            QString msg = m_option->expandEnvVars(args.at(0).toQString(m_tmp1));
            if (!msg.endsWith(QLatin1Char('?')))
                msg += QLatin1Char('?');
            fprintf(stderr, "Project PROMPT: %s ", qPrintable(msg));

            QFile qfile;
            if (qfile.open(stdin, QIODevice::ReadOnly)) {
                QTextStream t(&qfile);
                ret = split_value_list(t.readLine());
            }
        }
        break; }
#endif
    case E_REPLACE:
        if (args.count() != 3 ) {
            evalError(fL1S("replace(var, before, after) requires three arguments."));
        } else {
            const QRegExp before(args.at(1).toQString());
            const QString &after(args.at(2).toQString(m_tmp2));
            foreach (const ProString &val, values(map(args.at(0)))) {
                QString rstr = val.toQString(m_tmp1);
                QString copy = rstr; // Force a detach on modify
                rstr.replace(before, after);
                ret << (rstr.isSharedWith(m_tmp1) ? val : ProString(rstr, NoHash).setSource(val));
            }
        }
        break;
    case E_SORT_DEPENDS:
    case E_RESOLVE_DEPENDS:
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("%1(var, prefix) requires one or two arguments.").arg(func.toQString(m_tmp1)));
        } else {
            QHash<ProString, QSet<ProString> > dependencies;
            ProValueMap dependees;
            ProStringList rootSet;
            ProStringList orgList = values(args.at(0));
            populateDeps(orgList, (args.count() < 2 ? ProString() : args.at(1)),
                         dependencies, dependees, rootSet);
            for (int i = 0; i < rootSet.size(); ++i) {
                const ProString &item = rootSet.at(i);
                if ((func_t == E_RESOLVE_DEPENDS) || orgList.contains(item))
                    ret.prepend(item);
                foreach (const ProString &dep, dependees[item]) {
                    QSet<ProString> &dset = dependencies[dep];
                    dset.remove(rootSet.at(i)); // *Don't* use 'item' - rootSet may have changed!
                    if (dset.isEmpty())
                        rootSet << dep;
                }
            }
        }
        break;
    case E_ENUMERATE_VARS: {
        QSet<ProString> keys;
        foreach (const ProValueMap &vmap, m_valuemapStack)
            for (ProValueMap::ConstIterator it = vmap.constBegin(); it != vmap.constEnd(); ++it)
                keys.insert(it.key());
        ret.reserve(keys.size());
        foreach (const ProString &key, keys)
            ret << key;
        break; }
    case E_SHADOWED:
        if (args.count() != 1) {
            evalError(fL1S("shadowed(path) requires one argument."));
        } else {
            QString val = resolvePath(args.at(0).toQString(m_tmp1));
            if (m_option->source_root.isEmpty()) {
                ret += ProString(val, NoHash);
            } else if (val.startsWith(m_option->source_root)
                       && (val.length() == m_option->source_root.length()
                           || val.at(m_option->source_root.length()) == QLatin1Char('/'))) {
                ret += ProString(m_option->build_root + val.mid(m_option->source_root.length()),
                                 NoHash).setSource(args.at(0));
            }
        }
        break;
    case E_ABSOLUTE_PATH:
        if (args.count() > 2)
            evalError(fL1S("absolute_path(path[, base]) requires one or two arguments."));
        else
            ret << ProString(QDir::cleanPath(
                    QDir(args.count() > 1 ? args.at(1).toQString(m_tmp2) : currentDirectory())
                    .absoluteFilePath(args.at(0).toQString(m_tmp1))), NoHash).setSource(args.at(0));
        break;
    case E_RELATIVE_PATH:
        if (args.count() > 2)
            evalError(fL1S("relative_path(path[, base]) requires one or two arguments."));
        else
            ret << ProString(QDir::cleanPath(
                    QDir(args.count() > 1 ? args.at(1).toQString(m_tmp2) : currentDirectory())
                    .relativeFilePath(args.at(0).toQString(m_tmp1))), NoHash).setSource(args.at(0));
        break;
    case E_CLEAN_PATH:
        if (args.count() != 1)
            evalError(fL1S("clean_path(path) requires one argument."));
        else
            ret << ProString(QDir::cleanPath(args.at(0).toQString(m_tmp1)),
                             NoHash).setSource(args.at(0));
        break;
    case E_INVALID:
        evalError(fL1S("'%1' is not a recognized replace function.")
                  .arg(func.toQString(m_tmp1)));
        break;
    default:
        evalError(fL1S("Function '%1' is not implemented.").arg(func.toQString(m_tmp1)));
        break;
    }

    return ret;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditionalFunction(
        const ProString &function, const ProStringList &args)
{
    TestFunc func_t = (TestFunc)statics.functions.value(function);

    switch (func_t) {
    case T_DEFINED:
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("defined(function, [\"test\"|\"replace\"])"
                           " requires one or two arguments."));
            return ReturnFalse;
        }
        if (args.count() > 1) {
            if (args[1] == QLatin1String("test"))
                return returnBool(m_functionDefs.testFunctions.contains(args[0]));
            else if (args[1] == QLatin1String("replace"))
                return returnBool(m_functionDefs.replaceFunctions.contains(args[0]));
            evalError(fL1S("defined(function, type): unexpected type [%1].")
                      .arg(args.at(1).toQString(m_tmp1)));
            return ReturnFalse;
        }
        return returnBool(m_functionDefs.replaceFunctions.contains(args[0])
                          || m_functionDefs.testFunctions.contains(args[0]));
    case T_RETURN:
        m_returnValue = args;
        // It is "safe" to ignore returns - due to qmake brokeness
        // they cannot be used to terminate loops anyway.
        if (m_skipLevel || m_cumulative)
            return ReturnTrue;
        if (m_valuemapStack.isEmpty()) {
            evalError(fL1S("unexpected return()."));
            return ReturnFalse;
        }
        return ReturnReturn;
    case T_EXPORT: {
        if (m_skipLevel && !m_cumulative)
            return ReturnTrue;
        if (args.count() != 1) {
            evalError(fL1S("export(variable) requires one argument."));
            return ReturnFalse;
        }
        const ProString &var = map(args.at(0));
        for (int i = m_valuemapStack.size(); --i > 0; ) {
            ProValueMap::Iterator it = m_valuemapStack[i].find(var);
            if (it != m_valuemapStack.at(i).end()) {
                if (it->constBegin() == statics.fakeValue.constBegin()) {
                    // This is stupid, but qmake doesn't propagate deletions
                    m_valuemapStack[0][var] = ProStringList();
                } else {
                    m_valuemapStack[0][var] = *it;
                }
                m_valuemapStack[i].erase(it);
                while (--i)
                    m_valuemapStack[i].remove(var);
                break;
            }
        }
        return ReturnTrue;
    }
    case T_INFILE:
        if (args.count() < 2 || args.count() > 3) {
            evalError(fL1S("infile(file, var, [values]) requires two or three arguments."));
        } else {
            ProValueMap vars;
            QString fn = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));
            fn.detach();
            if (!evaluateFileInto(fn, QMakeHandler::EvalAuxFile, &vars, LoadProOnly))
                return ReturnFalse;
            if (args.count() == 2)
                return returnBool(vars.contains(args.at(1)));
            QRegExp regx;
            const QString &qry = args.at(2).toQString(m_tmp1);
            if (qry != QRegExp::escape(qry)) {
                QString copy = qry;
                copy.detach();
                regx.setPattern(copy);
            }
            int t = 0;
            foreach (const ProString &s, vars.value(map(args.at(1)))) {
                if ((!regx.isEmpty() && regx.exactMatch(s.toQString(m_tmp[t]))) || s == qry)
                    return ReturnTrue;
                t ^= 1;
            }
        }
        return ReturnFalse;
#if 0
    case T_REQUIRES:
#endif
    case T_EVAL: {
            ProFile *pro = m_parser->parsedProBlock(fL1S("(eval)"),
                                                    args.join(statics.field_sep));
            if (!pro)
                return ReturnFalse;
            m_locationStack.push(m_current);
            VisitReturn ret = visitProBlock(pro, pro->tokPtr());
            m_current = m_locationStack.pop();
            pro->deref();
            return ret;
        }
    case T_BREAK:
        if (m_skipLevel)
            return ReturnFalse;
        if (m_loopLevel)
            return ReturnBreak;
        evalError(fL1S("Unexpected break()."));
        return ReturnFalse;
    case T_NEXT:
        if (m_skipLevel)
            return ReturnFalse;
        if (m_loopLevel)
            return ReturnNext;
        evalError(fL1S("Unexpected next()."));
        return ReturnFalse;
    case T_IF: {
        if (m_skipLevel && !m_cumulative)
            return ReturnFalse;
        if (args.count() != 1) {
            evalError(fL1S("if(condition) requires one argument."));
            return ReturnFalse;
        }
        const ProString &cond = args.at(0);
        bool quoted = false;
        bool ret = true;
        bool orOp = false;
        bool invert = false;
        bool isFunc = false;
        int parens = 0;
        QString test;
        test.reserve(20);
        QString argsString;
        argsString.reserve(50);
        const QChar *d = cond.constData();
        const QChar *ed = d + cond.size();
        while (d < ed) {
            ushort c = (d++)->unicode();
            bool isOp = false;
            if (quoted) {
                if (c == '"')
                    quoted = false;
                else if (c == '!' && test.isEmpty())
                    invert = true;
                else
                    test += c;
            } else if (c == '(') {
                isFunc = true;
                if (parens)
                    argsString += c;
                ++parens;
            } else if (c == ')') {
                --parens;
                if (parens)
                    argsString += c;
            } else if (!parens) {
                if (c == '"')
                    quoted = true;
                else if (c == ':' || c == '|')
                    isOp = true;
                else if (c == '!' && test.isEmpty())
                    invert = true;
                else
                    test += c;
            } else {
                argsString += c;
            }
            if (!quoted && !parens && (isOp || d == ed)) {
                if (m_cumulative || (orOp != ret)) {
                    test = test.trimmed();
                    if (isFunc)
                        ret = evaluateConditionalFunction(ProString(test), ProString(argsString, NoHash));
                    else
                        ret = isActiveConfig(test, true);
                    ret ^= invert;
                }
                orOp = (c == '|');
                invert = false;
                isFunc = false;
                test.clear();
                argsString.clear();
            }
        }
        return returnBool(ret);
    }
    case T_CONFIG: {
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("CONFIG(config) requires one or two arguments."));
            return ReturnFalse;
        }
        if (args.count() == 1)
            return returnBool(isActiveConfig(args.at(0).toQString(m_tmp2)));
        const QStringList &mutuals = args.at(1).toQString(m_tmp2).split(QLatin1Char('|'));
        const ProStringList &configs = values(statics.strCONFIG);

        for (int i = configs.size() - 1; i >= 0; i--) {
            for (int mut = 0; mut < mutuals.count(); mut++) {
                if (configs[i] == mutuals[mut].trimmed()) {
                    return returnBool(configs[i] == args[0]);
                }
            }
        }
        return ReturnFalse;
    }
    case T_CONTAINS: {
        if (args.count() < 2 || args.count() > 3) {
            evalError(fL1S("contains(var, val) requires two or three arguments."));
            return ReturnFalse;
        }

        const QString &qry = args.at(1).toQString(m_tmp1);
        QRegExp regx;
        if (qry != QRegExp::escape(qry)) {
            QString copy = qry;
            copy.detach();
            regx.setPattern(copy);
        }
        const ProStringList &l = values(map(args.at(0)));
        if (args.count() == 2) {
            int t = 0;
            for (int i = 0; i < l.size(); ++i) {
                const ProString &val = l[i];
                if ((!regx.isEmpty() && regx.exactMatch(val.toQString(m_tmp[t]))) || val == qry)
                    return ReturnTrue;
                t ^= 1;
            }
        } else {
            const QStringList &mutuals = args.at(2).toQString(m_tmp3).split(QLatin1Char('|'));
            for (int i = l.size() - 1; i >= 0; i--) {
                const ProString val = l[i];
                for (int mut = 0; mut < mutuals.count(); mut++) {
                    if (val == mutuals[mut].trimmed()) {
                        return returnBool((!regx.isEmpty()
                                           && regx.exactMatch(val.toQString(m_tmp2)))
                                          || val == qry);
                    }
                }
            }
        }
        return ReturnFalse;
    }
    case T_COUNT: {
        if (args.count() != 2 && args.count() != 3) {
            evalError(fL1S("count(var, count, op=\"equals\") requires two or three arguments."));
            return ReturnFalse;
        }
        int cnt = values(map(args.at(0))).count();
        if (args.count() == 3) {
            const ProString &comp = args.at(2);
            const int val = args.at(1).toQString(m_tmp1).toInt();
            if (comp == QLatin1String(">") || comp == QLatin1String("greaterThan")) {
                return returnBool(cnt > val);
            } else if (comp == QLatin1String(">=")) {
                return returnBool(cnt >= val);
            } else if (comp == QLatin1String("<") || comp == QLatin1String("lessThan")) {
                return returnBool(cnt < val);
            } else if (comp == QLatin1String("<=")) {
                return returnBool(cnt <= val);
            } else if (comp == QLatin1String("equals") || comp == QLatin1String("isEqual")
                       || comp == QLatin1String("=") || comp == QLatin1String("==")) {
                return returnBool(cnt == val);
            } else {
                evalError(fL1S("Unexpected modifier to count(%2).").arg(comp.toQString(m_tmp1)));
                return ReturnFalse;
            }
        }
        return returnBool(cnt == args.at(1).toQString(m_tmp1).toInt());
    }
    case T_GREATERTHAN:
    case T_LESSTHAN: {
        if (args.count() != 2) {
            evalError(fL1S("%1(variable, value) requires two arguments.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        const QString &rhs(args.at(1).toQString(m_tmp1)),
                      &lhs(values(map(args.at(0))).join(statics.field_sep));
        bool ok;
        int rhs_int = rhs.toInt(&ok);
        if (ok) { // do integer compare
            int lhs_int = lhs.toInt(&ok);
            if (ok) {
                if (func_t == T_GREATERTHAN)
                    return returnBool(lhs_int > rhs_int);
                return returnBool(lhs_int < rhs_int);
            }
        }
        if (func_t == T_GREATERTHAN)
            return returnBool(lhs > rhs);
        return returnBool(lhs < rhs);
    }
    case T_EQUALS:
        if (args.count() != 2) {
            evalError(fL1S("%1(variable, value) requires two arguments.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        return returnBool(values(map(args.at(0))).join(statics.field_sep)
                          == args.at(1).toQString(m_tmp1));
    case T_CLEAR: {
        if (m_skipLevel && !m_cumulative)
            return ReturnFalse;
        if (args.count() != 1) {
            evalError(fL1S("%1(variable) requires one argument.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        ProValueMap *hsh;
        ProValueMap::Iterator it;
        const ProString &var = map(args.at(0));
        if (!(hsh = findValues(var, &it)))
            return ReturnFalse;
        if (hsh == &m_valuemapStack.top())
            it->clear();
        else
            m_valuemapStack.top()[var].clear();
        return ReturnTrue;
    }
    case T_UNSET: {
        if (m_skipLevel && !m_cumulative)
            return ReturnFalse;
        if (args.count() != 1) {
            evalError(fL1S("%1(variable) requires one argument.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        ProValueMap *hsh;
        ProValueMap::Iterator it;
        const ProString &var = map(args.at(0));
        if (!(hsh = findValues(var, &it)))
            return ReturnFalse;
        if (m_valuemapStack.size() == 1)
            hsh->erase(it);
        else if (hsh == &m_valuemapStack.top())
            *it = statics.fakeValue;
        else
            m_valuemapStack.top()[var] = statics.fakeValue;
        return ReturnTrue;
    }
    case T_INCLUDE: {
        if (m_skipLevel && !m_cumulative)
            return ReturnFalse;
        if (args.count() < 1 || args.count() > 3) {
            evalError(fL1S("include(file, [into, [silent]]) requires one, two or three arguments."));
            return ReturnFalse;
        }
        QString parseInto;
        LoadFlags flags = 0;
        if (args.count() >= 2) {
            parseInto = args.at(1).toQString(m_tmp2);
            if (args.count() >= 3 && isTrue(args.at(2), m_tmp3))
                flags = LoadSilent;
        }
        QString fn = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));
        fn.detach();
        bool ok;
        if (parseInto.isEmpty()) {
            ok = evaluateFile(fn, QMakeHandler::EvalIncludeFile, LoadProOnly | flags);
        } else {
            ProValueMap symbols;
            if ((ok = evaluateFileInto(fn, QMakeHandler::EvalAuxFile, &symbols, LoadAll | flags))) {
                ProValueMap newMap;
                for (ProValueMap::ConstIterator
                        it = m_valuemapStack.top().constBegin(),
                        end = m_valuemapStack.top().constEnd();
                        it != end; ++it) {
                    const QString &ky = it.key().toQString(m_tmp1);
                    if (!(ky.startsWith(parseInto) &&
                          (ky.length() == parseInto.length()
                           || ky.at(parseInto.length()) == QLatin1Char('.'))))
                        newMap[it.key()] = it.value();
                }
                for (ProValueMap::ConstIterator it = symbols.constBegin();
                     it != symbols.constEnd(); ++it) {
                    const QString &ky = it.key().toQString(m_tmp1);
                    if (!ky.startsWith(QLatin1Char('.')))
                        newMap.insert(ProString(parseInto + QLatin1Char('.') + ky), it.value());
                }
                m_valuemapStack.top() = newMap;
            }
        }
        return returnBool(ok || (flags & LoadSilent));
    }
    case T_LOAD: {
        if (m_skipLevel && !m_cumulative)
            return ReturnFalse;
        bool ignore_error = false;
        if (args.count() == 2) {
            ignore_error = isTrue(args.at(1), m_tmp2);
        } else if (args.count() != 1) {
            evalError(fL1S("load(feature) requires one or two arguments."));
            return ReturnFalse;
        }
        return returnBool(evaluateFeatureFile(m_option->expandEnvVars(args.at(0).toQString()),
                                              ignore_error) || ignore_error);
    }
    case T_DEBUG:
        // Yup - do nothing. Nothing is going to enable debug output anyway.
        return ReturnFalse;
    case T_MESSAGE: {
        if (args.count() != 1) {
            evalError(fL1S("%1(message) requires one argument.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        const QString &msg = m_option->expandEnvVars(args.at(0).toQString(m_tmp2));
        if (!m_skipLevel)
            m_handler->fileMessage(fL1S("Project %1: %2")
                                   .arg(function.toQString(m_tmp1).toUpper(), msg));
        // ### Consider real termination in non-cumulative mode
        return returnBool(function != QLatin1String("error"));
    }
#ifdef PROEVALUATOR_FULL
    case T_SYSTEM: {
        if (m_cumulative) // Anything else would be insanity
            return ReturnFalse;
        if (args.count() != 1) {
            evalError(fL1S("system(exec) requires one argument."));
            return ReturnFalse;
        }
#ifndef QT_BOOTSTRAPPED
        QProcess proc;
        proc.setProcessChannelMode(QProcess::MergedChannels);
        runProcess(&proc, args.at(0).toQString(m_tmp2), QProcess::StandardOutput);
        return returnBool(proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0);
#else
        return returnBool(system((QLatin1String("cd ")
                                  + IoUtils::shellQuote(currentDirectory())
                                  + QLatin1String(" && ") + args.at(0)).toLocal8Bit().constData()) == 0);
#endif
    }
#endif
    case T_ISEMPTY: {
        if (args.count() != 1) {
            evalError(fL1S("isEmpty(var) requires one argument."));
            return ReturnFalse;
        }
        const ProStringList &sl = values(map(args.at(0)));
        if (sl.count() == 0) {
            return ReturnTrue;
        } else if (sl.count() > 0) {
            const ProString &var = sl.first();
            if (var.isEmpty())
                return ReturnTrue;
        }
        return ReturnFalse;
    }
    case T_EXISTS: {
        if (args.count() != 1) {
            evalError(fL1S("exists(file) requires one argument."));
            return ReturnFalse;
        }
        const QString &file = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));

        if (IoUtils::exists(file)) {
            return ReturnTrue;
        }
        int slsh = file.lastIndexOf(QLatin1Char('/'));
        QString fn = file.mid(slsh+1);
        if (fn.contains(QLatin1Char('*')) || fn.contains(QLatin1Char('?'))) {
            QString dirstr = file.left(slsh+1);
            if (!QDir(dirstr).entryList(QStringList(fn)).isEmpty())
                return ReturnTrue;
        }

        return ReturnFalse;
    }
    case T_INVALID:
        evalError(fL1S("'%1' is not a recognized test function.")
                  .arg(function.toQString(m_tmp1)));
        return ReturnFalse;
    default:
        evalError(fL1S("Function '%1' is not implemented.").arg(function.toQString(m_tmp1)));
        return ReturnFalse;
    }
}

QT_END_NAMESPACE
