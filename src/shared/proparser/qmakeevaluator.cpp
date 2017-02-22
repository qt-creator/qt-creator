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

#include "qmakeevaluator.h"
#include "qmakeevaluator_p.h"

#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "qmakevfs.h"
#include "ioutils.h"

#include <qbytearray.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qregexp.h>
#include <qset.h>
#include <qstack.h>
#include <qstring.h>
#include <qstringlist.h>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qthreadpool.h>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/utsname.h>
#  ifdef Q_OS_BSD4
#    include <sys/sysctl.h>
#  endif
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

#define fL1S(s) QString::fromLatin1(s)

// we can't use QThread in qmake
// this function is a merger of QThread::idealThreadCount from qthread_win.cpp and qthread_unix.cpp
static int idealThreadCount()
{
#ifdef PROEVALUATOR_THREAD_SAFE
    return QThread::idealThreadCount();
#elif defined(Q_OS_WIN)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    // there are a couple more definitions in the Unix QThread::idealThreadCount, but
    // we don't need them all here
    int cores = 1;
#  if defined(Q_OS_BSD4)
    // FreeBSD, OpenBSD, NetBSD, BSD/OS, OS X
    size_t len = sizeof(cores);
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &cores, &len, NULL, 0) != 0) {
        perror("sysctl");
    }
#  elif defined(_SC_NPROCESSORS_ONLN)
    // the rest: Linux, Solaris, AIX, Tru64
    cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cores == -1)
        return 1;
#  endif
    return cores;
#endif
}


QMakeBaseKey::QMakeBaseKey(const QString &_root, const QString &_stash, bool _hostBuild)
    : root(_root), stash(_stash), hostBuild(_hostBuild)
{
}

uint qHash(const QMakeBaseKey &key)
{
    return qHash(key.root) ^ qHash(key.stash) ^ (uint)key.hostBuild;
}

bool operator==(const QMakeBaseKey &one, const QMakeBaseKey &two)
{
    return one.root == two.root && one.stash == two.stash && one.hostBuild == two.hostBuild;
}

QMakeBaseEnv::QMakeBaseEnv()
    : evaluator(0)
{
#ifdef PROEVALUATOR_THREAD_SAFE
    inProgress = false;
#endif
}

QMakeBaseEnv::~QMakeBaseEnv()
{
    delete evaluator;
}

namespace QMakeInternal {
QMakeStatics statics;
}

void QMakeEvaluator::initStatics()
{
    if (!statics.field_sep.isNull())
        return;

    statics.field_sep = QLatin1String(" ");
    statics.strtrue = QLatin1String("true");
    statics.strfalse = QLatin1String("false");
    statics.strCONFIG = ProKey("CONFIG");
    statics.strARGS = ProKey("ARGS");
    statics.strARGC = ProKey("ARGC");
    statics.strDot = QLatin1String(".");
    statics.strDotDot = QLatin1String("..");
    statics.strever = QLatin1String("ever");
    statics.strforever = QLatin1String("forever");
    statics.strhost_build = QLatin1String("host_build");
    statics.strTEMPLATE = ProKey("TEMPLATE");
    statics.strQMAKE_PLATFORM = ProKey("QMAKE_PLATFORM");
    statics.strQMAKESPEC = ProKey("QMAKESPEC");
#ifdef PROEVALUATOR_FULL
    statics.strREQUIRES = ProKey("REQUIRES");
#endif

    statics.fakeValue = ProStringList(ProString("_FAKE_")); // It has to have a unique begin() value

    initFunctionStatics();

    static const struct {
        const char * const oldname, * const newname;
    } mapInits[] = {
        {"INTERFACES", "FORMS"},
        {"QMAKE_POST_BUILD", "QMAKE_POST_LINK"},
        {"TARGETDEPS", "POST_TARGETDEPS"},
        {"LIBPATH", "QMAKE_LIBDIR"},
        {"QMAKE_EXT_MOC", "QMAKE_EXT_CPP_MOC"},
        {"QMAKE_MOD_MOC", "QMAKE_H_MOD_MOC"},
        {"QMAKE_LFLAGS_SHAPP", "QMAKE_LFLAGS_APP"},
        {"PRECOMPH", "PRECOMPILED_HEADER"},
        {"PRECOMPCPP", "PRECOMPILED_SOURCE"},
        {"INCPATH", "INCLUDEPATH"},
        {"QMAKE_EXTRA_WIN_COMPILERS", "QMAKE_EXTRA_COMPILERS"},
        {"QMAKE_EXTRA_UNIX_COMPILERS", "QMAKE_EXTRA_COMPILERS"},
        {"QMAKE_EXTRA_WIN_TARGETS", "QMAKE_EXTRA_TARGETS"},
        {"QMAKE_EXTRA_UNIX_TARGETS", "QMAKE_EXTRA_TARGETS"},
        {"QMAKE_EXTRA_UNIX_INCLUDES", "QMAKE_EXTRA_INCLUDES"},
        {"QMAKE_EXTRA_UNIX_VARIABLES", "QMAKE_EXTRA_VARIABLES"},
        {"QMAKE_RPATH", "QMAKE_LFLAGS_RPATH"},
        {"QMAKE_FRAMEWORKDIR", "QMAKE_FRAMEWORKPATH"},
        {"QMAKE_FRAMEWORKDIR_FLAGS", "QMAKE_FRAMEWORKPATH_FLAGS"},
        {"IN_PWD", "PWD"},
        {"DEPLOYMENT", "INSTALLS"}
    };
    statics.varMap.reserve((int)(sizeof(mapInits)/sizeof(mapInits[0])));
    for (unsigned i = 0; i < sizeof(mapInits)/sizeof(mapInits[0]); ++i)
        statics.varMap.insert(ProKey(mapInits[i].oldname), ProKey(mapInits[i].newname));
}

const ProKey &QMakeEvaluator::map(const ProKey &var)
{
    QHash<ProKey, ProKey>::ConstIterator it = statics.varMap.constFind(var);
    if (it == statics.varMap.constEnd())
        return var;
    deprecationWarning(fL1S("Variable %1 is deprecated; use %2 instead.")
                       .arg(var.toQString(), it.value().toQString()));
    return it.value();
}


QMakeEvaluator::QMakeEvaluator(QMakeGlobals *option, QMakeParser *parser, QMakeVfs *vfs,
                               QMakeHandler *handler)
  :
#ifdef PROEVALUATOR_DEBUG
    m_debugLevel(option->debugLevel),
#endif
    m_option(option), m_parser(parser), m_handler(handler), m_vfs(vfs)
{
    // So that single-threaded apps don't have to call initialize() for now.
    initStatics();

    // Configuration, more or less
    m_caller = 0;
#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = false;
#endif
    m_hostBuild = false;

    // Evaluator state
#ifdef PROEVALUATOR_CUMULATIVE
    m_skipLevel = 0;
#endif
    m_listCount = 0;
    m_valuemapStack.push(ProValueMap());
    m_valuemapInited = false;
}

QMakeEvaluator::~QMakeEvaluator()
{
}

void QMakeEvaluator::initFrom(const QMakeEvaluator *other)
{
    Q_ASSERT_X(other, "QMakeEvaluator::visitProFile", "Project not prepared");
    m_functionDefs = other->m_functionDefs;
    m_valuemapStack = other->m_valuemapStack;
    m_valuemapInited = true;
    m_qmakespec = other->m_qmakespec;
    m_qmakespecName = other->m_qmakespecName;
    m_mkspecPaths = other->m_mkspecPaths;
    m_featureRoots = other->m_featureRoots;
    m_dirSep = other->m_dirSep;
}

//////// Evaluator tools /////////

uint QMakeEvaluator::getBlockLen(const ushort *&tokPtr)
{
    uint len = *tokPtr++;
    len |= (uint)*tokPtr++ << 16;
    return len;
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
ProStringList QMakeEvaluator::split_value_list(const QStringRef &vals, const ProFile *source)
{
    QString build;
    ProStringList ret;

    if (!source)
        source = currentProFile();

    const QChar *vals_data = vals.data();
    const int vals_len = vals.length();
    ushort quote = 0;
    bool hadWord = false;
    for (int x = 0; x < vals_len; x++) {
        ushort unicode = vals_data[x].unicode();
        if (unicode == quote) {
            quote = 0;
            hadWord = true;
            build += QChar(unicode);
            continue;
        }
        switch (unicode) {
        case '"':
        case '\'':
            if (!quote)
                quote = unicode;
            hadWord = true;
            break;
        case ' ':
        case '\t':
            if (!quote) {
                if (hadWord) {
                    ret << ProString(build).setSource(source);
                    build.clear();
                    hadWord = false;
                }
                continue;
            }
            break;
        case '\\':
            if (x + 1 != vals_len) {
                ushort next = vals_data[++x].unicode();
                if (next == '\'' || next == '"' || next == '\\') {
                    build += QChar(unicode);
                    unicode = next;
                } else {
                    --x;
                }
            }
            // fallthrough
        default:
            hadWord = true;
            break;
        }
        build += QChar(unicode);
    }
    if (hadWord)
        ret << ProString(build).setSource(source);
    return ret;
}

static void replaceInList(ProStringList *varlist,
        const QRegExp &regexp, const QString &replace, bool global, QString &tmp)
{
    for (ProStringList::Iterator varit = varlist->begin(); varit != varlist->end(); ) {
        QString val = varit->toQString(tmp);
        QString copy = val; // Force detach and have a reference value
        val.replace(regexp, replace);
        if (!val.isSharedWith(copy) && val != copy) {
            if (val.isEmpty()) {
                varit = varlist->erase(varit);
            } else {
                (*varit).setValue(val);
                ++varit;
            }
            if (!global)
                break;
        } else {
            ++varit;
        }
    }
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

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateExpression(
        const ushort *&tokPtr, ProStringList *ret, bool joined)
{
    debugMsg(2, joined ? "evaluating joined expression" : "evaluating expression");
    ProFile *pro = m_current.pro;
    if (joined)
        *ret << ProString();
    bool pending = false;
    forever {
        ushort tok = *tokPtr++;
        if (tok & TokNewStr) {
            debugMsg(2, "new string");
            pending = false;
        }
        ushort maskedTok = tok & TokMask;
        switch (maskedTok) {
        case TokLine:
            m_current.line = *tokPtr++;
            break;
        case TokLiteral: {
            const ProString &val = pro->getStr(tokPtr);
            debugMsg(2, "literal %s", dbgStr(val));
            addStr(val, ret, pending, joined);
            break; }
        case TokHashLiteral: {
            const ProKey &val = pro->getHashStr(tokPtr);
            debugMsg(2, "hashed literal %s", dbgStr(val.toString()));
            addStr(val, ret, pending, joined);
            break; }
        case TokVariable: {
            const ProKey &var = pro->getHashStr(tokPtr);
            const ProStringList &val = values(map(var));
            debugMsg(2, "variable %s => %s", dbgKey(var), dbgStrList(val));
            addStrList(val, tok, ret, pending, joined);
            break; }
        case TokProperty: {
            const ProKey &var = pro->getHashStr(tokPtr);
            const ProString &val = propertyValue(var);
            debugMsg(2, "property %s => %s", dbgKey(var), dbgStr(val));
            addStr(val, ret, pending, joined);
            break; }
        case TokEnvVar: {
            const ProString &var = pro->getStr(tokPtr);
            const ProString &val = ProString(m_option->getEnv(var.toQString()));
            debugMsg(2, "env var %s => %s", dbgStr(var), dbgStr(val));
            addStr(val, ret, pending, joined);
            break; }
        case TokFuncName: {
            const ProKey &func = pro->getHashStr(tokPtr);
            debugMsg(2, "function %s", dbgKey(func));
            ProStringList val;
            if (evaluateExpandFunction(func, tokPtr, &val) == ReturnError)
                return ReturnError;
            addStrList(val, tok, ret, pending, joined);
            break; }
        default:
            debugMsg(2, "evaluated expression => %s", dbgStrList(*ret));
            tokPtr--;
            return ReturnTrue;
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
            case TokEnvVar:
                skipStr(tokPtr);
                break;
            case TokHashLiteral:
            case TokVariable:
            case TokProperty:
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
    traceMsg("entering block");
    ProStringList curr;
    ProFile *pro = m_current.pro;
    bool okey = true, or_op = false, invert = false;
    uint blockLen;
    while (ushort tok = *tokPtr++) {
        VisitReturn ret;
        switch (tok) {
        case TokLine:
            m_current.line = *tokPtr++;
            continue;
        case TokAssign:
        case TokAppend:
        case TokAppendUnique:
        case TokRemove:
        case TokReplace:
            ret = visitProVariable(tok, curr, tokPtr);
            if (ret == ReturnError)
                break;
            curr.clear();
            continue;
        case TokBranch:
            blockLen = getBlockLen(tokPtr);
            if (m_cumulative) {
#ifdef PROEVALUATOR_CUMULATIVE
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
#endif
            } else {
                if (okey) {
                    traceMsg("taking 'then' branch");
                    ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
                    traceMsg("finished 'then' branch");
                }
                tokPtr += blockLen;
                blockLen = getBlockLen(tokPtr);
                if (!okey) {
                    traceMsg("taking 'else' branch");
                    ret = blockLen ? visitProBlock(tokPtr) : ReturnTrue;
                    traceMsg("finished 'else' branch");
                }
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            break;
        case TokForLoop:
            if (m_cumulative || okey != or_op) {
                const ProKey &variable = pro->getHashStr(tokPtr);
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
                traceMsg("skipped loop");
                ret = ReturnTrue;
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            break;
        case TokTestDef:
        case TokReplaceDef:
            if (m_cumulative || okey != or_op) {
                const ProKey &name = pro->getHashStr(tokPtr);
                blockLen = getBlockLen(tokPtr);
                visitProFunctionDef(tok, name, tokPtr);
                traceMsg("defined %s function %s",
                      tok == TokTestDef ? "test" : "replace", dbgKey(name));
            } else {
                traceMsg("skipped function definition");
                skipHashStr(tokPtr);
                blockLen = getBlockLen(tokPtr);
            }
            tokPtr += blockLen;
            okey = true, or_op = false; // force next evaluation
            continue;
        case TokNot:
            traceMsg("NOT");
            invert ^= true;
            continue;
        case TokAnd:
            traceMsg("AND");
            or_op = false;
            continue;
        case TokOr:
            traceMsg("OR");
            or_op = true;
            continue;
        case TokCondition:
            if (!m_skipLevel && okey != or_op) {
                if (curr.size() != 1) {
                    if (!m_cumulative || !curr.isEmpty())
                        evalError(fL1S("Conditional must expand to exactly one word."));
                    okey = false;
                } else {
                    okey = isActiveConfig(curr.at(0).toQString(m_tmp2), true);
                    traceMsg("condition %s is %s", dbgStr(curr.at(0)), dbgBool(okey));
                    okey ^= invert;
                }
            } else {
                traceMsg("skipped condition %s", curr.size() == 1 ? dbgStr(curr.at(0)) : "<invalid>");
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
                    traceMsg("evaluating test function %s", dbgStr(curr.at(0)));
                    ret = evaluateConditionalFunction(curr.at(0).toKey(), tokPtr);
                    switch (ret) {
                    case ReturnTrue: okey = true; break;
                    case ReturnFalse: okey = false; break;
                    default:
                        traceMsg("aborting block, function status: %s", dbgReturn(ret));
                        return ret;
                    }
                    traceMsg("test function returned %s", dbgBool(okey));
                    okey ^= invert;
                }
            } else if (m_cumulative) {
#ifdef PROEVALUATOR_CUMULATIVE
                m_skipLevel++;
                if (curr.size() != 1)
                    skipExpression(tokPtr);
                else
                    evaluateConditionalFunction(curr.at(0).toKey(), tokPtr);
                m_skipLevel--;
#endif
            } else {
                skipExpression(tokPtr);
                traceMsg("skipped test function %s", curr.size() == 1 ? dbgStr(curr.at(0)) : "<invalid>");
            }
            or_op = !okey; // tentatively force next evaluation
            invert = false;
            curr.clear();
            continue;
        case TokReturn:
            m_returnValue = curr;
            curr.clear();
            ret = ReturnReturn;
            goto ctrlstm;
        case TokBreak:
            ret = ReturnBreak;
            goto ctrlstm;
        case TokNext:
            ret = ReturnNext;
          ctrlstm:
            if (!m_skipLevel && okey != or_op) {
                traceMsg("flow control statement '%s', aborting block", dbgReturn(ret));
                return ret;
            }
            traceMsg("skipped flow control statement '%s'", dbgReturn(ret));
            okey = false, or_op = true; // force next evaluation
            continue;
        default: {
                const ushort *oTokPtr = --tokPtr;
                ret = evaluateExpression(tokPtr, &curr, false);
                if (ret == ReturnError || tokPtr != oTokPtr)
                    break;
            }
            Q_ASSERT_X(false, "visitProBlock", "unexpected item type");
            continue;
        }
        if (ret != ReturnTrue && ret != ReturnFalse) {
            traceMsg("aborting block, status: %s", dbgReturn(ret));
            return ret;
        }
    }
    traceMsg("leaving block, okey=%s", dbgBool(okey));
    return returnBool(okey);
}


void QMakeEvaluator::visitProFunctionDef(
        ushort tok, const ProKey &name, const ushort *tokPtr)
{
    QHash<ProKey, ProFunctionDef> *hash =
            (tok == TokTestDef
             ? &m_functionDefs.testFunctions
             : &m_functionDefs.replaceFunctions);
    hash->insert(name, ProFunctionDef(m_current.pro, tokPtr - m_current.pro->tokPtr()));
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProLoop(
        const ProKey &_variable, const ushort *exprPtr, const ushort *tokPtr)
{
    VisitReturn ret = ReturnTrue;
    bool infinite = false;
    int index = 0;
    ProKey variable;
    ProStringList oldVarVal;
    ProStringList it_list_out;
    if (expandVariableReferences(exprPtr, 0, &it_list_out, true) == ReturnError)
        return ReturnError;
    ProString it_list = it_list_out.at(0);
    if (_variable.isEmpty()) {
        if (it_list != statics.strever) {
            evalError(fL1S("Invalid loop expression."));
            return ReturnFalse;
        }
        it_list = ProString(statics.strforever);
    } else {
        variable = map(_variable);
        oldVarVal = values(variable);
    }
    ProStringList list = values(it_list.toKey());
    if (list.isEmpty()) {
        if (it_list == statics.strforever) {
            if (m_cumulative) {
                // The termination conditions wouldn't be evaluated, so we must skip it.
                traceMsg("skipping forever loop in cumulative mode");
                return ReturnFalse;
            }
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
                        const int absDiff = qAbs(end - start);
                        if (m_cumulative && absDiff > 100) {
                            // Such a loop is unlikely to contribute something useful to the
                            // file collection, and may cause considerable delay.
                            traceMsg("skipping excessive loop in cumulative mode");
                            return ReturnFalse;
                        }
                        list.reserve(absDiff + 1);
                        if (start < end) {
                            for (int i = start; i <= end; i++)
                                list << ProString(QString::number(i));
                        } else {
                            for (int i = start; i >= end; i--)
                                list << ProString(QString::number(i));
                        }
                    }
                }
            }
        }
    }

    if (infinite)
        traceMsg("entering infinite loop for %s", dbgKey(variable));
    else
        traceMsg("entering loop for %s over %s", dbgKey(variable), dbgStrList(list));

    forever {
        if (infinite) {
            if (!variable.isEmpty())
                m_valuemapStack.top()[variable] = ProStringList(ProString(QString::number(index)));
            if (++index > 1000) {
                evalError(fL1S("Ran into infinite loop (> 1000 iterations)."));
                break;
            }
            traceMsg("loop iteration %d", index);
        } else {
            ProString val;
            do {
                if (index >= list.count())
                    goto do_break;
                val = list.at(index++);
            } while (val.isEmpty()); // stupid, but qmake is like that
            traceMsg("loop iteration %s", dbgStr(val));
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

    traceMsg("done looping");

    if (!variable.isEmpty())
        m_valuemapStack.top()[variable] = oldVarVal;
    return ret;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProVariable(
        ushort tok, const ProStringList &curr, const ushort *&tokPtr)
{
    int sizeHint = *tokPtr++;

    if (curr.size() != 1) {
        skipExpression(tokPtr);
        if (!m_cumulative || !curr.isEmpty())
            evalError(fL1S("Left hand side of assignment must expand to exactly one word."));
        return ReturnTrue;
    }
    const ProKey &varName = map(curr.first());

    if (tok == TokReplace) {      // ~=
        // DEFINES ~= s/a/b/?[gqi]

        ProStringList varVal;
        if (expandVariableReferences(tokPtr, sizeHint, &varVal, true) == ReturnError)
            return ReturnError;
        const QString &val = varVal.at(0).toQString(m_tmp1);
        if (val.length() < 4 || val.at(0) != QLatin1Char('s')) {
            evalError(fL1S("The ~= operator can handle only the s/// function."));
            return ReturnTrue;
        }
        QChar sep = val.at(1);
        QStringList func = val.split(sep);
        if (func.count() < 3 || func.count() > 4) {
            evalError(fL1S("The s/// function expects 3 or 4 arguments."));
            return ReturnTrue;
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

        // We could make a union of modified and unmodified values,
        // but this will break just as much as it fixes, so leave it as is.
        replaceInList(&valuesRef(varName), regexp, replace, global, m_tmp2);
        debugMsg(2, "replaced %s with %s", dbgQStr(pattern), dbgQStr(replace));
    } else {
        ProStringList varVal;
        if (expandVariableReferences(tokPtr, sizeHint, &varVal, false) == ReturnError)
            return ReturnError;
        switch (tok) {
        default: // whatever - cannot happen
        case TokAssign:          // =
            varVal.removeEmpty();
            // FIXME: add check+warning about accidental value removal.
            // This may be a bit too noisy, though.
            m_valuemapStack.top()[varName] = varVal;
            debugMsg(2, "assigning");
            break;
        case TokAppendUnique:    // *=
            valuesRef(varName).insertUnique(varVal);
            debugMsg(2, "appending unique");
            break;
        case TokAppend:          // +=
            varVal.removeEmpty();
            valuesRef(varName) += varVal;
            debugMsg(2, "appending");
            break;
        case TokRemove:       // -=
            if (!m_cumulative) {
                valuesRef(varName).removeEach(varVal);
            } else {
                // We are stingy with our values.
            }
            debugMsg(2, "removing");
            break;
        }
    }
    traceMsg("%s := %s", dbgKey(varName), dbgStrList(values(varName)));

    if (varName == statics.strTEMPLATE)
        setTemplate();
    else if (varName == statics.strQMAKE_PLATFORM)
        m_featureRoots = 0;
    else if (varName == statics.strQMAKESPEC) {
        if (!values(varName).isEmpty()) {
            QString spec = values(varName).first().toQString();
            if (IoUtils::isAbsolutePath(spec)) {
                m_qmakespec = spec;
                m_featureRoots = 0;
            }
        }
    }
#ifdef PROEVALUATOR_FULL
    else if (varName == statics.strREQUIRES)
        return checkRequirements(values(varName));
#endif

    return ReturnTrue;
}

void QMakeEvaluator::setTemplate()
{
    ProStringList &values = valuesRef(statics.strTEMPLATE);
    if (!m_option->user_template.isEmpty()) {
        // Don't allow override
        values = ProStringList(ProString(m_option->user_template));
    } else {
        if (values.isEmpty())
            values.append(ProString("app"));
        else
            values.erase(values.begin() + 1, values.end());
    }
    if (!m_option->user_template_prefix.isEmpty()) {
        QString val = values.first().toQString(m_tmp1);
        if (!val.startsWith(m_option->user_template_prefix)) {
            val.prepend(m_option->user_template_prefix);
            values = ProStringList(ProString(val));
        }
    }
}

#if defined(Q_CC_MSVC)
static ProString msvcBinDirToQMakeArch(QString subdir)
{
    int idx = subdir.indexOf(QLatin1Char('\\'));
    if (idx == -1)
        return ProString("x86");
    subdir.remove(0, idx + 1);
    idx = subdir.indexOf(QLatin1Char('_'));
    if (idx >= 0)
        subdir.remove(0, idx + 1);
    subdir = subdir.toLower();
    if (subdir == QLatin1String("amd64"))
        return ProString("x86_64");
    return ProString(subdir);
}

static ProString defaultMsvcArchitecture()
{
#if defined(Q_OS_WIN64)
    return ProString("x86_64");
#else
    return ProString("x86");
#endif
}

static ProString msvcArchitecture(const QString &vcInstallDir, const QString &pathVar)
{
    if (vcInstallDir.isEmpty())
        return defaultMsvcArchitecture();
    QString vcBinDir = vcInstallDir;
    if (vcBinDir.endsWith(QLatin1Char('\\')))
        vcBinDir.chop(1);
    const auto dirs = pathVar.split(QLatin1Char(';'));
    for (const QString &dir : dirs) {
        if (!dir.startsWith(vcBinDir, Qt::CaseInsensitive))
            continue;
        const ProString arch = msvcBinDirToQMakeArch(dir.mid(vcBinDir.length() + 1));
        if (!arch.isEmpty())
            return arch;
    }
    return defaultMsvcArchitecture();
}
#endif // defined(Q_CC_MSVC)

void QMakeEvaluator::loadDefaults()
{
    ProValueMap &vars = m_valuemapStack.top();

    vars[ProKey("DIR_SEPARATOR")] << ProString(m_option->dir_sep);
    vars[ProKey("DIRLIST_SEPARATOR")] << ProString(m_option->dirlist_sep);
    vars[ProKey("_DATE_")] << ProString(QDateTime::currentDateTime().toString());
    if (!m_option->qmake_abslocation.isEmpty())
        vars[ProKey("QMAKE_QMAKE")] << ProString(m_option->qmake_abslocation);
    if (!m_option->qmake_args.isEmpty())
        vars[ProKey("QMAKE_ARGS")] = ProStringList(m_option->qmake_args);
    if (!m_option->qtconf.isEmpty())
        vars[ProKey("QMAKE_QTCONF")] = ProString(m_option->qtconf);
    vars[ProKey("QMAKE_HOST.cpu_count")] = ProString(QString::number(idealThreadCount()));
#if defined(Q_OS_WIN32)
    vars[ProKey("QMAKE_HOST.os")] << ProString("Windows");

    DWORD name_length = 1024;
    wchar_t name[1024];
    if (GetComputerName(name, &name_length))
        vars[ProKey("QMAKE_HOST.name")] << ProString(QString::fromWCharArray(name));

    QSysInfo::WinVersion ver = QSysInfo::WindowsVersion;
    vars[ProKey("QMAKE_HOST.version")] << ProString(QString::number(ver));
    ProString verStr;
    switch (ver) {
    case QSysInfo::WV_Me: verStr = ProString("WinMe"); break;
    case QSysInfo::WV_95: verStr = ProString("Win95"); break;
    case QSysInfo::WV_98: verStr = ProString("Win98"); break;
    case QSysInfo::WV_NT: verStr = ProString("WinNT"); break;
    case QSysInfo::WV_2000: verStr = ProString("Win2000"); break;
    case QSysInfo::WV_2003: verStr = ProString("Win2003"); break;
    case QSysInfo::WV_XP: verStr = ProString("WinXP"); break;
    case QSysInfo::WV_VISTA: verStr = ProString("WinVista"); break;
    default: verStr = ProString("Unknown"); break;
    }
    vars[ProKey("QMAKE_HOST.version_string")] << verStr;

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    ProString archStr;
    switch (info.wProcessorArchitecture) {
# ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64:
        archStr = ProString("x86_64");
        break;
# endif
    case PROCESSOR_ARCHITECTURE_INTEL:
        archStr = ProString("x86");
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
# ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
# endif
        archStr = ProString("IA64");
        break;
    default:
        archStr = ProString("Unknown");
        break;
    }
    vars[ProKey("QMAKE_HOST.arch")] << archStr;

# if defined(Q_CC_MSVC) // ### bogus condition, but nobody x-builds for msvc with a different qmake
    vars[ProKey("QMAKE_TARGET.arch")] = msvcArchitecture(
                m_option->getEnv(QLatin1String("VCINSTALLDIR")),
                m_option->getEnv(QLatin1String("PATH")));
# endif
#elif defined(Q_OS_UNIX)
    struct utsname name;
    if (uname(&name) != -1) {
        vars[ProKey("QMAKE_HOST.os")] << ProString(name.sysname);
        vars[ProKey("QMAKE_HOST.name")] << ProString(QString::fromLocal8Bit(name.nodename));
        vars[ProKey("QMAKE_HOST.version")] << ProString(name.release);
        vars[ProKey("QMAKE_HOST.version_string")] << ProString(name.version);
        vars[ProKey("QMAKE_HOST.arch")] << ProString(name.machine);
    }
#endif

    m_valuemapInited = true;
}

bool QMakeEvaluator::prepareProject(const QString &inDir)
{
    QMakeVfs::VfsFlags flags = (m_cumulative ? QMakeVfs::VfsCumulative : QMakeVfs::VfsExact);
    QString superdir;
    if (m_option->do_cache) {
        QString conffile;
        QString cachefile = m_option->cachefile;
        if (cachefile.isEmpty())  { //find it as it has not been specified
            if (m_outputDir.isEmpty())
                goto no_cache;
            superdir = m_outputDir;
            forever {
                QString superfile = superdir + QLatin1String("/.qmake.super");
                if (m_vfs->exists(superfile, flags)) {
                    m_superfile = QDir::cleanPath(superfile);
                    break;
                }
                QFileInfo qdfi(superdir);
                if (qdfi.isRoot()) {
                    superdir.clear();
                    break;
                }
                superdir = qdfi.path();
            }
            QString sdir = inDir;
            QString dir = m_outputDir;
            forever {
                conffile = sdir + QLatin1String("/.qmake.conf");
                if (!m_vfs->exists(conffile, flags))
                    conffile.clear();
                cachefile = dir + QLatin1String("/.qmake.cache");
                if (!m_vfs->exists(cachefile, flags))
                    cachefile.clear();
                if (!conffile.isEmpty() || !cachefile.isEmpty()) {
                    if (dir != sdir)
                        m_sourceRoot = sdir;
                    m_buildRoot = dir;
                    break;
                }
                if (dir == superdir)
                    goto no_cache;
                QFileInfo qsdfi(sdir);
                QFileInfo qdfi(dir);
                if (qsdfi.isRoot() || qdfi.isRoot())
                    goto no_cache;
                sdir = qsdfi.path();
                dir = qdfi.path();
            }
        } else {
            m_buildRoot = QFileInfo(cachefile).path();
        }
        m_conffile = QDir::cleanPath(conffile);
        m_cachefile = QDir::cleanPath(cachefile);
    }
  no_cache:

    QString dir = m_outputDir;
    forever {
        QString stashfile = dir + QLatin1String("/.qmake.stash");
        if (dir == (!superdir.isEmpty() ? superdir : m_buildRoot) || m_vfs->exists(stashfile, flags)) {
            m_stashfile = QDir::cleanPath(stashfile);
            break;
        }
        QFileInfo qdfi(dir);
        if (qdfi.isRoot())
            break;
        dir = qdfi.path();
    }

    return true;
}

bool QMakeEvaluator::loadSpecInternal()
{
    if (evaluateFeatureFile(QLatin1String("spec_pre.prf")) != ReturnTrue)
        return false;
    QString spec = m_qmakespec + QLatin1String("/qmake.conf");
    if (evaluateFile(spec, QMakeHandler::EvalConfigFile, LoadProOnly) != ReturnTrue) {
        evalError(fL1S("Could not read qmake configuration file %1.").arg(spec));
        return false;
    }
#ifndef QT_BUILD_QMAKE
    // Legacy support for Qt4 default specs
#  ifdef Q_OS_UNIX
    if (m_qmakespec.endsWith(QLatin1String("/default-host"))
        || m_qmakespec.endsWith(QLatin1String("/default"))) {
        QString rspec = QFileInfo(m_qmakespec).readLink();
        if (!rspec.isEmpty())
            m_qmakespec = QDir::cleanPath(QDir(m_qmakespec).absoluteFilePath(rspec));
    }
#  else
    // We can't resolve symlinks as they do on Unix, so configure.exe puts
    // the source of the qmake.conf at the end of the default/qmake.conf in
    // the QMAKESPEC_ORIGINAL variable.
    const ProString &orig_spec = first(ProKey("QMAKESPEC_ORIGINAL"));
    if (!orig_spec.isEmpty()) {
        QString spec = orig_spec.toQString();
        if (IoUtils::isAbsolutePath(spec))
            m_qmakespec = spec;
    }
#  endif
#endif
    valuesRef(ProKey("QMAKESPEC")) = ProString(m_qmakespec);
    m_qmakespecName = IoUtils::fileName(m_qmakespec).toString();
    // This also ensures that m_featureRoots is valid.
    if (evaluateFeatureFile(QLatin1String("spec_post.prf")) != ReturnTrue)
        return false;
    // The MinGW and x-build specs may change the separator; $$shell_{path,quote}() need it
    m_dirSep = first(ProKey("QMAKE_DIR_SEP"));
    return true;
}

bool QMakeEvaluator::loadSpec()
{
    QString qmakespec = m_option->expandEnvVars(
                m_hostBuild ? m_option->qmakespec : m_option->xqmakespec);

    {
        QMakeEvaluator evaluator(m_option, m_parser, m_vfs, m_handler);
        evaluator.m_sourceRoot = m_sourceRoot;
        evaluator.m_buildRoot = m_buildRoot;

        if (!m_superfile.isEmpty() && evaluator.evaluateFile(
                m_superfile, QMakeHandler::EvalConfigFile, LoadProOnly|LoadHidden) != ReturnTrue) {
            return false;
        }
        if (!m_conffile.isEmpty() && evaluator.evaluateFile(
                m_conffile, QMakeHandler::EvalConfigFile, LoadProOnly|LoadHidden) != ReturnTrue) {
            return false;
        }
        if (!m_cachefile.isEmpty() && evaluator.evaluateFile(
                m_cachefile, QMakeHandler::EvalConfigFile, LoadProOnly|LoadHidden) != ReturnTrue) {
            return false;
        }
        if (qmakespec.isEmpty()) {
            if (!m_hostBuild)
                qmakespec = evaluator.first(ProKey("XQMAKESPEC")).toQString();
            if (qmakespec.isEmpty())
                qmakespec = evaluator.first(ProKey("QMAKESPEC")).toQString();
        }
        m_qmakepath = evaluator.values(ProKey("QMAKEPATH")).toQStringList();
        m_qmakefeatures = evaluator.values(ProKey("QMAKEFEATURES")).toQStringList();
    }

    updateMkspecPaths();
    if (qmakespec.isEmpty())
        qmakespec = propertyValue(ProKey(m_hostBuild ? "QMAKE_SPEC" : "QMAKE_XSPEC")).toQString();
#ifndef QT_BUILD_QMAKE
    // Legacy support for Qt4 qmake in Qt Creator, etc.
    if (qmakespec.isEmpty())
        qmakespec = m_hostBuild ? QLatin1String("default-host") : QLatin1String("default");
#endif
    if (IoUtils::isRelativePath(qmakespec)) {
        foreach (const QString &root, m_mkspecPaths) {
            QString mkspec = root + QLatin1Char('/') + qmakespec;
            if (IoUtils::exists(mkspec)) {
                qmakespec = mkspec;
                goto cool;
            }
        }
        evalError(fL1S("Could not find qmake spec '%1'.").arg(qmakespec));
        return false;
    }
  cool:
    m_qmakespec = QDir::cleanPath(qmakespec);

    if (!m_superfile.isEmpty()) {
        valuesRef(ProKey("_QMAKE_SUPER_CACHE_")) << ProString(m_superfile);
        if (evaluateFile(
                m_superfile, QMakeHandler::EvalConfigFile, LoadProOnly|LoadHidden) != ReturnTrue)
            return false;
    }
    if (!loadSpecInternal())
        return false;
    if (!m_conffile.isEmpty()) {
        valuesRef(ProKey("_QMAKE_CONF_")) << ProString(m_conffile);
        if (evaluateFile(
                m_conffile, QMakeHandler::EvalConfigFile, LoadProOnly) != ReturnTrue)
            return false;
    }
    if (!m_cachefile.isEmpty()) {
        valuesRef(ProKey("_QMAKE_CACHE_")) << ProString(m_cachefile);
        if (evaluateFile(
                m_cachefile, QMakeHandler::EvalConfigFile, LoadProOnly) != ReturnTrue)
            return false;
    }
    QMakeVfs::VfsFlags flags = (m_cumulative ? QMakeVfs::VfsCumulative : QMakeVfs::VfsExact);
    if (!m_stashfile.isEmpty() && m_vfs->exists(m_stashfile, flags)) {
        valuesRef(ProKey("_QMAKE_STASH_")) << ProString(m_stashfile);
        if (evaluateFile(
                m_stashfile, QMakeHandler::EvalConfigFile, LoadProOnly) != ReturnTrue)
            return false;
    }
    return true;
}

void QMakeEvaluator::setupProject()
{
    setTemplate();
    ProValueMap &vars = m_valuemapStack.top();
    ProFile *proFile = currentProFile();
    vars[ProKey("TARGET")] << ProString(QFileInfo(currentFileName()).baseName()).setSource(proFile);
    vars[ProKey("_PRO_FILE_")] << ProString(currentFileName()).setSource(proFile);
    vars[ProKey("_PRO_FILE_PWD_")] << ProString(currentDirectory()).setSource(proFile);
    vars[ProKey("OUT_PWD")] << ProString(m_outputDir).setSource(proFile);
}

void QMakeEvaluator::evaluateCommand(const QString &cmds, const QString &where)
{
    if (!cmds.isEmpty()) {
        ProFile *pro = m_parser->parsedProBlock(QStringRef(&cmds), where, -1);
        if (pro->isOk()) {
            m_locationStack.push(m_current);
            visitProBlock(pro, pro->tokPtr());
            m_current = m_locationStack.pop();
        }
        pro->deref();
    }
}

void QMakeEvaluator::applyExtraConfigs()
{
    if (m_extraConfigs.isEmpty())
        return;

    evaluateCommand(fL1S("CONFIG += ") + m_extraConfigs.join(QLatin1Char(' ')), fL1S("(extra configs)"));
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConfigFeatures()
{
    QSet<QString> processed;
    forever {
        bool finished = true;
        ProStringList configs = values(statics.strCONFIG);
        for (int i = configs.size() - 1; i >= 0; --i) {
            QString config = configs.at(i).toQString(m_tmp1).toLower();
            if (!processed.contains(config)) {
                config.detach();
                processed.insert(config);
                VisitReturn vr = evaluateFeatureFile(config, true);
                if (vr == ReturnError && !m_cumulative)
                    return vr;
                if (vr == ReturnTrue) {
                    finished = false;
                    break;
                }
            }
        }
        if (finished)
            break;
    }
    return ReturnTrue;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::visitProFile(
        ProFile *pro, QMakeHandler::EvalFileType type, LoadFlags flags)
{
    if (!m_cumulative && !pro->isOk())
        return ReturnFalse;

    if (flags & LoadPreFiles) {
        if (!prepareProject(pro->directoryName()))
            return ReturnFalse;

        m_hostBuild = pro->isHostBuild();

#ifdef PROEVALUATOR_THREAD_SAFE
        m_option->mutex.lock();
#endif
        QMakeBaseEnv **baseEnvPtr = &m_option->baseEnvs[QMakeBaseKey(m_buildRoot, m_stashfile, m_hostBuild)];
        if (!*baseEnvPtr)
            *baseEnvPtr = new QMakeBaseEnv;
        QMakeBaseEnv *baseEnv = *baseEnvPtr;

#ifdef PROEVALUATOR_THREAD_SAFE
        QMutexLocker locker(&baseEnv->mutex);
        m_option->mutex.unlock();
        if (baseEnv->inProgress) {
            QThreadPool::globalInstance()->releaseThread();
            baseEnv->cond.wait(&baseEnv->mutex);
            QThreadPool::globalInstance()->reserveThread();
            if (!baseEnv->isOk)
                return ReturnFalse;
        } else
#endif
        if (!baseEnv->evaluator) {
#ifdef PROEVALUATOR_THREAD_SAFE
            baseEnv->inProgress = true;
            locker.unlock();
#endif

            QMakeEvaluator *baseEval = new QMakeEvaluator(m_option, m_parser, m_vfs, m_handler);
            baseEnv->evaluator = baseEval;
            baseEval->m_superfile = m_superfile;
            baseEval->m_conffile = m_conffile;
            baseEval->m_cachefile = m_cachefile;
            baseEval->m_stashfile = m_stashfile;
            baseEval->m_sourceRoot = m_sourceRoot;
            baseEval->m_buildRoot = m_buildRoot;
            baseEval->m_hostBuild = m_hostBuild;
            bool ok = baseEval->loadSpec();

#ifdef PROEVALUATOR_THREAD_SAFE
            locker.relock();
            baseEnv->isOk = ok;
            baseEnv->inProgress = false;
            baseEnv->cond.wakeAll();
#endif

            if (!ok)
                return ReturnFalse;
        }
#ifdef PROEVALUATOR_THREAD_SAFE
        else if (!baseEnv->isOk)
            return ReturnFalse;
#endif

        initFrom(baseEnv->evaluator);
    } else {
        if (!m_valuemapInited)
            loadDefaults();
    }

    VisitReturn vr;

    m_handler->aboutToEval(currentProFile(), pro, type);
    m_profileStack.push(pro);
    valuesRef(ProKey("PWD")) = ProStringList(ProString(currentDirectory()));
    if (flags & LoadPreFiles) {
        setupProject();

        for (ProValueMap::ConstIterator it = m_extraVars.constBegin();
             it != m_extraVars.constEnd(); ++it)
            m_valuemapStack.first().insert(it.key(), it.value());

        // In case default_pre needs to make decisions based on the current
        // build pass configuration.
        applyExtraConfigs();

        if ((vr = evaluateFeatureFile(QLatin1String("default_pre.prf"))) == ReturnError)
            goto failed;

        if (!m_option->precmds.isEmpty()) {
            evaluateCommand(m_option->precmds, fL1S("(command line)"));

            // Again, after user configs, to override them
            applyExtraConfigs();
        }
    }

    debugMsg(1, "visiting file %s", qPrintable(pro->fileName()));
    if ((vr = visitProBlock(pro, pro->tokPtr())) == ReturnError)
        goto failed;
    debugMsg(1, "done visiting file %s", qPrintable(pro->fileName()));

    if (flags & LoadPostFiles) {
        evaluateCommand(m_option->postcmds, fL1S("(command line -after)"));

        // Again, to ensure the project does not mess with us.
        // Specifically, do not allow a project to override debug/release within a
        // debug_and_release build pass - it's too late for that at this point anyway.
        applyExtraConfigs();

        if ((vr = evaluateFeatureFile(QLatin1String("default_post.prf"))) == ReturnError)
            goto failed;

        if ((vr = evaluateConfigFeatures()) == ReturnError)
            goto failed;
    }
    vr = ReturnTrue;
  failed:
    m_profileStack.pop();
    valuesRef(ProKey("PWD")) = ProStringList(ProString(currentDirectory()));
    m_handler->doneWithEval(currentProFile());

    return vr;
}


void QMakeEvaluator::updateMkspecPaths()
{
    QStringList ret;
    const QString concat = QLatin1String("/mkspecs");

    const auto paths = m_option->getPathListEnv(QLatin1String("QMAKEPATH"));
    for (const QString &it : paths)
        ret << it + concat;

    foreach (const QString &it, m_qmakepath)
        ret << it + concat;

    if (!m_buildRoot.isEmpty())
        ret << m_buildRoot + concat;
    if (!m_sourceRoot.isEmpty())
        ret << m_sourceRoot + concat;

    ret << m_option->propertyValue(ProKey("QT_HOST_DATA/get")) + concat;
    ret << m_option->propertyValue(ProKey("QT_HOST_DATA/src")) + concat;

    ret.removeDuplicates();
    m_mkspecPaths = ret;
}

void QMakeEvaluator::updateFeaturePaths()
{
    QString mkspecs_concat = QLatin1String("/mkspecs");
    QString features_concat = QLatin1String("/features/");

    QStringList feature_roots;

    feature_roots += m_option->getPathListEnv(QLatin1String("QMAKEFEATURES"));
    feature_roots += m_qmakefeatures;
    feature_roots += m_option->splitPathList(
                m_option->propertyValue(ProKey("QMAKEFEATURES")).toQString(m_mtmp));

    QStringList feature_bases;
    if (!m_buildRoot.isEmpty()) {
        feature_bases << m_buildRoot + mkspecs_concat;
        feature_bases << m_buildRoot;
    }
    if (!m_sourceRoot.isEmpty()) {
        feature_bases << m_sourceRoot + mkspecs_concat;
        feature_bases << m_sourceRoot;
    }

    const auto items = m_option->getPathListEnv(QLatin1String("QMAKEPATH"));
    for (const QString &item : items)
        feature_bases << (item + mkspecs_concat);

    foreach (const QString &item, m_qmakepath)
        feature_bases << (item + mkspecs_concat);

    if (!m_qmakespec.isEmpty()) {
        // The spec is already platform-dependent, so no subdirs here.
        feature_roots << (m_qmakespec + features_concat);

        // Also check directly under the root directory of the mkspecs collection
        QDir specdir(m_qmakespec);
        while (!specdir.isRoot() && specdir.cdUp()) {
            const QString specpath = specdir.path();
            if (specpath.endsWith(mkspecs_concat)) {
                if (IoUtils::exists(specpath + features_concat))
                    feature_bases << specpath;
                break;
            }
        }
    }

    feature_bases << (m_option->propertyValue(ProKey("QT_HOST_DATA/get")) + mkspecs_concat);
    feature_bases << (m_option->propertyValue(ProKey("QT_HOST_DATA/src")) + mkspecs_concat);

    foreach (const QString &fb, feature_bases) {
        const auto sfxs = values(ProKey("QMAKE_PLATFORM"));
        for (const ProString &sfx : sfxs)
            feature_roots << (fb + features_concat + sfx + QLatin1Char('/'));
        feature_roots << (fb + features_concat);
    }

    for (int i = 0; i < feature_roots.count(); ++i)
        if (!feature_roots.at(i).endsWith((ushort)'/'))
            feature_roots[i].append((ushort)'/');

    feature_roots.removeDuplicates();

    QStringList ret;
    foreach (const QString &root, feature_roots)
        if (IoUtils::exists(root))
            ret << root;
    m_featureRoots = new QMakeFeatureRoots(ret);
}

ProString QMakeEvaluator::propertyValue(const ProKey &name) const
{
    if (name == QLatin1String("QMAKE_MKSPECS"))
        return ProString(m_mkspecPaths.join(m_option->dirlist_sep));
    ProString ret = m_option->propertyValue(name);
//    if (ret.isNull())
//        evalError(fL1S("Querying unknown property %1").arg(name.toQString(m_mtmp)));
    return ret;
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
    ProFile *pro = currentProFile();
    if (pro)
        return pro->directoryName();
    return QString();
}

bool QMakeEvaluator::isActiveConfig(const QString &config, bool regex)
{
    // magic types for easy flipping
    if (config == statics.strtrue)
        return true;
    if (config == statics.strfalse)
        return false;

    if (config == statics.strhost_build)
        return m_hostBuild;

    if (regex && (config.contains(QLatin1Char('*')) || config.contains(QLatin1Char('?')))) {
        QString cfg = config;
        cfg.detach(); // Keep m_tmp out of QRegExp's cache
        QRegExp re(cfg, Qt::CaseSensitive, QRegExp::Wildcard);

        // mkspecs
        if (re.exactMatch(m_qmakespecName))
            return true;

        // CONFIG variable
        int t = 0;
        const auto configValues = values(statics.strCONFIG);
        for (const ProString &configValue : configValues) {
            if (re.exactMatch(configValue.toQString(m_tmp[t])))
                return true;
            t ^= 1;
        }
    } else {
        // mkspecs
        if (m_qmakespecName == config)
            return true;

        // CONFIG variable
        if (values(statics.strCONFIG).contains(ProString(config)))
            return true;
    }

    return false;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::expandVariableReferences(
        const ushort *&tokPtr, int sizeHint, ProStringList *ret, bool joined)
{
    ret->reserve(sizeHint);
    forever {
        if (evaluateExpression(tokPtr, ret, joined) == ReturnError)
            return ReturnError;
        switch (*tokPtr) {
        case TokValueTerminator:
        case TokFuncTerminator:
            tokPtr++;
            return ReturnTrue;
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

QMakeEvaluator::VisitReturn QMakeEvaluator::prepareFunctionArgs(
        const ushort *&tokPtr, QList<ProStringList> *ret)
{
    if (*tokPtr != TokFuncTerminator) {
        for (;; tokPtr++) {
            ProStringList arg;
            if (evaluateExpression(tokPtr, &arg, false) == ReturnError)
                return ReturnError;
            *ret << arg;
            if (*tokPtr == TokFuncTerminator)
                break;
            Q_ASSERT(*tokPtr == TokArgSeparator);
        }
    }
    tokPtr++;
    return ReturnTrue;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateFunction(
        const ProFunctionDef &func, const QList<ProStringList> &argumentsList, ProStringList *ret)
{
    VisitReturn vr;

    if (m_valuemapStack.count() >= 100) {
        evalError(fL1S("Ran into infinite recursion (depth > 100)."));
        vr = ReturnFalse;
    } else {
        m_valuemapStack.push(ProValueMap());
        m_locationStack.push(m_current);

        ProStringList args;
        for (int i = 0; i < argumentsList.count(); ++i) {
            args += argumentsList[i];
            m_valuemapStack.top()[ProKey(QString::number(i+1))] = argumentsList[i];
        }
        m_valuemapStack.top()[statics.strARGS] = args;
        m_valuemapStack.top()[statics.strARGC] = ProStringList(ProString(QString::number(argumentsList.count())));
        vr = visitProBlock(func.pro(), func.tokPtr());
        if (vr == ReturnReturn)
            vr = ReturnTrue;
        if (vr == ReturnTrue)
            *ret = m_returnValue;
        m_returnValue.clear();

        m_current = m_locationStack.pop();
        m_valuemapStack.pop();
    }
    return vr;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateBoolFunction(
        const ProFunctionDef &func, const QList<ProStringList> &argumentsList,
        const ProString &function)
{
    ProStringList ret;
    VisitReturn vr = evaluateFunction(func, argumentsList, &ret);
    if (vr == ReturnTrue) {
        if (ret.isEmpty())
            return ReturnTrue;
        if (ret.at(0) != statics.strfalse) {
            if (ret.at(0) == statics.strtrue)
                return ReturnTrue;
            bool ok;
            int val = ret.at(0).toQString(m_tmp1).toInt(&ok);
            if (ok) {
                if (val)
                    return ReturnTrue;
            } else {
                evalError(fL1S("Unexpected return value from test '%1': %2.")
                          .arg(function.toQString(m_tmp1))
                          .arg(ret.join(QLatin1String(" :: "))));
            }
        }
        return ReturnFalse;
    }
    return vr;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditionalFunction(
        const ProKey &func, const ushort *&tokPtr)
{
    if (int func_t = statics.functions.value(func)) {
        //why don't the builtin functions just use args_list? --Sam
        ProStringList args;
        if (expandVariableReferences(tokPtr, 5, &args, true) == ReturnError)
            return ReturnError;
        return evaluateBuiltinConditional(func_t, func, args);
    }

    QHash<ProKey, ProFunctionDef>::ConstIterator it =
            m_functionDefs.testFunctions.constFind(func);
    if (it != m_functionDefs.testFunctions.constEnd()) {
        QList<ProStringList> args;
        if (prepareFunctionArgs(tokPtr, &args) == ReturnError)
            return ReturnError;
        traceMsg("calling %s(%s)", dbgKey(func), dbgStrListList(args));
        return evaluateBoolFunction(*it, args, func);
    }

    skipExpression(tokPtr);
    evalError(fL1S("'%1' is not a recognized test function.").arg(func.toQString(m_tmp1)));
    return ReturnFalse;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateExpandFunction(
        const ProKey &func, const ushort *&tokPtr, ProStringList *ret)
{
    if (int func_t = statics.expands.value(func)) {
        //why don't the builtin functions just use args_list? --Sam
        ProStringList args;
        if (expandVariableReferences(tokPtr, 5, &args, true) == ReturnError)
            return ReturnError;
        *ret = evaluateBuiltinExpand(func_t, func, args);
        return ReturnTrue;
    }

    QHash<ProKey, ProFunctionDef>::ConstIterator it =
            m_functionDefs.replaceFunctions.constFind(func);
    if (it != m_functionDefs.replaceFunctions.constEnd()) {
        QList<ProStringList> args;
        if (prepareFunctionArgs(tokPtr, &args) == ReturnError)
            return ReturnError;
        traceMsg("calling $$%s(%s)", dbgKey(func), dbgStrListList(args));
        return evaluateFunction(*it, args, ret);
    }

    skipExpression(tokPtr);
    evalError(fL1S("'%1' is not a recognized replace function.").arg(func.toQString(m_tmp1)));
    return ReturnFalse;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateConditional(
        const QStringRef &cond, const QString &where, int line)
{
    VisitReturn ret = ReturnFalse;
    ProFile *pro = m_parser->parsedProBlock(cond, where, line, QMakeParser::TestGrammar);
    if (pro->isOk()) {
        m_locationStack.push(m_current);
        ret = visitProBlock(pro, pro->tokPtr());
        m_current = m_locationStack.pop();
    }
    pro->deref();
    return ret;
}

#ifdef PROEVALUATOR_FULL
QMakeEvaluator::VisitReturn QMakeEvaluator::checkRequirements(const ProStringList &deps)
{
    ProStringList &failed = valuesRef(ProKey("QMAKE_FAILED_REQUIREMENTS"));
    for (const ProString &dep : deps) {
        VisitReturn vr = evaluateConditional(dep.toQStringRef(), m_current.pro->fileName(), m_current.line);
        if (vr == ReturnError)
            return ReturnError;
        if (vr != ReturnTrue)
            failed << dep;
    }
    return ReturnTrue;
}
#endif

static bool isFunctParam(const ProKey &variableName)
{
    const int len = variableName.size();
    const QChar *data = variableName.constData();
    for (int i = 0; i < len; i++) {
        ushort c = data[i].unicode();
        if (c < '0' || c > '9')
            return false;
    }
    return true;
}

ProValueMap *QMakeEvaluator::findValues(const ProKey &variableName, ProValueMap::Iterator *rit)
{
    ProValueMapStack::Iterator vmi = m_valuemapStack.end();
    for (bool first = true; ; first = false) {
        --vmi;
        ProValueMap::Iterator it = (*vmi).find(variableName);
        if (it != (*vmi).end()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                break;
            *rit = it;
            return &(*vmi);
        }
        if (vmi == m_valuemapStack.begin())
            break;
        if (first && isFunctParam(variableName))
            break;
    }
    return 0;
}

ProStringList &QMakeEvaluator::valuesRef(const ProKey &variableName)
{
    ProValueMap::Iterator it = m_valuemapStack.top().find(variableName);
    if (it != m_valuemapStack.top().end()) {
        if (it->constBegin() == statics.fakeValue.constBegin())
            it->clear();
        return *it;
    }
    if (!isFunctParam(variableName)) {
        ProValueMapStack::Iterator vmi = m_valuemapStack.end();
        if (--vmi != m_valuemapStack.begin()) {
            do {
                --vmi;
                ProValueMap::ConstIterator it = (*vmi).constFind(variableName);
                if (it != (*vmi).constEnd()) {
                    ProStringList &ret = m_valuemapStack.top()[variableName];
                    if (it->constBegin() != statics.fakeValue.constBegin())
                        ret = *it;
                    return ret;
                }
            } while (vmi != m_valuemapStack.begin());
        }
    }
    return m_valuemapStack.top()[variableName];
}

ProStringList QMakeEvaluator::values(const ProKey &variableName) const
{
    ProValueMapStack::ConstIterator vmi = m_valuemapStack.constEnd();
    for (bool first = true; ; first = false) {
        --vmi;
        ProValueMap::ConstIterator it = (*vmi).constFind(variableName);
        if (it != (*vmi).constEnd()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                break;
            return *it;
        }
        if (vmi == m_valuemapStack.constBegin())
            break;
        if (first && isFunctParam(variableName))
            break;
    }
    return ProStringList();
}

ProString QMakeEvaluator::first(const ProKey &variableName) const
{
    const ProStringList &vals = values(variableName);
    if (!vals.isEmpty())
        return vals.first();
    return ProString();
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateFile(
        const QString &fileName, QMakeHandler::EvalFileType type, LoadFlags flags)
{
    QMakeParser::ParseFlags pflags = QMakeParser::ParseUseCache;
    if (!(flags & LoadSilent))
        pflags |= QMakeParser::ParseReportMissing;
    if (ProFile *pro = m_parser->parsedProFile(fileName, pflags)) {
        m_locationStack.push(m_current);
        VisitReturn ok = visitProFile(pro, type, flags);
        m_current = m_locationStack.pop();
        pro->deref();
        if (ok == ReturnTrue && !(flags & LoadHidden)) {
            ProStringList &iif = m_valuemapStack.first()[ProKey("QMAKE_INTERNAL_INCLUDED_FILES")];
            ProString ifn(fileName);
            if (!iif.contains(ifn))
                iif << ifn;
        }
        return ok;
    } else {
        return ReturnFalse;
    }
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateFileChecked(
        const QString &fileName, QMakeHandler::EvalFileType type, LoadFlags flags)
{
    if (fileName.isEmpty())
        return ReturnFalse;
    const QMakeEvaluator *ref = this;
    do {
        for (const ProFile *pf : ref->m_profileStack)
            if (pf->fileName() == fileName) {
                evalError(fL1S("Circular inclusion of %1.").arg(fileName));
                return ReturnFalse;
            }
    } while ((ref = ref->m_caller));
    return evaluateFile(fileName, type, flags);
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateFeatureFile(
        const QString &fileName, bool silent)
{
    QString fn = fileName;
    if (!fn.endsWith(QLatin1String(".prf")))
        fn += QLatin1String(".prf");

    if (!m_featureRoots)
        updateFeaturePaths();
#ifdef PROEVALUATOR_THREAD_SAFE
    m_featureRoots->mutex.lock();
#endif
    QString currFn = currentFileName();
    if (IoUtils::fileName(currFn) != IoUtils::fileName(fn))
        currFn.clear();
    // Null values cannot regularly exist in the hash, so they indicate that the value still
    // needs to be determined. Failed lookups are represented via non-null empty strings.
    QString *fnp = &m_featureRoots->cache[qMakePair(fn, currFn)];
    if (fnp->isNull()) {
#ifdef QMAKE_OVERRIDE_PRFS
        {
            QString ovrfn(QLatin1String(":/qmake/override_features/") + fn);
            if (QFileInfo::exists(ovrfn)) {
                fn = ovrfn;
                goto cool;
            }
        }
#endif
        {
            int start_root = 0;
            const QStringList &paths = m_featureRoots->paths;
            if (!currFn.isEmpty()) {
                QStringRef currPath = IoUtils::pathName(currFn);
                for (int root = 0; root < paths.size(); ++root)
                    if (currPath == paths.at(root)) {
                        start_root = root + 1;
                        break;
                    }
            }
            for (int root = start_root; root < paths.size(); ++root) {
                QString fname = paths.at(root) + fn;
                if (IoUtils::exists(fname)) {
                    fn = fname;
                    goto cool;
                }
            }
        }
#ifdef QMAKE_BUILTIN_PRFS
        fn.prepend(QLatin1String(":/qmake/features/"));
        if (QFileInfo::exists(fn))
            goto cool;
#endif
        fn = QLatin1String(""); // Indicate failed lookup. See comment above.

      cool:
        *fnp = fn;
    } else {
        fn = *fnp;
    }
#ifdef PROEVALUATOR_THREAD_SAFE
    m_featureRoots->mutex.unlock();
#endif
    if (fn.isEmpty()) {
        if (!silent)
            evalError(fL1S("Cannot find feature %1").arg(fileName));
        return ReturnFalse;
    }
    ProStringList &already = valuesRef(ProKey("QMAKE_INTERNAL_INCLUDED_FEATURES"));
    ProString afn(fn);
    if (already.contains(afn)) {
        if (!silent)
            languageWarning(fL1S("Feature %1 already included").arg(fileName));
        return ReturnTrue;
    }
    already.append(afn);

#ifdef PROEVALUATOR_CUMULATIVE
    bool cumulative = m_cumulative;
    m_cumulative = false;
#endif

    // The path is fully normalized already.
    VisitReturn ok = evaluateFile(fn, QMakeHandler::EvalFeatureFile, LoadProOnly);

#ifdef PROEVALUATOR_CUMULATIVE
    m_cumulative = cumulative;
#endif
    return ok;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateFileInto(
        const QString &fileName, ProValueMap *values, LoadFlags flags)
{
    QMakeEvaluator visitor(m_option, m_parser, m_vfs, m_handler);
    visitor.m_caller = this;
    visitor.m_outputDir = m_outputDir;
    visitor.m_featureRoots = m_featureRoots;
    VisitReturn ret = visitor.evaluateFileChecked(fileName, QMakeHandler::EvalAuxFile, flags);
    if (ret != ReturnTrue)
        return ret;
    *values = visitor.m_valuemapStack.top();
    ProKey qiif("QMAKE_INTERNAL_INCLUDED_FILES");
    ProStringList &iif = m_valuemapStack.first()[qiif];
    const auto ifns = values->value(qiif);
    for (const ProString &ifn : ifns)
        if (!iif.contains(ifn))
            iif << ifn;
    return ReturnTrue;
}

void QMakeEvaluator::message(int type, const QString &msg) const
{
    if (!m_skipLevel)
        m_handler->message(type | (m_cumulative ? QMakeHandler::CumulativeEvalMessage : 0), msg,
                m_current.line ? m_current.pro->fileName() : QString(),
                m_current.line != 0xffff ? m_current.line : -1);
}

#ifdef PROEVALUATOR_DEBUG
void QMakeEvaluator::debugMsgInternal(int level, const char *fmt, ...) const
{
    va_list ap;

    if (level <= m_debugLevel) {
        fprintf(stderr, "DEBUG %d: ", level);
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fputc('\n', stderr);
    }
}

void QMakeEvaluator::traceMsgInternal(const char *fmt, ...) const
{
    va_list ap;

    if (!m_current.pro)
        fprintf(stderr, "DEBUG 1: ");
    else if (m_current.line <= 0)
        fprintf(stderr, "DEBUG 1: %s: ", qPrintable(m_current.pro->fileName()));
    else
        fprintf(stderr, "DEBUG 1: %s:%d: ", qPrintable(m_current.pro->fileName()), m_current.line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

QString QMakeEvaluator::formatValue(const ProString &val, bool forceQuote)
{
    QString ret;
    ret.reserve(val.size() + 2);
    const QChar *chars = val.constData();
    bool quote = forceQuote || val.isEmpty();
    for (int i = 0, l = val.size(); i < l; i++) {
        QChar c = chars[i];
        ushort uc = c.unicode();
        if (uc < 32) {
            switch (uc) {
            case '\r':
                ret += QLatin1String("\\r");
                break;
            case '\n':
                ret += QLatin1String("\\n");
                break;
            case '\t':
                ret += QLatin1String("\\t");
                break;
            default:
                ret += QString::fromLatin1("\\x%1").arg(uc, 2, 16, QLatin1Char('0'));
                break;
            }
        } else {
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
            case 32:
                quote = true;
                // fallthrough
            default:
                ret += c;
                break;
            }
        }
    }
    if (quote) {
        ret.prepend(QLatin1Char('"'));
        ret.append(QLatin1Char('"'));
    }
    return ret;
}

QString QMakeEvaluator::formatValueList(const ProStringList &vals, bool commas)
{
    QString ret;

    for (const ProString &str : vals) {
        if (!ret.isEmpty()) {
            if (commas)
                ret += QLatin1Char(',');
            ret += QLatin1Char(' ');
        }
        ret += formatValue(str);
    }
    return ret;
}

QString QMakeEvaluator::formatValueListList(const QList<ProStringList> &lists)
{
    QString ret;

    for (const ProStringList &list : lists) {
        if (!ret.isEmpty())
            ret += QLatin1String(", ");
        ret += formatValueList(list);
    }
    return ret;
}
#endif

QT_END_NAMESPACE
