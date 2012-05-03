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

#include "qmakeevaluator.h"

#include "profileevaluator.h"
#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "qmakeevaluator_p.h"
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

using namespace ProFileEvaluatorInternal;

QT_BEGIN_NAMESPACE

using namespace ProStringConstants;


#define fL1S(s) QString::fromLatin1(s)

namespace ProFileEvaluatorInternal {
QMakeStatics statics;
}

void QMakeEvaluator::initStatics()
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

    initFunctionStatics();

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

const ProString &QMakeEvaluator::map(const ProString &var)
{
    QHash<ProString, ProString>::ConstIterator it = statics.varMap.constFind(var);
    return (it != statics.varMap.constEnd()) ? it.value() : var;
}


QMakeEvaluator::QMakeEvaluator(QMakeGlobals *option,
                               QMakeParser *parser, QMakeEvaluatorHandler *handler)
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

QMakeEvaluator::~QMakeEvaluator()
{
}

//////// Evaluator tools /////////

uint QMakeEvaluator::getBlockLen(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    len |= (uint)*tokPtr++ << 16;
    return len;
}

ProString QMakeEvaluator::getStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    ProString ret(m_current.pro->items(), tokPtr - m_current.pro->tokPtr(), len, NoHash);
    ret.setSource(m_current.pro);
    tokPtr += len;
    return ret;
}

ProString QMakeEvaluator::getHashStr(const ushort *&tokPtr)
{
    uint hash = getBlockLen(tokPtr);
    uint len = *tokPtr++;
    ProString ret(m_current.pro->items(), tokPtr - m_current.pro->tokPtr(), len, hash);
    tokPtr += len;
    return ret;
}

void QMakeEvaluator::skipStr(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    tokPtr += len;
}

void QMakeEvaluator::skipHashStr(const ushort *&tokPtr)
{
    tokPtr += 2;
    uint len = *tokPtr++;
    tokPtr += len;
}

// FIXME: this should not build new strings for direct sections.
// Note that the E_SPRINTF and E_LIST implementations rely on the deep copy.
ProStringList QMakeEvaluator::split_value_list(const QString &vals, const ProFile *source)
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

QString QMakeEvaluator::expandEnvVars(const QString &str) const
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
QString QMakeEvaluator::fixPathToLocalOS(const QString &str) const
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

void QMakeEvaluator::evaluateExpression(
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

void QMakeEvaluator::skipExpression(const ushort *&pTokPtr)
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

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProBlock(
        ProFile *pro, const ushort *tokPtr)
{
    m_current.pro = pro;
    m_current.line = 0;
    return visitProBlock(tokPtr);
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProBlock(
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


void QMakeEvaluator::visitProFunctionDef(
        ushort tok, const ProString &name, const ushort *tokPtr)
{
    QHash<ProString, ProFunctionDef> *hash =
            (tok == TokTestDef
             ? &m_functionDefs.testFunctions
             : &m_functionDefs.replaceFunctions);
    hash->insert(name, ProFunctionDef(m_current.pro, tokPtr - m_current.pro->tokPtr()));
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProLoop(
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

void QMakeEvaluator::visitProVariable(
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

void QMakeEvaluator::visitCmdLine(const QString &cmds)
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

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProFile(
        ProFile *pro, QMakeEvaluatorHandler::EvalFileType type, LoadFlags flags)
{
    if (!m_cumulative && !pro->isOk())
        return ReturnFalse;

    m_handler->aboutToEval(currentProFile(), pro, type);
    m_profileStack.push(pro);
    if (flags & LoadPreFiles) {
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
                    if (evaluateFileInto(qmake_cache, QMakeEvaluatorHandler::EvalConfigFile,
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
                    if (!evaluateFileDirect(spec,
                                            QMakeEvaluatorHandler::EvalConfigFile, LoadProOnly)) {
                        m_handler->configError(
                                fL1S("Could not read qmake configuration file %1").arg(spec));
                    } else if (!m_option->cachefile.isEmpty()) {
                        evaluateFileDirect(m_option->cachefile,
                                           QMakeEvaluatorHandler::EvalConfigFile, LoadProOnly);
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

    if (flags & LoadPostFiles) {
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


QStringList QMakeEvaluator::qmakeMkspecPaths() const
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

QStringList QMakeEvaluator::qmakeFeaturePaths() const
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

QString QMakeEvaluator::propertyValue(const QString &name, bool complain) const
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

ProFile *QMakeEvaluator::currentProFile() const
{
    if (m_profileStack.count() > 0)
        return m_profileStack.top();
    return 0;
}

QString QMakeEvaluator::currentFileName() const
{
    ProFile *pro = currentProFile();
    if (pro)
        return pro->fileName();
    return QString();
}

QString QMakeEvaluator::currentDirectory() const
{
    ProFile *cur = m_profileStack.top();
    return cur->directoryName();
}

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

ProStringList QMakeEvaluator::expandVariableReferences(
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

bool QMakeEvaluator::modesForGenerator(const QString &gen,
                                       QMakeGlobals::TARG_MODE *target_mode) const
{
    if (gen == fL1S("UNIX")) {
#ifdef Q_OS_MAC
        *target_mode = QMakeGlobals::TARG_MACX_MODE;
#else
        *target_mode = QMakeGlobals::TARG_UNIX_MODE;
#endif
    } else if (gen == fL1S("MSVC.NET") || gen == fL1S("BMAKE") || gen == fL1S("MSBUILD")) {
        *target_mode = QMakeGlobals::TARG_WIN_MODE;
    } else if (gen == fL1S("MINGW")) {
        *target_mode = QMakeGlobals::TARG_WIN_MODE;
    } else if (gen == fL1S("PROJECTBUILDER") || gen == fL1S("XCODE")) {
        *target_mode = QMakeGlobals::TARG_MACX_MODE;
    } else if (gen == fL1S("SYMBIAN_ABLD") || gen == fL1S("SYMBIAN_SBSV2")
               || gen == fL1S("SYMBIAN_UNIX") || gen == fL1S("SYMBIAN_MINGW")) {
        *target_mode = QMakeGlobals::TARG_SYMBIAN_MODE;
    } else {
        evalError(fL1S("Unknown generator specified: %1").arg(gen));
        return false;
    }
    return true;
}

void QMakeEvaluator::validateModes() const
{
    if (m_option->target_mode == QMakeGlobals::TARG_UNKNOWN_MODE) {
        const QHash<ProString, ProStringList> &vals =
                m_option->base_valuemap.isEmpty() ? m_valuemapStack[0] : m_option->base_valuemap;
        QMakeGlobals::TARG_MODE target_mode;
        const ProStringList &gen = vals.value(ProString("MAKEFILE_GENERATOR"));
        if (gen.isEmpty()) {
            evalError(fL1S("Using OS scope before setting MAKEFILE_GENERATOR"));
        } else if (modesForGenerator(gen.at(0).toQString(), &target_mode)) {
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

bool QMakeEvaluator::isActiveConfig(const QString &config, bool regex)
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

ProStringList QMakeEvaluator::expandVariableReferences(
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

QList<ProStringList> QMakeEvaluator::prepareFunctionArgs(const ushort *&tokPtr)
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

QList<ProStringList> QMakeEvaluator::prepareFunctionArgs(const ProString &arguments)
{
    QList<ProStringList> args_list;
    for (int pos = 0; pos < arguments.size(); )
        args_list << expandVariableReferences(arguments, &pos);
    return args_list;
}

ProStringList QMakeEvaluator::evaluateFunction(
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

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateBoolFunction(
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

ProStringList QMakeEvaluator::evaluateExpandFunction(
        const ProString &func, const ushort *&tokPtr)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd())
        return evaluateFunction(*it, prepareFunctionArgs(tokPtr), 0);

    //why don't the builtin functions just use args_list? --Sam
    return evaluateExpandFunction(func, expandVariableReferences(tokPtr, 5, true));
}

ProStringList QMakeEvaluator::evaluateExpandFunction(
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

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditionalFunction(
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

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditionalFunction(
        const ProString &function, const ushort *&tokPtr)
{
    QHash<ProString, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(function);
    if (it != m_functionDefs.testFunctions.constEnd())
        return evaluateBoolFunction(*it, prepareFunctionArgs(tokPtr), function);

    //why don't the builtin functions just use args_list? --Sam
    return evaluateConditionalFunction(function, expandVariableReferences(tokPtr, 5, true));
}

QHash<ProString, ProStringList> *QMakeEvaluator::findValues(
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

ProStringList &QMakeEvaluator::valuesRef(const ProString &variableName)
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

ProStringList QMakeEvaluator::valuesDirect(const ProString &variableName) const
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

ProStringList QMakeEvaluator::values(const ProString &variableName) const
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

ProString QMakeEvaluator::first(const ProString &variableName) const
{
    const ProStringList &vals = values(variableName);
    if (!vals.isEmpty())
        return vals.first();
    return ProString();
}

bool QMakeEvaluator::evaluateFileDirect(
        const QString &fileName, QMakeEvaluatorHandler::EvalFileType type, LoadFlags flags)
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

bool QMakeEvaluator::evaluateFile(
        const QString &fileName, QMakeEvaluatorHandler::EvalFileType type, LoadFlags flags)
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

bool QMakeEvaluator::evaluateFeatureFile(const QString &fileName)
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
    bool ok = evaluateFileDirect(fn, QMakeEvaluatorHandler::EvalFeatureFile, LoadProOnly);

#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = cumulative;
#endif
    return ok;
}

bool QMakeEvaluator::evaluateFileInto(
        const QString &fileName, QMakeEvaluatorHandler::EvalFileType type,
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
            (mode == EvalWithSetup) ? LoadAll : LoadProOnly))
        return false;
    *values = visitor.d->m_valuemapStack.top();
//    if (funcs)
//        *funcs = visitor.d->m_functionDefs;
    return true;
}

void QMakeEvaluator::evalError(const QString &message) const
{
    if (!m_skipLevel)
        m_handler->evalError(m_current.line ? m_current.pro->fileName() : QString(),
                             m_current.line, message);
}

QT_END_NAMESPACE
