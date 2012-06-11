/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profileevaluator.h"

#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "ioutils.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QRegExp>
#include <QSet>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QTextStream>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <QThreadPool>
#endif

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

///////////////////////////////////////////////////////////////////////
//
// ProFileEvaluator::Private
//
///////////////////////////////////////////////////////////////////////

class ProFileEvaluator::Private
{
public:
    static void initStatics();
    Private(QMakeGlobals *option, QMakeParser *parser,
            ProFileEvaluatorHandler *handler);
    ~Private();

    enum VisitReturn {
        ReturnFalse,
        ReturnTrue,
        ReturnBreak,
        ReturnNext,
        ReturnReturn
    };

    static ALWAYS_INLINE uint getBlockLen(const ushort *&tokPtr);
    ProString getStr(const ushort *&tokPtr);
    ProString getHashStr(const ushort *&tokPtr);
    void evaluateExpression(const ushort *&tokPtr, ProStringList *ret, bool joined);
    static ALWAYS_INLINE void skipStr(const ushort *&tokPtr);
    static ALWAYS_INLINE void skipHashStr(const ushort *&tokPtr);
    void skipExpression(const ushort *&tokPtr);

    void visitCmdLine(const QString &cmds);
    VisitReturn visitProFile(ProFile *pro, ProFileEvaluatorHandler::EvalFileType type,
                             ProFileEvaluator::LoadFlags flags);
    VisitReturn visitProBlock(ProFile *pro, const ushort *tokPtr);
    VisitReturn visitProBlock(const ushort *tokPtr);
    VisitReturn visitProLoop(const ProString &variable, const ushort *exprPtr,
                             const ushort *tokPtr);
    void visitProFunctionDef(ushort tok, const ProString &name, const ushort *tokPtr);
    void visitProVariable(ushort tok, const ProStringList &curr, const ushort *&tokPtr);

    static inline const ProString &map(const ProString &var);
    QHash<ProString, ProStringList> *findValues(const ProString &variableName,
                                                QHash<ProString, ProStringList>::Iterator *it);
    ProStringList &valuesRef(const ProString &variableName);
    ProStringList valuesDirect(const ProString &variableName) const;
    ProStringList values(const ProString &variableName) const;
    QString propertyValue(const QString &val, bool complain) const;

    ProStringList split_value_list(const QString &vals, const ProFile *source = 0);
    bool isActiveConfig(const QString &config, bool regex = false);
    ProStringList expandVariableReferences(const ProString &value, int *pos = 0, bool joined = false);
    ProStringList expandVariableReferences(const ushort *&tokPtr, int sizeHint = 0, bool joined = false);
    ProStringList evaluateExpandFunction(const ProString &function, const ProString &arguments);
    ProStringList evaluateExpandFunction(const ProString &function, const ushort *&tokPtr);
    ProStringList evaluateExpandFunction(const ProString &function, const ProStringList &args);
    void evalError(const QString &msg) const;

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;
    QString resolvePath(const QString &fileName) const
        { return IoUtils::resolvePath(currentDirectory(), fileName); }

    VisitReturn evaluateConditionalFunction(const ProString &function, const ProString &arguments);
    VisitReturn evaluateConditionalFunction(const ProString &function, const ushort *&tokPtr);
    VisitReturn evaluateConditionalFunction(const ProString &function, const ProStringList &args);
    bool evaluateFileDirect(const QString &fileName, ProFileEvaluatorHandler::EvalFileType type,
                            ProFileEvaluator::LoadFlags flags);
    bool evaluateFile(const QString &fileName, ProFileEvaluatorHandler::EvalFileType type,
                      ProFileEvaluator::LoadFlags flags);
    bool evaluateFeatureFile(const QString &fileName);
    enum EvalIntoMode { EvalProOnly, EvalWithDefaults, EvalWithSetup };
    bool evaluateFileInto(const QString &fileName, ProFileEvaluatorHandler::EvalFileType type,
                          QHash<ProString, ProStringList> *values, ProFunctionDefs *defs,
                          EvalIntoMode mode); // values are output-only, defs are input-only

    static ALWAYS_INLINE VisitReturn returnBool(bool b)
        { return b ? ReturnTrue : ReturnFalse; }

    QList<ProStringList> prepareFunctionArgs(const ushort *&tokPtr);
    QList<ProStringList> prepareFunctionArgs(const ProString &arguments);
    ProStringList evaluateFunction(const ProFunctionDef &func,
                                   const QList<ProStringList> &argumentsList, bool *ok);
    VisitReturn evaluateBoolFunction(const ProFunctionDef &func,
                                     const QList<ProStringList> &argumentsList,
                                     const ProString &function);

    bool modesForGenerator(const QString &gen,
            QMakeGlobals::HOST_MODE *host_mode, QMakeGlobals::TARG_MODE *target_mode) const;
    void validateModes() const;
    QStringList qmakeMkspecPaths() const;
    QStringList qmakeFeaturePaths() const;

    void populateDeps(
            const ProStringList &deps, const ProString &prefix,
            QHash<ProString, QSet<ProString> > &dependencies,
            QHash<ProString, ProStringList> &dependees,
            ProStringList &rootSet) const;

    QString expandEnvVars(const QString &str) const;
    QString fixPathToLocalOS(const QString &str) const;

#ifndef QT_BOOTSTRAPPED
    void runProcess(QProcess *proc, const QString &command, QProcess::ProcessChannel chan) const;
#endif

    int m_skipLevel;
    int m_loopLevel; // To report unexpected break() and next()s
#ifdef PROEVALUATOR_CUMULATIVE
    bool m_cumulative;
#else
    enum { m_cumulative = 0 };
#endif

    struct Location {
        Location() : pro(0), line(0) {}
        Location(ProFile *_pro, int _line) : pro(_pro), line(_line) {}
        ProFile *pro;
        int line;
    };

    Location m_current; // Currently evaluated location
    QStack<Location> m_locationStack; // All execution location changes
    QStack<ProFile *> m_profileStack; // Includes only

    QString m_outputDir;

    int m_listCount;
    ProFunctionDefs m_functionDefs;
    ProStringList m_returnValue;
    QStack<QHash<ProString, ProStringList> > m_valuemapStack;         // VariableName must be us-ascii, the content however can be non-us-ascii.
    QString m_tmp1, m_tmp2, m_tmp3, m_tmp[2]; // Temporaries for efficient toQString

    QMakeGlobals *m_option;
    QMakeParser *m_parser;
    ProFileEvaluatorHandler *m_handler;

    enum ExpandFunc {
        E_INVALID = 0, E_MEMBER, E_FIRST, E_LAST, E_SIZE, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
        E_SPRINTF, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
        E_FIND, E_SYSTEM, E_UNIQUE, E_QUOTE, E_ESCAPE_EXPAND,
        E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE,
        E_REPLACE, E_SORT_DEPENDS, E_RESOLVE_DEPENDS
    };

    enum TestFunc {
        T_INVALID = 0, T_REQUIRES, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
        T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
        T_RETURN, T_BREAK, T_NEXT, T_DEFINED, T_CONTAINS, T_INFILE,
        T_COUNT, T_ISEMPTY, T_INCLUDE, T_LOAD, T_DEBUG, T_MESSAGE, T_IF
    };

    enum VarName {
        V_LITERAL_DOLLAR, V_LITERAL_HASH, V_LITERAL_WHITESPACE,
        V_DIRLIST_SEPARATOR, V_DIR_SEPARATOR,
        V_OUT_PWD, V_PWD, V_IN_PWD,
        V__FILE_, V__LINE_, V__PRO_FILE_, V__PRO_FILE_PWD_,
        V_QMAKE_HOST_arch, V_QMAKE_HOST_name, V_QMAKE_HOST_os,
        V_QMAKE_HOST_version, V_QMAKE_HOST_version_string,
        V__DATE_, V__QMAKE_CACHE_
    };
};

static struct {
    QString field_sep;
    QString strtrue;
    QString strfalse;
    QString strunix;
    QString strmacx;
    QString strmac;
    QString strwin32;
    QString strsymbian;
    ProString strCONFIG;
    ProString strARGS;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    ProString strTEMPLATE;
    ProString strQMAKE_DIR_SEP;
    QHash<ProString, int> expands;
    QHash<ProString, int> functions;
    QHash<ProString, int> varList;
    QHash<ProString, ProString> varMap;
    QRegExp reg_variableName;
    ProStringList fakeValue;
} statics;

void ProFileEvaluator::Private::initStatics()
{
    if (!statics.field_sep.isNull())
        return;

    statics.field_sep = QLatin1String(" ");
    statics.strtrue = QLatin1String("true");
    statics.strfalse = QLatin1String("false");
    statics.strunix = QLatin1String("unix");
    statics.strmacx = QLatin1String("macx");
    statics.strmac = QLatin1String("mac");
    statics.strwin32 = QLatin1String("win32");
    statics.strsymbian = QLatin1String("symbian");
    statics.strCONFIG = ProString("CONFIG");
    statics.strARGS = ProString("ARGS");
    statics.strDot = QLatin1String(".");
    statics.strDotDot = QLatin1String("..");
    statics.strever = QLatin1String("ever");
    statics.strforever = QLatin1String("forever");
    statics.strTEMPLATE = ProString("TEMPLATE");
    statics.strQMAKE_DIR_SEP = ProString("QMAKE_DIR_SEP");

    statics.reg_variableName.setPattern(QLatin1String("\\$\\(.*\\)"));
    statics.reg_variableName.setMinimal(true);

    statics.fakeValue = ProStringList(ProString("_FAKE_")); // It has to have a unique begin() value

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
        { "join", E_JOIN },
        { "split", E_SPLIT },
        { "basename", E_BASENAME },
        { "dirname", E_DIRNAME },
        { "section", E_SECTION },
        { "find", E_FIND },
        { "system", E_SYSTEM },
        { "unique", E_UNIQUE },
        { "quote", E_QUOTE },
        { "escape_expand", E_ESCAPE_EXPAND },
        { "upper", E_UPPER },
        { "lower", E_LOWER },
        { "re_escape", E_RE_ESCAPE },
        { "files", E_FILES },
        { "prompt", E_PROMPT }, // interactive, so cannot be implemented
        { "replace", E_REPLACE },
        { "sort_depends", E_SORT_DEPENDS },
        { "resolve_depends", E_RESOLVE_DEPENDS }
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

    static const char * const names[] = {
        "LITERAL_DOLLAR", "LITERAL_HASH", "LITERAL_WHITESPACE",
        "DIRLIST_SEPARATOR", "DIR_SEPARATOR",
        "OUT_PWD", "PWD", "IN_PWD",
        "_FILE_", "_LINE_", "_PRO_FILE_", "_PRO_FILE_PWD_",
        "QMAKE_HOST.arch", "QMAKE_HOST.name", "QMAKE_HOST.os",
        "QMAKE_HOST.version", "QMAKE_HOST.version_string",
        "_DATE_", "_QMAKE_CACHE_"
    };
    for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i)
        statics.varList.insert(ProString(names[i]), i);

    static const struct {
        const char * const oldname, * const newname;
    } mapInits[] = {
        { "INTERFACES", "FORMS" },
        { "QMAKE_POST_BUILD", "QMAKE_POST_LINK" },
        { "TARGETDEPS", "POST_TARGETDEPS" },
        { "LIBPATH", "QMAKE_LIBDIR" },
        { "QMAKE_EXT_MOC", "QMAKE_EXT_CPP_MOC" },
        { "QMAKE_MOD_MOC", "QMAKE_H_MOD_MOC" },
        { "QMAKE_LFLAGS_SHAPP", "QMAKE_LFLAGS_APP" },
        { "PRECOMPH", "PRECOMPILED_HEADER" },
        { "PRECOMPCPP", "PRECOMPILED_SOURCE" },
        { "INCPATH", "INCLUDEPATH" },
        { "QMAKE_EXTRA_WIN_COMPILERS", "QMAKE_EXTRA_COMPILERS" },
        { "QMAKE_EXTRA_UNIX_COMPILERS", "QMAKE_EXTRA_COMPILERS" },
        { "QMAKE_EXTRA_WIN_TARGETS", "QMAKE_EXTRA_TARGETS" },
        { "QMAKE_EXTRA_UNIX_TARGETS", "QMAKE_EXTRA_TARGETS" },
        { "QMAKE_EXTRA_UNIX_INCLUDES", "QMAKE_EXTRA_INCLUDES" },
        { "QMAKE_EXTRA_UNIX_VARIABLES", "QMAKE_EXTRA_VARIABLES" },
        { "QMAKE_RPATH", "QMAKE_LFLAGS_RPATH" },
        { "QMAKE_FRAMEWORKDIR", "QMAKE_FRAMEWORKPATH" },
        { "QMAKE_FRAMEWORKDIR_FLAGS", "QMAKE_FRAMEWORKPATH_FLAGS" }
    };
    for (unsigned i = 0; i < sizeof(mapInits)/sizeof(mapInits[0]); ++i)
        statics.varMap.insert(ProString(mapInits[i].oldname),
                              ProString(mapInits[i].newname));
}

const ProString &ProFileEvaluator::Private::map(const ProString &var)
{
    QHash<ProString, ProString>::ConstIterator it = statics.varMap.constFind(var);
    return (it != statics.varMap.constEnd()) ? it.value() : var;
}


ProFileEvaluator::Private::Private(QMakeGlobals *option,
                                   QMakeParser *parser, ProFileEvaluatorHandler *handler)
  : m_option(option), m_parser(parser), m_handler(handler)
{
    // So that single-threaded apps don't have to call initialize() for now.
    initStatics();

    // Configuration, more or less
#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = true;
#endif

    // Evaluator state
    m_skipLevel = 0;
    m_loopLevel = 0;
    m_listCount = 0;
    m_valuemapStack.push(QHash<ProString, ProStringList>());
}

ProFileEvaluator::Private::~Private()
{
}

//////// Evaluator tools /////////

uint ProFileEvaluator::Private::getBlockLen(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    len |= (uint)*tokPtr++ << 16;
    return len;
}

ProString ProFileEvaluator::Private::getStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    ProString ret(m_current.pro->items(), tokPtr - m_current.pro->tokPtr(), len, NoHash);
    ret.setSource(m_current.pro);
    tokPtr += len;
    return ret;
}

ProString ProFileEvaluator::Private::getHashStr(const ushort *&tokPtr)
{
    uint hash = getBlockLen(tokPtr);
    uint len = *tokPtr++;
    ProString ret(m_current.pro->items(), tokPtr - m_current.pro->tokPtr(), len, hash);
    tokPtr += len;
    return ret;
}

void ProFileEvaluator::Private::skipStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    tokPtr += len;
}

void ProFileEvaluator::Private::skipHashStr(const ushort *&tokPtr)
{
    tokPtr += 2;
    uint len = *tokPtr++;
    tokPtr += len;
}

// FIXME: this should not build new strings for direct sections.
// Note that the E_SPRINTF and E_LIST implementations rely on the deep copy.
ProStringList ProFileEvaluator::Private::split_value_list(const QString &vals, const ProFile *source)
{
    QString build;
    ProStringList ret;
    QStack<char> quote;

    const ushort SPACE = ' ';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';
    const ushort BACKSLASH = '\\';

    if (!source)
        source = currentProFile();

    ushort unicode;
    const QChar *vals_data = vals.data();
    const int vals_len = vals.length();
    for (int x = 0, parens = 0; x < vals_len; x++) {
        unicode = vals_data[x].unicode();
        if (x != (int)vals_len-1 && unicode == BACKSLASH &&
            (vals_data[x+1].unicode() == SINGLEQUOTE || vals_data[x+1].unicode() == DOUBLEQUOTE)) {
            build += vals_data[x++]; //get that 'escape'
        } else if (!quote.isEmpty() && unicode == quote.top()) {
            quote.pop();
        } else if (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE) {
            quote.push(unicode);
        } else if (unicode == RPAREN) {
            --parens;
        } else if (unicode == LPAREN) {
            ++parens;
        }

        if (!parens && quote.isEmpty() && vals_data[x] == SPACE) {
            ret << ProString(build, NoHash).setSource(source);
            build.clear();
        } else {
            build += vals_data[x];
        }
    }
    if (!build.isEmpty())
        ret << ProString(build, NoHash).setSource(source);
    return ret;
}

static void zipEmpty(ProStringList *value)
{
    for (int i = value->size(); --i >= 0;)
        if (value->at(i).isEmpty())
            value->remove(i);
}

static void insertUnique(ProStringList *varlist, const ProStringList &value)
{
    foreach (const ProString &str, value)
        if (!str.isEmpty() && !varlist->contains(str))
            varlist->append(str);
}

static void removeAll(ProStringList *varlist, const ProString &value)
{
    for (int i = varlist->size(); --i >= 0; )
        if (varlist->at(i) == value)
            varlist->remove(i);
}

static void removeEach(ProStringList *varlist, const ProStringList &value)
{
    foreach (const ProString &str, value)
        if (!str.isEmpty())
            removeAll(varlist, str);
}

static void replaceInList(ProStringList *varlist,
        const QRegExp &regexp, const QString &replace, bool global, QString &tmp)
{
    for (ProStringList::Iterator varit = varlist->begin(); varit != varlist->end(); ) {
        QString val = varit->toQString(tmp);
        QString copy = val; // Force detach and have a reference value
        val.replace(regexp, replace);
        if (!val.isSharedWith(copy)) {
            if (val.isEmpty()) {
                varit = varlist->erase(varit);
            } else {
                (*varit).setValue(val, NoHash);
                ++varit;
            }
            if (!global)
                break;
        } else {
            ++varit;
        }
    }
}

QString ProFileEvaluator::Private::expandEnvVars(const QString &str) const
{
    QString string = str;
    int rep;
    QRegExp reg_variableName = statics.reg_variableName; // Copy for thread safety
    while ((rep = reg_variableName.indexIn(string)) != -1)
        string.replace(rep, reg_variableName.matchedLength(),
                       m_option->getEnv(string.mid(rep + 2, reg_variableName.matchedLength() - 3)));
    return string;
}

// This is braindead, but we want qmake compat
QString ProFileEvaluator::Private::fixPathToLocalOS(const QString &str) const
{
    QString string = expandEnvVars(str);

    if (string.length() > 2 && string.at(0).isLetter() && string.at(1) == QLatin1Char(':'))
        string[0] = string[0].toLower();

#if defined(Q_OS_WIN32)
    string.replace(QLatin1Char('/'), QLatin1Char('\\'));
#else
    string.replace(QLatin1Char('\\'), QLatin1Char('/'));
#endif
    return string;
}

static bool isTrue(const ProString &_str, QString &tmp)
{
    const QString &str = _str.toQString(tmp);
    return !str.compare(statics.strtrue, Qt::CaseInsensitive) || str.toInt();
}

//////// Evaluator /////////

static ALWAYS_INLINE void addStr(
        const ProString &str, ProStringList *ret, bool &pending, bool joined)
{
    if (joined) {
        ret->last().append(str, &pending);
    } else {
        if (!pending) {
            pending = true;
            *ret << str;
        } else {
            ret->last().append(str);
        }
    }
}

static ALWAYS_INLINE void addStrList(
        const ProStringList &list, ushort tok, ProStringList *ret, bool &pending, bool joined)
{
    if (!list.isEmpty()) {
        if (joined) {
            ret->last().append(list, &pending, !(tok & TokQuoted));
        } else {
            if (tok & TokQuoted) {
                if (!pending) {
                    pending = true;
                    *ret << ProString();
                }
                ret->last().append(list);
            } else {
                if (!pending) {
                    // Another qmake bizzarity: if nothing is pending and the
                    // first element is empty, it will be eaten
                    if (!list.at(0).isEmpty()) {
                        // The common case
                        pending = true;
                        *ret += list;
                        return;
                    }
                } else {
                    ret->last().append(list.at(0));
                }
                // This is somewhat slow, but a corner case
                for (int j = 1; j < list.size(); ++j) {
                    pending = true;
                    *ret << list.at(j);
                }
            }
        }
    }
}

void ProFileEvaluator::Private::evaluateExpression(
        const ushort *&tokPtr, ProStringList *ret, bool joined)
{
    if (joined)
        *ret << ProString();
    bool pending = false;
    forever {
        ushort tok = *tokPtr++;
        if (tok & TokNewStr)
            pending = false;
        ushort maskedTok = tok & TokMask;
        switch (maskedTok) {
        case TokLine:
            m_current.line = *tokPtr++;
            break;
        case TokLiteral:
            addStr(getStr(tokPtr), ret, pending, joined);
            break;
        case TokHashLiteral:
            addStr(getHashStr(tokPtr), ret, pending, joined);
            break;
        case TokVariable:
            addStrList(values(map(getHashStr(tokPtr))), tok, ret, pending, joined);
            break;
        case TokProperty:
            addStr(ProString(propertyValue(
                      getStr(tokPtr).toQString(m_tmp1), true), NoHash).setSource(currentProFile()),
                   ret, pending, joined);
            break;
        case TokEnvVar:
            addStrList(split_value_list(m_option->getEnv(getStr(tokPtr).toQString(m_tmp1))),
                       tok, ret, pending, joined);
            break;
        case TokFuncName: {
            ProString func = getHashStr(tokPtr);
            addStrList(evaluateExpandFunction(func, tokPtr), tok, ret, pending, joined);
            break; }
        default:
            tokPtr--;
            return;
        }
    }
}

void ProFileEvaluator::Private::skipExpression(const ushort *&pTokPtr)
{
    const ushort *tokPtr = pTokPtr;
    forever {
        ushort tok = *tokPtr++;
        switch (tok) {
        case TokLine:
            m_current.line = *tokPtr++;
            break;
        case TokValueTerminator:
        case TokFuncTerminator:
            pTokPtr = tokPtr;
            return;
        case TokArgSeparator:
            break;
        default:
            switch (tok & TokMask) {
            case TokLiteral:
            case TokProperty:
            case TokEnvVar:
                skipStr(tokPtr);
                break;
            case TokHashLiteral:
            case TokVariable:
                skipHashStr(tokPtr);
                break;
            case TokFuncName:
                skipHashStr(tokPtr);
                pTokPtr = tokPtr;
                skipExpression(pTokPtr);
                tokPtr = pTokPtr;
                break;
            default:
                Q_ASSERT_X(false, "skipExpression", "Unrecognized token");
                break;
            }
        }
    }
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProBlock(
        ProFile *pro, const ushort *tokPtr)
{
    m_current.pro = pro;
    m_current.line = 0;
    return visitProBlock(tokPtr);
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProBlock(
        const ushort *tokPtr)
{
    ProStringList curr;
    bool okey = true, or_op = false, invert = false;
    uint blockLen;
    VisitReturn ret = ReturnTrue;
    while (ushort tok = *tokPtr++) {
        switch (tok) {
        case TokLine:
            m_current.line = *tokPtr++;
            continue;
        case TokAssign:
        case TokAppend:
        case TokAppendUnique:
        case TokRemove:
        case TokReplace:
            visitProVariable(tok, curr, tokPtr);
            curr.clear();
            continue;
        case TokBranch:
            blockLen = getBlockLen(tokPtr);
            if (m_cumulative) {
                if (!okey)
                    m_skipLevel++;
                ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
                tokPtr += blockLen;
                blockLen = getBlockLen(tokPtr);
                if (!okey)
                    m_skipLevel--;
                else
                    m_skipLevel++;
                if ((ret == ReturnTrue || ret == ReturnFalse) && blockLen)
                    ret = visitProBlock(tokPtr);
                if (okey)
                    m_skipLevel--;
            } else {
                if (okey)
                    ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
                tokPtr += blockLen;
                blockLen = getBlockLen(tokPtr);
                if (!okey)
                    ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            break;
        case TokForLoop:
            if (m_cumulative) { // This is a no-win situation, so just pretend it's no loop
                skipHashStr(tokPtr);
                uint exprLen = getBlockLen(tokPtr);
                tokPtr += exprLen;
                blockLen = getBlockLen(tokPtr);
                ret = visitProBlock(tokPtr);
            } else if (okey != or_op) {
                const ProString &variable = getHashStr(tokPtr);
                uint exprLen = getBlockLen(tokPtr);
                const ushort *exprPtr = tokPtr;
                tokPtr += exprLen;
                blockLen = getBlockLen(tokPtr);
                ret = visitProLoop(variable, exprPtr, tokPtr);
            } else {
                skipHashStr(tokPtr);
                uint exprLen = getBlockLen(tokPtr);
                tokPtr += exprLen;
                blockLen = getBlockLen(tokPtr);
                ret = ReturnTrue;
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            break;
        case TokTestDef:
        case TokReplaceDef:
            if (m_cumulative || okey != or_op) {
                const ProString &name = getHashStr(tokPtr);
                blockLen = getBlockLen(tokPtr);
                visitProFunctionDef(tok, name, tokPtr);
            } else {
                skipHashStr(tokPtr);
                blockLen = getBlockLen(tokPtr);
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            continue;
        case TokNot:
            invert ^= true;
            continue;
        case TokAnd:
            or_op = false;
            continue;
        case TokOr:
            or_op = true;
            continue;
        case TokCondition:
            if (!m_skipLevel && okey != or_op) {
                if (curr.size() != 1) {
                    if (!m_cumulative || !curr.isEmpty())
                        evalError(fL1S("Conditional must expand to exactly one word."));
                    okey = false;
                } else {
                    okey = isActiveConfig(curr.at(0).toQString(m_tmp2), true) ^ invert;
                }
            }
            or_op = !okey; // tentatively force next evaluation
            invert = false;
            curr.clear();
            continue;
        case TokTestCall:
            if (!m_skipLevel && okey != or_op) {
                if (curr.size() != 1) {
                    if (!m_cumulative || !curr.isEmpty())
                        evalError(fL1S("Test name must expand to exactly one word."));
                    skipExpression(tokPtr);
                    okey = false;
                } else {
                    ret = evaluateConditionalFunction(curr.at(0), tokPtr);
                    switch (ret) {
                    case ReturnTrue: okey = true; break;
                    case ReturnFalse: okey = false; break;
                    default: return ret;
                    }
                    okey ^= invert;
                }
            } else if (m_cumulative) {
                m_skipLevel++;
                if (curr.size() != 1)
                    skipExpression(tokPtr);
                else
                    evaluateConditionalFunction(curr.at(0), tokPtr);
                m_skipLevel--;
            } else {
                skipExpression(tokPtr);
            }
            or_op = !okey; // tentatively force next evaluation
            invert = false;
            curr.clear();
            continue;
        default: {
                const ushort *oTokPtr = --tokPtr;
                evaluateExpression(tokPtr, &curr, false);
                if (tokPtr != oTokPtr)
                    continue;
            }
            Q_ASSERT_X(false, "visitProBlock", "unexpected item type");
        }
        if (ret != ReturnTrue && ret != ReturnFalse)
            break;
    }
    return ret;
}


void ProFileEvaluator::Private::visitProFunctionDef(
        ushort tok, const ProString &name, const ushort *tokPtr)
{
    QHash<ProString, ProFunctionDef> *hash =
            (tok == TokTestDef
             ? &m_functionDefs.testFunctions
             : &m_functionDefs.replaceFunctions);
    hash->insert(name, ProFunctionDef(m_current.pro, tokPtr - m_current.pro->tokPtr()));
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProLoop(
        const ProString &_variable, const ushort *exprPtr, const ushort *tokPtr)
{
    VisitReturn ret = ReturnTrue;
    bool infinite = false;
    int index = 0;
    ProString variable;
    ProStringList oldVarVal;
    ProString it_list = expandVariableReferences(exprPtr, 0, true).at(0);
    if (_variable.isEmpty()) {
        if (it_list != statics.strever) {
            evalError(fL1S("Invalid loop expression."));
            return ReturnFalse;
        }
        it_list = ProString(statics.strforever);
    } else {
        variable = map(_variable);
        oldVarVal = valuesDirect(variable);
    }
    ProStringList list = valuesDirect(it_list);
    if (list.isEmpty()) {
        if (it_list == statics.strforever) {
            infinite = true;
        } else {
            const QString &itl = it_list.toQString(m_tmp1);
            int dotdot = itl.indexOf(statics.strDotDot);
            if (dotdot != -1) {
                bool ok;
                int start = itl.left(dotdot).toInt(&ok);
                if (ok) {
                    int end = itl.mid(dotdot+2).toInt(&ok);
                    if (ok) {
                        if (start < end) {
                            for (int i = start; i <= end; i++)
                                list << ProString(QString::number(i), NoHash);
                        } else {
                            for (int i = start; i >= end; i--)
                                list << ProString(QString::number(i), NoHash);
                        }
                    }
                }
            }
        }
    }

    m_loopLevel++;
    forever {
        if (infinite) {
            if (!variable.isEmpty())
                m_valuemapStack.top()[variable] = ProStringList(ProString(QString::number(index++), NoHash));
            if (index > 1000) {
                evalError(fL1S("ran into infinite loop (> 1000 iterations)."));
                break;
            }
        } else {
            ProString val;
            do {
                if (index >= list.count())
                    goto do_break;
                val = list.at(index++);
            } while (val.isEmpty()); // stupid, but qmake is like that
            m_valuemapStack.top()[variable] = ProStringList(val);
        }

        ret = visitProBlock(tokPtr);
        switch (ret) {
        case ReturnTrue:
        case ReturnFalse:
            break;
        case ReturnNext:
            ret = ReturnTrue;
            break;
        case ReturnBreak:
            ret = ReturnTrue;
            goto do_break;
        default:
            goto do_break;
        }
    }
  do_break:
    m_loopLevel--;

    if (!variable.isEmpty())
        m_valuemapStack.top()[variable] = oldVarVal;
    return ret;
}

void ProFileEvaluator::Private::visitProVariable(
        ushort tok, const ProStringList &curr, const ushort *&tokPtr)
{
    int sizeHint = *tokPtr++;

    if (curr.size() != 1) {
        skipExpression(tokPtr);
        if (!m_cumulative || !curr.isEmpty())
            evalError(fL1S("Left hand side of assignment must expand to exactly one word."));
        return;
    }
    const ProString &varName = map(curr.first());

    if (tok == TokReplace) {      // ~=
        // DEFINES ~= s/a/b/?[gqi]

        const ProStringList &varVal = expandVariableReferences(tokPtr, sizeHint, true);
        const QString &val = varVal.at(0).toQString(m_tmp1);
        if (val.length() < 4 || val.at(0) != QLatin1Char('s')) {
            evalError(fL1S("the ~= operator can handle only the s/// function."));
            return;
        }
        QChar sep = val.at(1);
        QStringList func = val.split(sep);
        if (func.count() < 3 || func.count() > 4) {
            evalError(fL1S("the s/// function expects 3 or 4 arguments."));
            return;
        }

        bool global = false, quote = false, case_sense = false;
        if (func.count() == 4) {
            global = func[3].indexOf(QLatin1Char('g')) != -1;
            case_sense = func[3].indexOf(QLatin1Char('i')) == -1;
            quote = func[3].indexOf(QLatin1Char('q')) != -1;
        }
        QString pattern = func[1];
        QString replace = func[2];
        if (quote)
            pattern = QRegExp::escape(pattern);

        QRegExp regexp(pattern, case_sense ? Qt::CaseSensitive : Qt::CaseInsensitive);

        if (!m_skipLevel || m_cumulative) {
            // We could make a union of modified and unmodified values,
            // but this will break just as much as it fixes, so leave it as is.
            replaceInList(&valuesRef(varName), regexp, replace, global, m_tmp2);
        }
    } else {
        ProStringList varVal = expandVariableReferences(tokPtr, sizeHint);
        switch (tok) {
        default: // whatever - cannot happen
        case TokAssign:          // =
            if (!m_cumulative) {
                if (!m_skipLevel) {
                    zipEmpty(&varVal);
                    m_valuemapStack.top()[varName] = varVal;
                }
            } else {
                zipEmpty(&varVal);
                if (!varVal.isEmpty()) {
                    // We are greedy for values. But avoid exponential growth.
                    ProStringList &v = valuesRef(varName);
                    if (v.isEmpty()) {
                        v = varVal;
                    } else {
                        ProStringList old = v;
                        v = varVal;
                        QSet<ProString> has;
                        has.reserve(v.size());
                        foreach (const ProString &s, v)
                            has.insert(s);
                        v.reserve(v.size() + old.size());
                        foreach (const ProString &s, old)
                            if (!has.contains(s))
                                v << s;
                    }
                }
            }
            break;
        case TokAppendUnique:    // *=
            if (!m_skipLevel || m_cumulative)
                insertUnique(&valuesRef(varName), varVal);
            break;
        case TokAppend:          // +=
            if (!m_skipLevel || m_cumulative) {
                zipEmpty(&varVal);
                valuesRef(varName) += varVal;
            }
            break;
        case TokRemove:       // -=
            if (!m_cumulative) {
                if (!m_skipLevel)
                    removeEach(&valuesRef(varName), varVal);
            } else {
                // We are stingy with our values, too.
            }
            break;
        }
    }
}

void ProFileEvaluator::Private::visitCmdLine(const QString &cmds)
{
    if (!cmds.isEmpty()) {
        if (ProFile *pro = m_parser->parsedProBlock(fL1S("(command line)"), cmds)) {
            m_locationStack.push(m_current);
            visitProBlock(pro, pro->tokPtr());
            m_current = m_locationStack.pop();
            pro->deref();
        }
    }
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProFile(
        ProFile *pro, ProFileEvaluatorHandler::EvalFileType type,
        ProFileEvaluator::LoadFlags flags)
{
    if (!m_cumulative && !pro->isOk())
        return ReturnFalse;

    m_handler->aboutToEval(currentProFile(), pro, type);
    m_profileStack.push(pro);
    if (flags & ProFileEvaluator::LoadPreFiles) {
#ifdef PROEVALUATOR_THREAD_SAFE
        {
            QMutexLocker locker(&m_option->mutex);
            if (m_option->base_inProgress) {
                QThreadPool::globalInstance()->releaseThread();
                m_option->cond.wait(&m_option->mutex);
                QThreadPool::globalInstance()->reserveThread();
            } else
#endif
            if (m_option->base_valuemap.isEmpty()) {
#ifdef PROEVALUATOR_THREAD_SAFE
                m_option->base_inProgress = true;
                locker.unlock();
#endif

#ifdef PROEVALUATOR_CUMULATIVE
                bool cumulative = m_cumulative;
                m_cumulative = false;
#endif

                // ### init QMAKE_QMAKE, QMAKE_SH
                // ### init QMAKE_EXT_{C,H,CPP,OBJ}
                // ### init TEMPLATE_PREFIX

                QString qmake_cache = m_option->cachefile;
                if (qmake_cache.isEmpty() && !m_outputDir.isEmpty())  { //find it as it has not been specified
                    QDir dir(m_outputDir);
                    forever {
                        qmake_cache = dir.path() + QLatin1String("/.qmake.cache");
                        if (IoUtils::exists(qmake_cache))
                            break;
                        if (!dir.cdUp() || dir.isRoot()) {
                            qmake_cache.clear();
                            break;
                        }
                    }
                }
                if (!qmake_cache.isEmpty()) {
                    qmake_cache = resolvePath(qmake_cache);
                    QHash<ProString, ProStringList> cache_valuemap;
                    if (evaluateFileInto(qmake_cache, ProFileEvaluatorHandler::EvalConfigFile,
                                         &cache_valuemap, 0, EvalProOnly)) {
                        if (m_option->qmakespec.isEmpty()) {
                            const ProStringList &vals = cache_valuemap.value(ProString("QMAKESPEC"));
                            if (!vals.isEmpty())
                                m_option->qmakespec = vals.first().toQString();
                        }
                    } else {
                        qmake_cache.clear();
                    }
                }
                m_option->cachefile = qmake_cache;

                QStringList mkspec_roots = qmakeMkspecPaths();

                QString qmakespec = expandEnvVars(m_option->qmakespec);
                if (qmakespec.isEmpty()) {
                    foreach (const QString &root, mkspec_roots) {
                        QString mkspec = root + QLatin1String("/default");
                        if (IoUtils::fileType(mkspec) == IoUtils::FileIsDir) {
                            qmakespec = mkspec;
                            break;
                        }
                    }
                    if (qmakespec.isEmpty()) {
                        m_handler->configError(fL1S("Could not find qmake configuration directory"));
                        // Unlike in qmake, not finding the spec is not critical ...
                    }
                }

                if (IoUtils::isRelativePath(qmakespec)) {
                    if (IoUtils::exists(currentDirectory() + QLatin1Char('/') + qmakespec
                                        + QLatin1String("/qmake.conf"))) {
                        qmakespec = currentDirectory() + QLatin1Char('/') + qmakespec;
                    } else if (!m_outputDir.isEmpty()
                               && IoUtils::exists(m_outputDir + QLatin1Char('/') + qmakespec
                                                  + QLatin1String("/qmake.conf"))) {
                        qmakespec = m_outputDir + QLatin1Char('/') + qmakespec;
                    } else {
                        foreach (const QString &root, mkspec_roots) {
                            QString mkspec = root + QLatin1Char('/') + qmakespec;
                            if (IoUtils::exists(mkspec)) {
                                qmakespec = mkspec;
                                goto cool;
                            }
                        }
                        m_handler->configError(fL1S("Could not find qmake configuration file"));
                        // Unlike in qmake, a missing config is not critical ...
                        qmakespec.clear();
                      cool: ;
                    }
                }

                if (!qmakespec.isEmpty()) {
                    m_option->qmakespec = QDir::cleanPath(qmakespec);

                    QString spec = m_option->qmakespec + QLatin1String("/qmake.conf");
                    if (!evaluateFileDirect(spec, ProFileEvaluatorHandler::EvalConfigFile,
                                            ProFileEvaluator::LoadProOnly)) {
                        m_handler->configError(
                                fL1S("Could not read qmake configuration file %1").arg(spec));
                    } else if (!m_option->cachefile.isEmpty()) {
                        evaluateFileDirect(m_option->cachefile,
                                           ProFileEvaluatorHandler::EvalConfigFile,
                                           ProFileEvaluator::LoadProOnly);
                    }
                    m_option->qmakespec_name = IoUtils::fileName(m_option->qmakespec).toString();
                    if (m_option->qmakespec_name == QLatin1String("default")) {
#ifdef Q_OS_UNIX
                        char buffer[1024];
                        int l = ::readlink(m_option->qmakespec.toLocal8Bit().constData(), buffer, 1024);
                        if (l != -1)
                            m_option->qmakespec_name =
                                    IoUtils::fileName(QString::fromLocal8Bit(buffer, l)).toString();
#else
                        // We can't resolve symlinks as they do on Unix, so configure.exe puts
                        // the source of the qmake.conf at the end of the default/qmake.conf in
                        // the QMAKESPEC_ORG variable.
                        const ProStringList &spec_org =
                                m_option->base_valuemap.value(ProString("QMAKESPEC_ORIGINAL"));
                        if (!spec_org.isEmpty())
                            m_option->qmakespec_name =
                                    IoUtils::fileName(spec_org.first().toQString()).toString();
#endif
                    }
                }

                m_option->base_valuemap = m_valuemapStack.top();
                m_option->base_functions = m_functionDefs;

#ifdef PROEVALUATOR_CUMULATIVE
                m_cumulative = cumulative;
#endif

#ifdef PROEVALUATOR_THREAD_SAFE
                locker.relock();
                m_option->base_inProgress = false;
                m_option->cond.wakeAll();
#endif
                goto fresh;
            }
#ifdef PROEVALUATOR_THREAD_SAFE
        }
#endif

            m_valuemapStack.top() = m_option->base_valuemap;
            m_functionDefs = m_option->base_functions;

          fresh:
            evaluateFeatureFile(QLatin1String("default_pre.prf"));

            ProStringList &tgt = m_valuemapStack.top()[ProString("TARGET")];
            if (tgt.isEmpty())
                tgt.append(ProString(QFileInfo(pro->fileName()).baseName(), NoHash));

        visitCmdLine(m_option->precmds);
    }

    visitProBlock(pro, pro->tokPtr());

    if (flags & ProFileEvaluator::LoadPostFiles) {
        visitCmdLine(m_option->postcmds);

            evaluateFeatureFile(QLatin1String("default_post.prf"));

            QSet<QString> processed;
            forever {
                bool finished = true;
                ProStringList configs = valuesDirect(statics.strCONFIG);
                for (int i = configs.size() - 1; i >= 0; --i) {
                    QString config = configs.at(i).toQString(m_tmp1).toLower();
                    if (!processed.contains(config)) {
                        config.detach();
                        processed.insert(config);
                        if (evaluateFeatureFile(config)) {
                            finished = false;
                            break;
                        }
                    }
                }
                if (finished)
                    break;
            }
    }
    m_profileStack.pop();
    m_handler->doneWithEval(currentProFile());

    return ReturnTrue;
}


QStringList ProFileEvaluator::Private::qmakeMkspecPaths() const
{
    QStringList ret;
    const QString concat = QLatin1String("/mkspecs");

    QString qmakepath = m_option->getEnv(QLatin1String("QMAKEPATH"));
    if (!qmakepath.isEmpty())
        foreach (const QString &it, qmakepath.split(m_option->dirlist_sep))
            ret << QDir::cleanPath(it) + concat;

    QString builtIn = propertyValue(QLatin1String("QT_INSTALL_DATA"), false) + concat;
    if (!ret.contains(builtIn))
        ret << builtIn;

    return ret;
}

QStringList ProFileEvaluator::Private::qmakeFeaturePaths() const
{
    QString mkspecs_concat = QLatin1String("/mkspecs");
    QString features_concat = QLatin1String("/features");
    QStringList concat;

    validateModes();
    switch (m_option->target_mode) {
    case QMakeGlobals::TARG_MACX_MODE:
        concat << QLatin1String("/features/mac");
        concat << QLatin1String("/features/macx");
        concat << QLatin1String("/features/unix");
        break;
    default: // Can't happen, just make the compiler shut up
    case QMakeGlobals::TARG_UNIX_MODE:
        concat << QLatin1String("/features/unix");
        break;
    case QMakeGlobals::TARG_WIN_MODE:
        concat << QLatin1String("/features/win32");
        break;
    case QMakeGlobals::TARG_SYMBIAN_MODE:
        concat << QLatin1String("/features/symbian");
        break;
    }
    concat << features_concat;

    QStringList feature_roots;

    QString mkspec_path = m_option->getEnv(QLatin1String("QMAKEFEATURES"));
    if (!mkspec_path.isEmpty())
        foreach (const QString &f, mkspec_path.split(m_option->dirlist_sep))
            feature_roots += resolvePath(f);

    feature_roots += propertyValue(QLatin1String("QMAKEFEATURES"), false).split(
            m_option->dirlist_sep, QString::SkipEmptyParts);

    if (!m_option->cachefile.isEmpty()) {
        QString path = m_option->cachefile.left(m_option->cachefile.lastIndexOf((ushort)'/'));
        foreach (const QString &concat_it, concat)
            feature_roots << (path + concat_it);
    }

    QString qmakepath = m_option->getEnv(QLatin1String("QMAKEPATH"));
    if (!qmakepath.isNull()) {
        const QStringList lst = qmakepath.split(m_option->dirlist_sep);
        foreach (const QString &item, lst) {
            QString citem = resolvePath(item);
            foreach (const QString &concat_it, concat)
                feature_roots << (citem + mkspecs_concat + concat_it);
        }
    }

    if (!m_option->qmakespec.isEmpty()) {
        QString qmakespec = resolvePath(m_option->qmakespec);
        feature_roots << (qmakespec + features_concat);

        QDir specdir(qmakespec);
        while (!specdir.isRoot()) {
            if (!specdir.cdUp() || specdir.isRoot())
                break;
            if (IoUtils::exists(specdir.path() + features_concat)) {
                foreach (const QString &concat_it, concat)
                    feature_roots << (specdir.path() + concat_it);
                break;
            }
        }
    }

    foreach (const QString &concat_it, concat)
        feature_roots << (propertyValue(QLatin1String("QT_INSTALL_PREFIX"), false) +
                          mkspecs_concat + concat_it);
    foreach (const QString &concat_it, concat)
        feature_roots << (propertyValue(QLatin1String("QT_INSTALL_DATA"), false) +
                          mkspecs_concat + concat_it);

    for (int i = 0; i < feature_roots.count(); ++i)
        if (!feature_roots.at(i).endsWith((ushort)'/'))
            feature_roots[i].append((ushort)'/');

    feature_roots.removeDuplicates();

    return feature_roots;
}

QString ProFileEvaluator::Private::propertyValue(const QString &name, bool complain) const
{
    if (m_option->properties.contains(name))
        return m_option->properties.value(name);
    if (name == QLatin1String("QMAKE_MKSPECS"))
        return qmakeMkspecPaths().join(m_option->dirlist_sep);
    if (name == QLatin1String("QMAKE_VERSION"))
        return QLatin1String("1.0");        //### FIXME
    if (complain)
        evalError(fL1S("Querying unknown property %1").arg(name));
    return QString();
}

ProFile *ProFileEvaluator::Private::currentProFile() const
{
    if (m_profileStack.count() > 0)
        return m_profileStack.top();
    return 0;
}

QString ProFileEvaluator::Private::currentFileName() const
{
    ProFile *pro = currentProFile();
    if (pro)
        return pro->fileName();
    return QString();
}

QString ProFileEvaluator::Private::currentDirectory() const
{
    ProFile *cur = m_profileStack.top();
    return cur->directoryName();
}

#ifndef QT_BOOTSTRAPPED
void ProFileEvaluator::Private::runProcess(QProcess *proc, const QString &command,
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
    m_handler->evalError(QString(), 0, QString::fromLocal8Bit(errout));
}
#endif

// The (QChar*)current->constData() constructs below avoid pointless detach() calls
// FIXME: This is inefficient. Should not make new string if it is a straight subsegment
static ALWAYS_INLINE void appendChar(ushort unicode,
    QString *current, QChar **ptr, ProString *pending)
{
    if (!pending->isEmpty()) {
        int len = pending->size();
        current->resize(current->size() + len);
        ::memcpy((QChar*)current->constData(), pending->constData(), len * 2);
        pending->clear();
        *ptr = (QChar*)current->constData() + len;
    }
    *(*ptr)++ = QChar(unicode);
}

static void appendString(const ProString &string,
    QString *current, QChar **ptr, ProString *pending)
{
    if (string.isEmpty())
        return;
    QChar *uc = (QChar*)current->constData();
    int len;
    if (*ptr != uc) {
        len = *ptr - uc;
        current->resize(current->size() + string.size());
    } else if (!pending->isEmpty()) {
        len = pending->size();
        current->resize(current->size() + len + string.size());
        ::memcpy((QChar*)current->constData(), pending->constData(), len * 2);
        pending->clear();
    } else {
        *pending = string;
        return;
    }
    *ptr = (QChar*)current->constData() + len;
    ::memcpy(*ptr, string.constData(), string.size() * 2);
    *ptr += string.size();
}

static void flushCurrent(ProStringList *ret,
    QString *current, QChar **ptr, ProString *pending, bool joined)
{
    QChar *uc = (QChar*)current->constData();
    int len = *ptr - uc;
    if (len) {
        ret->append(ProString(QString(uc, len), NoHash));
        *ptr = uc;
    } else if (!pending->isEmpty()) {
        ret->append(*pending);
        pending->clear();
    } else if (joined) {
        ret->append(ProString());
    }
}

static inline void flushFinal(ProStringList *ret,
    const QString &current, const QChar *ptr, const ProString &pending,
    const ProString &str, bool replaced, bool joined)
{
    int len = ptr - current.data();
    if (len) {
        if (!replaced && len == str.size())
            ret->append(str);
        else
            ret->append(ProString(QString(current.data(), len), NoHash));
    } else if (!pending.isEmpty()) {
        ret->append(pending);
    } else if (joined) {
        ret->append(ProString());
    }
}

ProStringList ProFileEvaluator::Private::expandVariableReferences(
    const ProString &str, int *pos, bool joined)
{
    ProStringList ret;
//    if (ok)
//        *ok = true;
    if (str.isEmpty() && !pos)
        return ret;

    const ushort LSQUARE = '[';
    const ushort RSQUARE = ']';
    const ushort LCURLY = '{';
    const ushort RCURLY = '}';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort DOLLAR = '$';
    const ushort BACKSLASH = '\\';
    const ushort UNDERSCORE = '_';
    const ushort DOT = '.';
    const ushort SPACE = ' ';
    const ushort TAB = '\t';
    const ushort COMMA = ',';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';

    ushort unicode, quote = 0, parens = 0;
    const ushort *str_data = (const ushort *)str.constData();
    const int str_len = str.size();

    ProString var, args;

    bool replaced = false;
    bool putSpace = false;
    QString current; // Buffer for successively assembled string segments
    current.resize(str.size());
    QChar *ptr = current.data();
    ProString pending; // Buffer for string segments from variables
    // Only one of the above buffers can be filled at a given time.
    for (int i = pos ? *pos : 0; i < str_len; ++i) {
        unicode = str_data[i];
        if (unicode == DOLLAR) {
            if (str_len > i+2 && str_data[i+1] == DOLLAR) {
                ++i;
                ushort term = 0;
                enum { VAR, ENVIRON, FUNCTION, PROPERTY } var_type = VAR;
                unicode = str_data[++i];
                if (unicode == LSQUARE) {
                    unicode = str_data[++i];
                    term = RSQUARE;
                    var_type = PROPERTY;
                } else if (unicode == LCURLY) {
                    unicode = str_data[++i];
                    var_type = VAR;
                    term = RCURLY;
                } else if (unicode == LPAREN) {
                    unicode = str_data[++i];
                    var_type = ENVIRON;
                    term = RPAREN;
                }
                int name_start = i;
                forever {
                    if (!(unicode & (0xFF<<8)) &&
                       unicode != DOT && unicode != UNDERSCORE &&
                       //unicode != SINGLEQUOTE && unicode != DOUBLEQUOTE &&
                       (unicode < 'a' || unicode > 'z') && (unicode < 'A' || unicode > 'Z') &&
                       (unicode < '0' || unicode > '9'))
                        break;
                    if (++i == str_len)
                        break;
                    unicode = str_data[i];
                    // at this point, i points to either the 'term' or 'next' character (which is in unicode)
                }
                var = str.mid(name_start, i - name_start);
                if (var_type == VAR && unicode == LPAREN) {
                    var_type = FUNCTION;
                    name_start = i + 1;
                    int depth = 0;
                    forever {
                        if (++i == str_len)
                            break;
                        unicode = str_data[i];
                        if (unicode == LPAREN) {
                            depth++;
                        } else if (unicode == RPAREN) {
                            if (!depth)
                                break;
                            --depth;
                        }
                    }
                    args = str.mid(name_start, i - name_start);
                    if (++i < str_len)
                        unicode = str_data[i];
                    else
                        unicode = 0;
                    // at this point i is pointing to the 'next' character (which is in unicode)
                    // this might actually be a term character since you can do $${func()}
                }
                if (term) {
                    if (unicode != term) {
                        evalError(fL1S("Missing %1 terminator [found %2]")
                                  .arg(QChar(term))
                                  .arg(unicode ? QString(unicode) : fL1S("end-of-line")));
//                        if (ok)
//                            *ok = false;
                        if (pos)
                            *pos = str_len;
                        return ProStringList();
                    }
                } else {
                    // move the 'cursor' back to the last char of the thing we were looking at
                    --i;
                }

                ProStringList replacement;
                if (var_type == ENVIRON) {
                    replacement = split_value_list(m_option->getEnv(var.toQString(m_tmp1)));
                } else if (var_type == PROPERTY) {
                    replacement << ProString(propertyValue(var.toQString(m_tmp1), true), NoHash);
                } else if (var_type == FUNCTION) {
                    replacement += evaluateExpandFunction(var, args);
                } else if (var_type == VAR) {
                    replacement = values(map(var));
                }
                if (!replacement.isEmpty()) {
                    if (quote || joined) {
                        if (putSpace) {
                            putSpace = false;
                            if (!replacement.at(0).isEmpty()) // Bizarre, indeed
                                appendChar(' ', &current, &ptr, &pending);
                        }
                        appendString(ProString(replacement.join(statics.field_sep), NoHash),
                                     &current, &ptr, &pending);
                    } else {
                        appendString(replacement.at(0), &current, &ptr, &pending);
                        if (replacement.size() > 1) {
                            flushCurrent(&ret, &current, &ptr, &pending, false);
                            int j = 1;
                            if (replacement.size() > 2) {
                                // FIXME: ret.reserve(ret.size() + replacement.size() - 2);
                                for (; j < replacement.size() - 1; ++j)
                                    ret << replacement.at(j);
                            }
                            pending = replacement.at(j);
                        }
                    }
                    replaced = true;
                }
                continue;
            }
        } else if (unicode == BACKSLASH) {
            static const char symbols[] = "[]{}()$\\'\"";
            ushort unicode2 = str_data[i+1];
            if (!(unicode2 & 0xff00) && strchr(symbols, unicode2)) {
                unicode = unicode2;
                ++i;
            }
        } else if (quote) {
            if (unicode == quote) {
                quote = 0;
                continue;
            }
        } else {
            if (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE) {
                quote = unicode;
                continue;
            } else if (unicode == SPACE || unicode == TAB) {
                if (!joined)
                    flushCurrent(&ret, &current, &ptr, &pending, false);
                else if ((ptr - (QChar*)current.constData()) || !pending.isEmpty())
                    putSpace = true;
                continue;
            } else if (pos) {
                if (unicode == LPAREN) {
                    ++parens;
                } else if (unicode == RPAREN) {
                    --parens;
                } else if (!parens && unicode == COMMA) {
                    if (!joined) {
                        *pos = i + 1;
                        flushFinal(&ret, current, ptr, pending, str, replaced, false);
                        return ret;
                    }
                    flushCurrent(&ret, &current, &ptr, &pending, true);
                    putSpace = false;
                    continue;
                }
            }
        }
        if (putSpace) {
            putSpace = false;
            appendChar(' ', &current, &ptr, &pending);
        }
        appendChar(unicode, &current, &ptr, &pending);
    }
    if (pos)
        *pos = str_len;
    flushFinal(&ret, current, ptr, pending, str, replaced, joined);
    return ret;
}

bool ProFileEvaluator::Private::modesForGenerator(const QString &gen,
        QMakeGlobals::HOST_MODE *host_mode, QMakeGlobals::TARG_MODE *target_mode) const
{
    if (gen == fL1S("UNIX")) {
#ifdef Q_OS_MAC
        *host_mode = QMakeGlobals::HOST_MACX_MODE;
        *target_mode = QMakeGlobals::TARG_MACX_MODE;
#else
        *host_mode = QMakeGlobals::HOST_UNIX_MODE;
        *target_mode = QMakeGlobals::TARG_UNIX_MODE;
#endif
    } else if (gen == fL1S("MSVC.NET") || gen == fL1S("BMAKE") || gen == fL1S("MSBUILD")) {
        *host_mode = QMakeGlobals::HOST_WIN_MODE;
        *target_mode = QMakeGlobals::TARG_WIN_MODE;
    } else if (gen == fL1S("MINGW")) {
#if defined(Q_OS_MAC)
        *host_mode = QMakeGlobals::HOST_MACX_MODE;
#elif defined(Q_OS_UNIX)
        *host_mode = QMakeGlobals::HOST_UNIX_MODE;
#else
        *host_mode = QMakeGlobals::HOST_WIN_MODE;
#endif
        *target_mode = QMakeGlobals::TARG_WIN_MODE;
    } else if (gen == fL1S("PROJECTBUILDER") || gen == fL1S("XCODE")) {
        *host_mode = QMakeGlobals::HOST_MACX_MODE;
        *target_mode = QMakeGlobals::TARG_MACX_MODE;
    } else if (gen == fL1S("SYMBIAN_ABLD") || gen == fL1S("SYMBIAN_SBSV2")
               || gen == fL1S("SYMBIAN_UNIX") || gen == fL1S("SYMBIAN_MINGW")) {
#if defined(Q_OS_MAC)
        *host_mode = QMakeGlobals::HOST_MACX_MODE;
#elif defined(Q_OS_UNIX)
        *host_mode = QMakeGlobals::HOST_UNIX_MODE;
#else
        *host_mode = QMakeGlobals::HOST_WIN_MODE;
#endif
        *target_mode = QMakeGlobals::TARG_SYMBIAN_MODE;
    } else {
        evalError(fL1S("Unknown generator specified: %1").arg(gen));
        return false;
    }
    return true;
}

void ProFileEvaluator::Private::validateModes() const
{
    if (m_option->host_mode == QMakeGlobals::HOST_UNKNOWN_MODE
        || m_option->target_mode == QMakeGlobals::TARG_UNKNOWN_MODE) {
        const QHash<ProString, ProStringList> &vals =
                m_option->base_valuemap.isEmpty() ? m_valuemapStack[0] : m_option->base_valuemap;
        QMakeGlobals::HOST_MODE host_mode;
        QMakeGlobals::TARG_MODE target_mode;
        const ProStringList &gen = vals.value(ProString("MAKEFILE_GENERATOR"));
        if (gen.isEmpty()) {
            evalError(fL1S("Using OS scope before setting MAKEFILE_GENERATOR"));
        } else if (modesForGenerator(gen.at(0).toQString(), &host_mode, &target_mode)) {
            if (m_option->host_mode == QMakeGlobals::HOST_UNKNOWN_MODE) {
                m_option->host_mode = host_mode;
                m_option->applyHostMode();
            }

            if (m_option->target_mode == QMakeGlobals::TARG_UNKNOWN_MODE) {
                const ProStringList &tgt = vals.value(ProString("TARGET_PLATFORM"));
                if (!tgt.isEmpty()) {
                    const QString &os = tgt.at(0).toQString();
                    if (os == statics.strunix)
                        m_option->target_mode = QMakeGlobals::TARG_UNIX_MODE;
                    else if (os == statics.strmacx)
                        m_option->target_mode = QMakeGlobals::TARG_MACX_MODE;
                    else if (os == statics.strsymbian)
                        m_option->target_mode = QMakeGlobals::TARG_SYMBIAN_MODE;
                    else if (os == statics.strwin32)
                        m_option->target_mode = QMakeGlobals::TARG_WIN_MODE;
                    else
                        evalError(fL1S("Unknown target platform specified: %1").arg(os));
                } else {
                    m_option->target_mode = target_mode;
                }
            }
        }
    }
}

bool ProFileEvaluator::Private::isActiveConfig(const QString &config, bool regex)
{
    // magic types for easy flipping
    if (config == statics.strtrue)
        return true;
    if (config == statics.strfalse)
        return false;

    if (config == statics.strunix) {
        validateModes();
        return m_option->target_mode == QMakeGlobals::TARG_UNIX_MODE
               || m_option->target_mode == QMakeGlobals::TARG_MACX_MODE
               || m_option->target_mode == QMakeGlobals::TARG_SYMBIAN_MODE;
    } else if (config == statics.strmacx || config == statics.strmac) {
        validateModes();
        return m_option->target_mode == QMakeGlobals::TARG_MACX_MODE;
    } else if (config == statics.strsymbian) {
        validateModes();
        return m_option->target_mode == QMakeGlobals::TARG_SYMBIAN_MODE;
    } else if (config == statics.strwin32) {
        validateModes();
        return m_option->target_mode == QMakeGlobals::TARG_WIN_MODE;
    }

    if (regex && (config.contains(QLatin1Char('*')) || config.contains(QLatin1Char('?')))) {
        QString cfg = config;
        cfg.detach(); // Keep m_tmp out of QRegExp's cache
        QRegExp re(cfg, Qt::CaseSensitive, QRegExp::Wildcard);

        // mkspecs
        if (re.exactMatch(m_option->qmakespec_name))
            return true;

        // CONFIG variable
        int t = 0;
        foreach (const ProString &configValue, valuesDirect(statics.strCONFIG)) {
            if (re.exactMatch(configValue.toQString(m_tmp[t])))
                return true;
            t ^= 1;
        }
    } else {
        // mkspecs
        if (m_option->qmakespec_name == config)
            return true;

        // CONFIG variable
        if (valuesDirect(statics.strCONFIG).contains(ProString(config, NoHash)))
            return true;
    }

    return false;
}

ProStringList ProFileEvaluator::Private::expandVariableReferences(
        const ushort *&tokPtr, int sizeHint, bool joined)
{
    ProStringList ret;
    ret.reserve(sizeHint);
    forever {
        evaluateExpression(tokPtr, &ret, joined);
        switch (*tokPtr) {
        case TokValueTerminator:
        case TokFuncTerminator:
            tokPtr++;
            return ret;
        case TokArgSeparator:
            if (joined) {
                tokPtr++;
                continue;
            }
            // fallthrough
        default:
            Q_ASSERT_X(false, "expandVariableReferences", "Unrecognized token");
            break;
        }
    }
}

void ProFileEvaluator::Private::populateDeps(
        const ProStringList &deps, const ProString &prefix,
        QHash<ProString, QSet<ProString> > &dependencies, QHash<ProString, ProStringList> &dependees,
        ProStringList &rootSet) const
{
    foreach (const ProString &item, deps)
        if (!dependencies.contains(item)) {
            QSet<ProString> &dset = dependencies[item]; // Always create entry
            ProStringList depends = valuesDirect(ProString(prefix + item + QString::fromLatin1(".depends")));
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

QList<ProStringList> ProFileEvaluator::Private::prepareFunctionArgs(const ushort *&tokPtr)
{
    QList<ProStringList> args_list;
    if (*tokPtr != TokFuncTerminator) {
        for (;; tokPtr++) {
            ProStringList arg;
            evaluateExpression(tokPtr, &arg, false);
            args_list << arg;
            if (*tokPtr == TokFuncTerminator)
                break;
            Q_ASSERT(*tokPtr == TokArgSeparator);
        }
    }
    tokPtr++;
    return args_list;
}

QList<ProStringList> ProFileEvaluator::Private::prepareFunctionArgs(const ProString &arguments)
{
    QList<ProStringList> args_list;
    for (int pos = 0; pos < arguments.size(); )
        args_list << expandVariableReferences(arguments, &pos);
    return args_list;
}

ProStringList ProFileEvaluator::Private::evaluateFunction(
        const ProFunctionDef &func, const QList<ProStringList> &argumentsList, bool *ok)
{
    bool oki;
    ProStringList ret;

    if (m_valuemapStack.count() >= 100) {
        evalError(fL1S("ran into infinite recursion (depth > 100)."));
        oki = false;
    } else {
        m_valuemapStack.push(QHash<ProString, ProStringList>());
        m_locationStack.push(m_current);
        int loopLevel = m_loopLevel;
        m_loopLevel = 0;

        ProStringList args;
        for (int i = 0; i < argumentsList.count(); ++i) {
            args += argumentsList[i];
            m_valuemapStack.top()[ProString(QString::number(i+1))] = argumentsList[i];
        }
        m_valuemapStack.top()[statics.strARGS] = args;
        oki = (visitProBlock(func.pro(), func.tokPtr()) != ReturnFalse); // True || Return
        ret = m_returnValue;
        m_returnValue.clear();

        m_loopLevel = loopLevel;
        m_current = m_locationStack.pop();
        m_valuemapStack.pop();
    }
    if (ok)
        *ok = oki;
    if (oki)
        return ret;
    return ProStringList();
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateBoolFunction(
        const ProFunctionDef &func, const QList<ProStringList> &argumentsList,
        const ProString &function)
{
    bool ok;
    ProStringList ret = evaluateFunction(func, argumentsList, &ok);
    if (ok) {
        if (ret.isEmpty())
            return ReturnTrue;
        if (ret.at(0) != statics.strfalse) {
            if (ret.at(0) == statics.strtrue)
                return ReturnTrue;
            int val = ret.at(0).toQString(m_tmp1).toInt(&ok);
            if (ok) {
                if (val)
                    return ReturnTrue;
            } else {
                evalError(fL1S("Unexpected return value from test '%1': %2")
                          .arg(function.toQString(m_tmp1))
                          .arg(ret.join(QLatin1String(" :: "))));
            }
        }
    }
    return ReturnFalse;
}

ProStringList ProFileEvaluator::Private::evaluateExpandFunction(
        const ProString &func, const ushort *&tokPtr)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd())
        return evaluateFunction(*it, prepareFunctionArgs(tokPtr), 0);

    //why don't the builtin functions just use args_list? --Sam
    return evaluateExpandFunction(func, expandVariableReferences(tokPtr, 5, true));
}

ProStringList ProFileEvaluator::Private::evaluateExpandFunction(
        const ProString &func, const ProString &arguments)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd())
        return evaluateFunction(*it, prepareFunctionArgs(arguments), 0);

    //why don't the builtin functions just use args_list? --Sam
    int pos = 0;
    return evaluateExpandFunction(func, expandVariableReferences(arguments, &pos, true));
}

ProStringList ProFileEvaluator::Private::evaluateExpandFunction(
        const ProString &func, const ProStringList &args)
{
    ExpandFunc func_t = ExpandFunc(statics.expands.value(func));
    if (func_t == 0) {
        const QString &fn = func.toQString(m_tmp1);
        const QString &lfn = fn.toLower();
        if (!fn.isSharedWith(lfn))
            func_t = ExpandFunc(statics.expands.value(ProString(lfn)));
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
            if(args.count() < 1) {
                evalError(fL1S("sprintf(format, ...) requires at least one argument"));
            } else {
                QString tmp = args.at(0).toQString(m_tmp1);
                for (int i = 1; i < args.count(); ++i)
                    tmp = tmp.arg(args.at(i).toQString(m_tmp2));
                // Note: this depends on split_value_list() making a deep copy
                ret = split_value_list(tmp);
            }
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
            if (args.count() != 2) {
                evalError(fL1S("split(var, sep) requires one or two arguments"));
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
                            evalError(fL1S("member() argument 3 (end) '%2' invalid.\n")
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

                QFile qfile(resolvePath(expandEnvVars(file)));
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
                QHash<ProString, ProStringList> vars;
                QString fn = resolvePath(expandEnvVars(args.at(0).toQString(m_tmp1)));
                fn.detach();
                if (evaluateFileInto(fn, ProFileEvaluatorHandler::EvalAuxFile,
                                     &vars, &m_functionDefs, EvalWithDefaults))
                    ret = vars.value(map(args.at(1)));
            }
            break;
        case E_EVAL:
            if (args.count() != 1) {
                evalError(fL1S("eval(variable) requires one argument"));
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
            if(args.count() != 1) {
                evalError(fL1S("unique(var) requires one argument."));
            } else {
                ret = values(map(args.at(0)));
                ret.removeDuplicates();
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
                evalError(fL1S("files(pattern, recursive=false) requires one or two arguments"));
            } else {
                bool recursive = false;
                if (args.count() == 2)
                    recursive = isTrue(args.at(1), m_tmp2);
                QStringList dirs;
                QString r = fixPathToLocalOS(args.at(0).toQString(m_tmp1));
                QString pfx;
                if (IoUtils::isRelativePath(r)) {
                    pfx = currentDirectory();
                    if (!pfx.endsWith(QLatin1Char('/')))
                        pfx += QLatin1Char('/');
                }
                int slash = r.lastIndexOf(QDir::separator());
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
                                dirs.append(fname + QDir::separator());
                        }
                        if (regex.exactMatch(qdir[i]))
                            ret += ProString(fname, NoHash).setSource(currentProFile());
                    }
                }
            }
            break;
        case E_REPLACE:
            if(args.count() != 3 ) {
                evalError(fL1S("replace(var, before, after) requires three arguments"));
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
                evalError(fL1S("%1(var, prefix) requires one or two arguments").arg(func.toQString(m_tmp1)));
            } else {
                QHash<ProString, QSet<ProString> > dependencies;
                QHash<ProString, ProStringList> dependees;
                ProStringList rootSet;
                ProStringList orgList = valuesDirect(args.at(0));
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
        case E_INVALID:
            evalError(fL1S("'%1' is not a recognized replace function")
                      .arg(func.toQString(m_tmp1)));
            break;
        default:
            evalError(fL1S("Function '%1' is not implemented").arg(func.toQString(m_tmp1)));
            break;
    }

    return ret;
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(
        const ProString &function, const ProString &arguments)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(function);
    if (it != m_functionDefs.testFunctions.constEnd())
        return evaluateBoolFunction(*it, prepareFunctionArgs(arguments), function);

    //why don't the builtin functions just use args_list? --Sam
    int pos = 0;
    return evaluateConditionalFunction(function, expandVariableReferences(arguments, &pos, true));
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(
        const ProString &function, const ushort *&tokPtr)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(function);
    if (it != m_functionDefs.testFunctions.constEnd())
        return evaluateBoolFunction(*it, prepareFunctionArgs(tokPtr), function);

    //why don't the builtin functions just use args_list? --Sam
    return evaluateConditionalFunction(function, expandVariableReferences(tokPtr, 5, true));
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(
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
                evalError(fL1S("defined(function, type): unexpected type [%1].\n")
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
                QHash<ProString, ProStringList>::Iterator it = m_valuemapStack[i].find(var);
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
                QHash<ProString, ProStringList> vars;
                QString fn = resolvePath(expandEnvVars(args.at(0).toQString(m_tmp1)));
                fn.detach();
                if (!evaluateFileInto(fn, ProFileEvaluatorHandler::EvalAuxFile,
                                      &vars, &m_functionDefs, EvalWithDefaults))
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
            evalError(fL1S("unexpected break()."));
            return ReturnFalse;
        case T_NEXT:
            if (m_skipLevel)
                return ReturnFalse;
            if (m_loopLevel)
                return ReturnNext;
            evalError(fL1S("unexpected next()."));
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
            const ProStringList &configs = valuesDirect(statics.strCONFIG);

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
                    evalError(fL1S("unexpected modifier to count(%2)").arg(comp.toQString(m_tmp1)));
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
            QHash<ProString, ProStringList> *hsh;
            QHash<ProString, ProStringList>::Iterator it;
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
            QHash<ProString, ProStringList> *hsh;
            QHash<ProString, ProStringList>::Iterator it;
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
            QString parseInto;
            // the third optional argument to include() controls warnings
            //      and is not used here
            if ((args.count() == 2) || (args.count() == 3) ) {
                parseInto = args.at(1).toQString(m_tmp2);
            } else if (args.count() != 1) {
                evalError(fL1S("include(file, into, silent) requires one, two or three arguments."));
                return ReturnFalse;
            }
            QString fn = resolvePath(expandEnvVars(args.at(0).toQString(m_tmp1)));
            fn.detach();
            bool ok;
            if (parseInto.isEmpty()) {
                ok = evaluateFile(fn, ProFileEvaluatorHandler::EvalIncludeFile,
                                  ProFileEvaluator::LoadProOnly);
            } else {
                QHash<ProString, ProStringList> symbols;
                if ((ok = evaluateFileInto(fn, ProFileEvaluatorHandler::EvalAuxFile,
                                           &symbols, 0, EvalWithSetup))) {
                    QHash<ProString, ProStringList> newMap;
                    for (QHash<ProString, ProStringList>::ConstIterator
                            it = m_valuemapStack.top().constBegin(),
                            end = m_valuemapStack.top().constEnd();
                            it != end; ++it) {
                        const QString &ky = it.key().toQString(m_tmp1);
                        if (!(ky.startsWith(parseInto) &&
                              (ky.length() == parseInto.length()
                               || ky.at(parseInto.length()) == QLatin1Char('.'))))
                            newMap[it.key()] = it.value();
                    }
                    for (QHash<ProString, ProStringList>::ConstIterator it = symbols.constBegin();
                         it != symbols.constEnd(); ++it) {
                        const QString &ky = it.key().toQString(m_tmp1);
                        if (!ky.startsWith(QLatin1Char('.')))
                            newMap.insert(ProString(parseInto + QLatin1Char('.') + ky), it.value());
                    }
                    m_valuemapStack.top() = newMap;
                }
            }
            return returnBool(ok);
        }
        case T_LOAD: {
            if (m_skipLevel && !m_cumulative)
                return ReturnFalse;
            // bool ignore_error = false;
            if (args.count() == 2) {
                // ignore_error = isTrue(args.at(1), m_tmp2);
            } else if (args.count() != 1) {
                evalError(fL1S("load(feature) requires one or two arguments."));
                return ReturnFalse;
            }
            // XXX ignore_error unused
            return returnBool(evaluateFeatureFile(expandEnvVars(args.at(0).toQString())));
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
            const QString &msg = expandEnvVars(args.at(0).toQString(m_tmp2));
            if (!m_skipLevel)
                m_handler->fileMessage(fL1S("Project %1: %2")
                                       .arg(function.toQString(m_tmp1).toUpper(), msg));
            // ### Consider real termination in non-cumulative mode
            return returnBool(function != QLatin1String("error"));
        }
#if 0 // Way too dangerous to enable.
        case T_SYSTEM: {
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
            const QString &file = resolvePath(expandEnvVars(args.at(0).toQString(m_tmp1)));

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
            evalError(fL1S("'%1' is not a recognized test function")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        default:
            evalError(fL1S("Function '%1' is not implemented").arg(function.toQString(m_tmp1)));
            return ReturnFalse;
    }
}

QHash<ProString, ProStringList> *ProFileEvaluator::Private::findValues(
        const ProString &variableName, QHash<ProString, ProStringList>::Iterator *rit)
{
    for (int i = m_valuemapStack.size(); --i >= 0; ) {
        QHash<ProString, ProStringList>::Iterator it = m_valuemapStack[i].find(variableName);
        if (it != m_valuemapStack[i].end()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                return 0;
            *rit = it;
            return &m_valuemapStack[i];
        }
    }
    return 0;
}

ProStringList &ProFileEvaluator::Private::valuesRef(const ProString &variableName)
{
    QHash<ProString, ProStringList>::Iterator it = m_valuemapStack.top().find(variableName);
    if (it != m_valuemapStack.top().end()) {
        if (it->constBegin() == statics.fakeValue.constBegin())
            it->clear();
        return *it;
    }
    for (int i = m_valuemapStack.size() - 1; --i >= 0; ) {
        QHash<ProString, ProStringList>::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            ProStringList &ret = m_valuemapStack.top()[variableName];
            if (it->constBegin() != statics.fakeValue.constBegin())
                ret = *it;
            return ret;
        }
    }
    return m_valuemapStack.top()[variableName];
}

ProStringList ProFileEvaluator::Private::valuesDirect(const ProString &variableName) const
{
    for (int i = m_valuemapStack.size(); --i >= 0; ) {
        QHash<ProString, ProStringList>::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                break;
            return *it;
        }
    }
    return ProStringList();
}

ProStringList ProFileEvaluator::Private::values(const ProString &variableName) const
{
    QHash<ProString, int>::ConstIterator vli = statics.varList.find(variableName);
    if (vli != statics.varList.constEnd()) {
        int vlidx = *vli;
        QString ret;
        switch ((VarName)vlidx) {
        case V_LITERAL_WHITESPACE: ret = QLatin1String("\t"); break;
        case V_LITERAL_DOLLAR: ret = QLatin1String("$"); break;
        case V_LITERAL_HASH: ret = QLatin1String("#"); break;
        case V_OUT_PWD: // the outgoing dir (shadow of _PRO_FILE_PWD_)
            ret = m_outputDir;
            break;
        case V_PWD: // containing directory of most nested project/include file
        case V_IN_PWD:
            ret = currentDirectory();
            break;
        case V_DIR_SEPARATOR:
            validateModes();
            ret = m_option->dir_sep;
            break;
        case V_DIRLIST_SEPARATOR:
            ret = m_option->dirlist_sep;
            break;
        case V__LINE_: // currently executed line number
            ret = QString::number(m_current.line);
            break;
        case V__FILE_: // currently executed file
            ret = m_current.pro->fileName();
            break;
        case V__DATE_: //current date/time
            ret = QDateTime::currentDateTime().toString();
            break;
        case V__PRO_FILE_:
            ret = m_profileStack.first()->fileName();
            break;
        case V__PRO_FILE_PWD_:
            ret = m_profileStack.first()->directoryName();
            break;
        case V__QMAKE_CACHE_:
            ret = m_option->cachefile;
            break;
#if defined(Q_OS_WIN32)
        case V_QMAKE_HOST_os: ret = QLatin1String("Windows"); break;
        case V_QMAKE_HOST_name: {
            DWORD name_length = 1024;
            TCHAR name[1024];
            if (GetComputerName(name, &name_length))
                ret = QString::fromUtf16((ushort*)name, name_length);
            break;
        }
        case V_QMAKE_HOST_version:
            ret = QString::number(QSysInfo::WindowsVersion);
            break;
        case V_QMAKE_HOST_version_string:
            switch (QSysInfo::WindowsVersion) {
            case QSysInfo::WV_Me: ret = QLatin1String("WinMe"); break;
            case QSysInfo::WV_95: ret = QLatin1String("Win95"); break;
            case QSysInfo::WV_98: ret = QLatin1String("Win98"); break;
            case QSysInfo::WV_NT: ret = QLatin1String("WinNT"); break;
            case QSysInfo::WV_2000: ret = QLatin1String("Win2000"); break;
            case QSysInfo::WV_2003: ret = QLatin1String("Win2003"); break;
            case QSysInfo::WV_XP: ret = QLatin1String("WinXP"); break;
            case QSysInfo::WV_VISTA: ret = QLatin1String("WinVista"); break;
            default: ret = QLatin1String("Unknown"); break;
            }
            break;
        case V_QMAKE_HOST_arch:
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            switch(info.wProcessorArchitecture) {
#ifdef PROCESSOR_ARCHITECTURE_AMD64
            case PROCESSOR_ARCHITECTURE_AMD64:
                ret = QLatin1String("x86_64");
                break;
#endif
            case PROCESSOR_ARCHITECTURE_INTEL:
                ret = QLatin1String("x86");
                break;
            case PROCESSOR_ARCHITECTURE_IA64:
#ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
            case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
#endif
                ret = QLatin1String("IA64");
                break;
            default:
                ret = QLatin1String("Unknown");
                break;
            }
            break;
#elif defined(Q_OS_UNIX)
        case V_QMAKE_HOST_os:
        case V_QMAKE_HOST_name:
        case V_QMAKE_HOST_version:
        case V_QMAKE_HOST_version_string:
        case V_QMAKE_HOST_arch:
            {
                struct utsname name;
                const char *what;
                if (!uname(&name)) {
                    switch (vlidx) {
                    case V_QMAKE_HOST_os: what = name.sysname; break;
                    case V_QMAKE_HOST_name: what = name.nodename; break;
                    case V_QMAKE_HOST_version: what = name.release; break;
                    case V_QMAKE_HOST_version_string: what = name.version; break;
                    case V_QMAKE_HOST_arch: what = name.machine; break;
                    }
                    ret = QString::fromLocal8Bit(what);
                }
            }
#endif
        }
        return ProStringList(ProString(ret, NoHash));
    }

    ProStringList result = valuesDirect(variableName);
    if (result.isEmpty()) {
        if (variableName == statics.strTEMPLATE) {
            result.append(ProString("app", NoHash));
        } else if (variableName == statics.strQMAKE_DIR_SEP) {
            result.append(ProString(m_option->dirlist_sep, NoHash));
        }
    }
    return result;
}

bool ProFileEvaluator::Private::evaluateFileDirect(
        const QString &fileName, ProFileEvaluatorHandler::EvalFileType type,
        ProFileEvaluator::LoadFlags flags)
{
    if (ProFile *pro = m_parser->parsedProFile(fileName, true)) {
        m_locationStack.push(m_current);
        bool ok = (visitProFile(pro, type, flags) == ReturnTrue);
        m_current = m_locationStack.pop();
        pro->deref();
        return ok;
    } else {
        return false;
    }
}

bool ProFileEvaluator::Private::evaluateFile(
        const QString &fileName, ProFileEvaluatorHandler::EvalFileType type,
        ProFileEvaluator::LoadFlags flags)
{
    if (fileName.isEmpty())
        return false;
    foreach (const ProFile *pf, m_profileStack)
        if (pf->fileName() == fileName) {
            evalError(fL1S("circular inclusion of %1").arg(fileName));
            return false;
        }
    return evaluateFileDirect(fileName, type, flags);
}

bool ProFileEvaluator::Private::evaluateFeatureFile(const QString &fileName)
{
    QString fn = fileName;
    if (!fn.endsWith(QLatin1String(".prf")))
        fn += QLatin1String(".prf");

    if ((!fileName.contains((ushort)'/') && !fileName.contains((ushort)'\\'))
        || !IoUtils::exists(resolvePath(fn))) {
        if (m_option->feature_roots.isEmpty())
            m_option->feature_roots = qmakeFeaturePaths();
        int start_root = 0;
        QString currFn = currentFileName();
        if (IoUtils::fileName(currFn) == IoUtils::fileName(fn)) {
            for (int root = 0; root < m_option->feature_roots.size(); ++root)
                if (currFn == m_option->feature_roots.at(root) + fn) {
                    start_root = root + 1;
                    break;
                }
        }
        for (int root = start_root; root < m_option->feature_roots.size(); ++root) {
            QString fname = m_option->feature_roots.at(root) + fn;
            if (IoUtils::exists(fname)) {
                fn = fname;
                goto cool;
            }
        }
        return false;

      cool:
        // It's beyond me why qmake has this inside this if ...
        ProStringList &already = valuesRef(ProString("QMAKE_INTERNAL_INCLUDED_FEATURES"));
        ProString afn(fn, NoHash);
        if (already.contains(afn))
            return true;
        already.append(afn);
    } else {
        fn = resolvePath(fn);
    }

#ifdef PROEVALUATOR_CUMULATIVE
    bool cumulative = m_cumulative;
    m_cumulative = false;
#endif

    // The path is fully normalized already.
    bool ok = evaluateFileDirect(fn, ProFileEvaluatorHandler::EvalFeatureFile,
                                 ProFileEvaluator::LoadProOnly);

#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = cumulative;
#endif
    return ok;
}

bool ProFileEvaluator::Private::evaluateFileInto(
        const QString &fileName, ProFileEvaluatorHandler::EvalFileType type,
        QHash<ProString, ProStringList> *values, ProFunctionDefs *funcs, EvalIntoMode mode)
{
    ProFileEvaluator visitor(m_option, m_parser, m_handler);
#ifdef PROEVALUATOR_CUMULATIVE
    visitor.d->m_cumulative = false;
#endif
    visitor.d->m_outputDir = m_outputDir;
//    visitor.d->m_valuemapStack.top() = *values;
    if (funcs)
        visitor.d->m_functionDefs = *funcs;
    if (mode == EvalWithDefaults)
        visitor.d->evaluateFeatureFile(QLatin1String("default_pre.prf"));
    if (!visitor.d->evaluateFile(fileName, type,
            (mode == EvalWithSetup) ? ProFileEvaluator::LoadAll : ProFileEvaluator::LoadProOnly))
        return false;
    *values = visitor.d->m_valuemapStack.top();
//    if (funcs)
//        *funcs = visitor.d->m_functionDefs;
    return true;
}

void ProFileEvaluator::Private::evalError(const QString &message) const
{
    if (!m_skipLevel)
        m_handler->evalError(m_current.line ? m_current.pro->fileName() : QString(),
                             m_current.line, message);
}


///////////////////////////////////////////////////////////////////////
//
// ProFileEvaluator
//
///////////////////////////////////////////////////////////////////////

void ProFileEvaluator::initialize()
{
    Private::initStatics();
}

ProFileEvaluator::ProFileEvaluator(QMakeGlobals *option, QMakeParser *parser,
                                   ProFileEvaluatorHandler *handler)
  : d(new Private(option, parser, handler))
{
}

ProFileEvaluator::~ProFileEvaluator()
{
    delete d;
}

bool ProFileEvaluator::contains(const QString &variableName) const
{
    return d->m_valuemapStack.top().contains(ProString(variableName));
}

QString ProFileEvaluator::value(const QString &variable) const
{
    const QStringList &vals = values(variable);
    if (!vals.isEmpty())
        return vals.first();

    return QString();
}

QStringList ProFileEvaluator::values(const QString &variableName) const
{
    const ProStringList &values = d->values(ProString(variableName));
    QStringList ret;
    ret.reserve(values.size());
    foreach (const ProString &str, values)
        ret << d->expandEnvVars(str.toQString());
    return ret;
}

QStringList ProFileEvaluator::values(const QString &variableName, const ProFile *pro) const
{
    // It makes no sense to put any kind of magic into expanding these
    const ProStringList &values = d->m_valuemapStack.at(0).value(ProString(variableName));
    QStringList ret;
    ret.reserve(values.size());
    foreach (const ProString &str, values)
        if (str.sourceFile() == pro)
            ret << d->expandEnvVars(str.toQString());
    return ret;
}

QString ProFileEvaluator::sysrootify(const QString &path, const QString &baseDir) const
{
#ifdef Q_OS_WIN
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    const bool isHostSystemPath =
        d->m_option->sysroot.isEmpty() || path.startsWith(d->m_option->sysroot, cs)
        || path.startsWith(baseDir, cs) || path.startsWith(d->m_outputDir, cs);

    return isHostSystemPath ? path : d->m_option->sysroot + path;
}

QStringList ProFileEvaluator::absolutePathValues(
        const QString &variable, const QString &baseDirectory) const
{
    QStringList result;
    foreach (const QString &el, values(variable)) {
        QString absEl = IoUtils::isAbsolutePath(el)
            ? sysrootify(el, baseDirectory) : IoUtils::resolvePath(baseDirectory, el);
        if (IoUtils::fileType(absEl) == IoUtils::FileIsDir)
            result << QDir::cleanPath(absEl);
    }
    return result;
}

QStringList ProFileEvaluator::absoluteFileValues(
        const QString &variable, const QString &baseDirectory, const QStringList &searchDirs,
        const ProFile *pro) const
{
    QStringList result;
    foreach (const QString &el, pro ? values(variable, pro) : values(variable)) {
        QString absEl;
        if (IoUtils::isAbsolutePath(el)) {
            const QString elWithSysroot = sysrootify(el, baseDirectory);
            if (IoUtils::exists(elWithSysroot)) {
                result << QDir::cleanPath(elWithSysroot);
                goto next;
            }
            absEl = elWithSysroot;
        } else {
            foreach (const QString &dir, searchDirs) {
                QString fn = dir + QLatin1Char('/') + el;
                if (IoUtils::exists(fn)) {
                    result << QDir::cleanPath(fn);
                    goto next;
                }
            }
            if (baseDirectory.isEmpty())
                goto next;
            absEl = baseDirectory + QLatin1Char('/') + el;
        }
        {
            absEl = QDir::cleanPath(absEl);
            int nameOff = absEl.lastIndexOf(QLatin1Char('/'));
            QString absDir = d->m_tmp1.setRawData(absEl.constData(), nameOff);
            if (IoUtils::exists(absDir)) {
                QString wildcard = d->m_tmp2.setRawData(absEl.constData() + nameOff + 1,
                                                        absEl.length() - nameOff - 1);
                if (wildcard.contains(QLatin1Char('*')) || wildcard.contains(QLatin1Char('?'))) {
                    wildcard.detach(); // Keep m_tmp out of QRegExp's cache
                    QDir theDir(absDir);
                    foreach (const QString &fn, theDir.entryList(QStringList(wildcard)))
                        if (fn != statics.strDot && fn != statics.strDotDot)
                            result << absDir + QLatin1Char('/') + fn;
                } // else if (acceptMissing)
            }
        }
      next: ;
    }
    return result;
}

ProFileEvaluator::TemplateType ProFileEvaluator::templateType() const
{
    const ProStringList &templ = d->values(statics.strTEMPLATE);
    if (templ.count() >= 1) {
        const QString &t = templ.at(0).toQString();
        if (!t.compare(QLatin1String("app"), Qt::CaseInsensitive))
            return TT_Application;
        if (!t.compare(QLatin1String("lib"), Qt::CaseInsensitive))
            return TT_Library;
        if (!t.compare(QLatin1String("script"), Qt::CaseInsensitive))
            return TT_Script;
        if (!t.compare(QLatin1String("aux"), Qt::CaseInsensitive))
            return TT_Aux;
        if (!t.compare(QLatin1String("subdirs"), Qt::CaseInsensitive))
            return TT_Subdirs;
    }
    return TT_Unknown;
}

bool ProFileEvaluator::accept(ProFile *pro, LoadFlags flags)
{
    return d->visitProFile(pro, ProFileEvaluatorHandler::EvalProjectFile, flags) == Private::ReturnTrue;
}

QString ProFileEvaluator::propertyValue(const QString &name) const
{
    return d->propertyValue(name, false);
}

#ifdef PROEVALUATOR_CUMULATIVE
void ProFileEvaluator::setCumulative(bool on)
{
    d->m_cumulative = on;
}
#endif

void ProFileEvaluator::setOutputDir(const QString &dir)
{
    d->m_outputDir = dir;
}

QT_END_NAMESPACE
