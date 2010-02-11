/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "profileevaluator.h"
#include "proitems.h"
#include "ioutils.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>

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

static void refFunctions(QHash<QString, ProBlock *> *defs)
{
    foreach (ProBlock *itm, *defs)
        itm->ref();
}

static void clearFunctions(QHash<QString, ProBlock *> *defs)
{
    foreach (ProBlock *itm, *defs)
        itm->deref();
    defs->clear();
}

static void clearFunctions(ProFileEvaluator::FunctionDefs *defs)
{
    clearFunctions(&defs->replaceFunctions);
    clearFunctions(&defs->testFunctions);
}


///////////////////////////////////////////////////////////////////////
//
// ProFileCache
//
///////////////////////////////////////////////////////////////////////

ProFileCache::~ProFileCache()
{
    foreach (ProFile *pro, parsed_files)
        pro->deref();
}

void ProFileCache::discardFile(const QString &fileName)
{
    QHash<QString, ProFile *>::Iterator it = parsed_files.find(fileName);
    if (it != parsed_files.end()) {
        it.value()->deref();
        parsed_files.erase(it);
    }
}

void ProFileCache::discardFiles(const QString &prefix)
{
    QHash<QString, ProFile *>::Iterator
            it = parsed_files.begin(),
            end = parsed_files.end();
    while (it != end)
        if (it.key().startsWith(prefix)) {
            it.value()->deref();
            it = parsed_files.erase(it);
        } else {
            ++it;
        }
}

void ProFileCache::addFile(ProFile *pro)
{
    parsed_files[pro->fileName()] = pro;
    pro->ref();
}

ProFile *ProFileCache::getFile(const QString &fileName)
{
    ProFile *pro = parsed_files.value(fileName);
    if (pro)
        pro->ref();
    return pro;
}

///////////////////////////////////////////////////////////////////////
//
// ProFileOption
//
///////////////////////////////////////////////////////////////////////

ProFileOption::ProFileOption()
{
#ifdef Q_OS_WIN
    dirlist_sep = QLatin1Char(';');
    dir_sep = QLatin1Char('\\');
#else
    dirlist_sep = QLatin1Char(':');
    dir_sep = QLatin1Char('/');
#endif
    qmakespec = QString::fromLatin1(qgetenv("QMAKESPEC").data());

#if defined(Q_OS_WIN32)
    target_mode = TARG_WIN_MODE;
#elif defined(Q_OS_MAC)
    target_mode = TARG_MACX_MODE;
#elif defined(Q_OS_QNX6)
    target_mode = TARG_QNX6_MODE;
#else
    target_mode = TARG_UNIX_MODE;
#endif

    cache = 0;
}

ProFileOption::~ProFileOption()
{
    clearFunctions(&base_functions);
}

///////////////////////////////////////////////////////////////////////
//
// ProFileEvaluator::Private
//
///////////////////////////////////////////////////////////////////////

class ProFileEvaluator::Private
{
public:
    static void initStatics();
    Private(ProFileEvaluator *q_, ProFileOption *option);
    ~Private();

    ProFileEvaluator *q;
    int m_lineNo;                                   // Error reporting
    bool m_verbose;

    /////////////// Reading pro file

    struct BlockCursor {
        BlockCursor() : cursor(0) {}
        BlockCursor(ProBlock *blk) : block(blk), cursor(blk->itemsRef()) {}
        ~BlockCursor() { if (cursor) *cursor = 0; }
        void set(ProBlock *blk) { if (cursor) *cursor = 0; block = blk; cursor = blk->itemsRef(); }
        void reset() { if (cursor) { *cursor = 0; cursor = 0; } }
        void append(ProItem *item) { *cursor = item; cursor = item->nextRef(); }
        bool isValid() const { return cursor != 0; }
        ProBlock *block;
        ProItem **cursor;
    };

    bool read(ProFile *pro);
    bool read(ProBlock *pro, const QString &content);
    bool readInternal(ProBlock *pro, const QString &content, ushort *buf);

    BlockCursor &currentBlock();
    void updateItem(ushort *uc, ushort *ptr);
    ProVariable *startVariable(ushort *uc, ushort *ptr);
    void finalizeVariable(ProVariable *var, ushort *uc, ushort *ptr);
    void insertOperator(const char op);
    void enterScope(bool multiLine);
    void leaveScope();
    void finalizeBlock();

    QStack<BlockCursor> m_blockstack;
    BlockCursor m_block;

    /////////////// Evaluating pro file contents

    ProItem::ProItemReturn visitProFile(ProFile *pro);
    ProItem::ProItemReturn visitProBlock(ProBlock *block);
    ProItem::ProItemReturn visitProItem(ProItem *item);
    ProItem::ProItemReturn visitProLoopIteration();
    void visitProLoopCleanup();
    void visitProVariable(ProVariable *variable);
    ProItem::ProItemReturn visitProFunction(ProFunction *function);
    void visitProOperator(ProOperator *oper);
    void visitProCondition(ProCondition *condition);

    QHash<QString, QStringList> *findValues(const QString &variableName,
                                            QHash<QString, QStringList>::Iterator *it);
    QStringList &valuesRef(const QString &variableName);
    QStringList valuesDirect(const QString &variableName) const;
    QStringList values(const QString &variableName) const;
    QString propertyValue(const QString &val, bool complain = true) const;

    static QStringList split_value_list(const QString &vals);
    bool isActiveConfig(const QString &config, bool regex = false);
    QStringList expandVariableReferences(const QString &value, bool do_semicolon = false,
                                         int *pos = 0);
    void doVariableReplace(QString *str);
    QStringList evaluateExpandFunction(const QString &function, const QString &arguments);
    QString format(const char *format) const;
    void logMessage(const QString &msg) const;
    void errorMessage(const QString &msg) const;
    void fileMessage(const QString &msg) const;

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;
    QString resolvePath(const QString &fileName) const
        { return IoUtils::resolvePath(currentDirectory(), fileName); }

    ProItem::ProItemReturn evaluateConditionalFunction(const QString &function, const QString &arguments);
    ProFile *parsedProFile(const QString &fileName, bool cache,
                           const QString &contents = QString());
    bool evaluateFile(const QString &fileName);
    bool evaluateFeatureFile(const QString &fileName,
                             QHash<QString, QStringList> *values = 0, FunctionDefs *defs = 0);
    bool evaluateFileInto(const QString &fileName,
                          QHash<QString, QStringList> *values, FunctionDefs *defs);

    static inline ProItem::ProItemReturn returnBool(bool b)
        { return b ? ProItem::ReturnTrue : ProItem::ReturnFalse; }

    QList<QStringList> prepareFunctionArgs(const QString &arguments);
    QStringList evaluateFunction(ProBlock *funcPtr, const QList<QStringList> &argumentsList, bool *ok);

    QStringList qmakeMkspecPaths() const;
    QStringList qmakeFeaturePaths() const;

    struct State {
        bool condition;
        bool prevCondition;
    } m_sts;
    bool m_invertNext; // Short-lived, so not in State
    bool m_hadCondition; // Nested calls set it on return, so no need for it to be in State
    int m_skipLevel;
    bool m_cumulative;
    QStack<ProFile*> m_profileStack;                // To handle 'include(a.pri), so we can track back to 'a.pro' when finished with 'a.pri'
    struct ProLoop {
        QString variable;
        QStringList oldVarVal;
        QStringList list;
        int index;
        bool infinite;
    };
    QStack<ProLoop> m_loopStack;

    QString m_outputDir;

    int m_listCount;
    bool m_definingTest;
    QString m_definingFunc;
    FunctionDefs m_functionDefs;
    QStringList m_returnValue;
    QStack<QHash<QString, QStringList> > m_valuemapStack;         // VariableName must be us-ascii, the content however can be non-us-ascii.
    QHash<const ProFile*, QHash<QString, QStringList> > m_filevaluemap; // Variables per include file

    QStringList m_addUserConfigCmdArgs;
    QStringList m_removeUserConfigCmdArgs;
    bool m_parsePreAndPostFiles;

    ProFileOption *m_option;

    enum ExpandFunc {
        E_MEMBER=1, E_FIRST, E_LAST, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
        E_SPRINTF, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
        E_FIND, E_SYSTEM, E_UNIQUE, E_QUOTE, E_ESCAPE_EXPAND,
        E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE,
        E_REPLACE
    };

    enum TestFunc {
        T_REQUIRES=1, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
        T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
        T_RETURN, T_BREAK, T_NEXT, T_DEFINED, T_CONTAINS, T_INFILE,
        T_COUNT, T_ISEMPTY, T_INCLUDE, T_LOAD, T_DEBUG, T_MESSAGE, T_IF,
        T_FOR, T_DEFINE_TEST, T_DEFINE_REPLACE
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

#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::State, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::ProLoop, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::BlockCursor, Q_MOVABLE_TYPE);
#endif

static struct {
    QString field_sep;
    QString deppath;
    QString incpath;
    QString strelse;
    QString strtrue;
    QString strfalse;
    QString strunix;
    QString strmacx;
    QString strqnx6;
    QString strmac9;
    QString strmac;
    QString strwin32;
    QString strCONFIG;
    QString strARGS;
    QString strDot;
    QString strDotDot;
    QString strever;
    QString strforever;
    QString strTEMPLATE;
    QString strQMAKE_DIR_SEP;
    QHash<QString, int> expands;
    QHash<QString, int> functions;
    QHash<QString, int> varList;
    QRegExp reg_variableName;
    QStringList fakeValue;
} statics;

void ProFileEvaluator::Private::initStatics()
{
    if (!statics.field_sep.isNull())
        return;

    statics.field_sep = QLatin1String(" ");
    statics.deppath = QLatin1String("DEPENDPATH");
    statics.incpath = QLatin1String("INCLUDEPATH");
    statics.strelse = QLatin1String("else");
    statics.strtrue = QLatin1String("true");
    statics.strfalse = QLatin1String("false");
    statics.strunix = QLatin1String("unix");
    statics.strmacx = QLatin1String("macx");
    statics.strqnx6 = QLatin1String("qnx6");
    statics.strmac9 = QLatin1String("mac9");
    statics.strmac = QLatin1String("mac");
    statics.strwin32 = QLatin1String("win32");
    statics.strCONFIG = QLatin1String("CONFIG");
    statics.strARGS = QLatin1String("ARGS");
    statics.strDot = QLatin1String(".");
    statics.strDotDot = QLatin1String("..");
    statics.strever = QLatin1String("ever");
    statics.strforever = QLatin1String("forever");
    statics.strTEMPLATE = QLatin1String("TEMPLATE");
    statics.strQMAKE_DIR_SEP = QLatin1String("QMAKE_DIR_SEP");

    statics.reg_variableName.setPattern(QLatin1String("\\$\\(.*\\)"));
    statics.reg_variableName.setMinimal(true);

    statics.fakeValue.detach(); // It has to have a unique begin() value

    static const struct {
        const char * const name;
        const ExpandFunc func;
    } expandInits[] = {
        { "member", E_MEMBER },
        { "first", E_FIRST },
        { "last", E_LAST },
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
        { "replace", E_REPLACE }
    };
    for (unsigned i = 0; i < sizeof(expandInits)/sizeof(expandInits[0]); ++i)
        statics.expands.insert(QLatin1String(expandInits[i].name), expandInits[i].func);

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
        { "for", T_FOR },
        { "defineTest", T_DEFINE_TEST },
        { "defineReplace", T_DEFINE_REPLACE }
    };
    for (unsigned i = 0; i < sizeof(testInits)/sizeof(testInits[0]); ++i)
        statics.functions.insert(QLatin1String(testInits[i].name), testInits[i].func);

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
        statics.varList.insert(QLatin1String(names[i]), i);
}


ProFileEvaluator::Private::Private(ProFileEvaluator *q_, ProFileOption *option)
  : q(q_), m_option(option)
{
    // So that single-threaded apps don't have to call initialize() for now.
    initStatics();

    // Configuration, more or less
    m_verbose = true;
    m_cumulative = true;
    m_parsePreAndPostFiles = true;

    // Evaluator state
    m_sts.condition = false;
    m_sts.prevCondition = false;
    m_invertNext = false;
    m_skipLevel = 0;
    m_definingFunc.clear();
    m_listCount = 0;
    m_valuemapStack.push(QHash<QString, QStringList>());
}

ProFileEvaluator::Private::~Private()
{
    clearFunctions(&m_functionDefs);
}

////////// Parser ///////////

bool ProFileEvaluator::Private::read(ProFile *pro)
{
    QFile file(pro->fileName());
    if (!file.open(QIODevice::ReadOnly)) {
        if (IoUtils::exists(pro->fileName()))
            errorMessage(format("%1 not readable.").arg(pro->fileName()));
        return false;
    }

    QString content(QString::fromLatin1(file.readAll())); // yes, really latin1
    file.close();
    m_lineNo = 1;
    return readInternal(pro, content, (ushort*)content.data());
}

bool ProFileEvaluator::Private::read(ProBlock *pro, const QString &content)
{
    QString buf;
    buf.reserve(content.size());
    m_lineNo = 1;
    return readInternal(pro, content, (ushort*)buf.data());
}

// We know that the buffer cannot grow larger than the input string,
// and the read() functions rely on it.
bool ProFileEvaluator::Private::readInternal(ProBlock *pro, const QString &in, ushort *buf)
{
    // Parser state
    m_blockstack.push(BlockCursor(pro));

    // We rely on QStrings being null-terminated, so don't maintain a global end pointer.
    const ushort *cur = (const ushort *)in.unicode();
  freshLine:
    ProVariable *currAssignment = 0;
    ushort *ptr = buf;
    int parens = 0;
    bool inError = false;
    bool putSpace = false;
    ushort quote = 0;
    forever {
        ushort c;

        // First, skip leading whitespace
        for (;; ++cur) {
            c = *cur;
            if (c == '\n' || !c) { // Entirely empty line (sans whitespace)
                if (currAssignment) {
                    finalizeVariable(currAssignment, buf, ptr);
                    currAssignment = 0;
                } else {
                    updateItem(buf, ptr);
                }
                finalizeBlock();
                if (c) {
                    ++cur;
                    ++m_lineNo;
                    goto freshLine;
                }
                m_block.reset();
                m_blockstack.clear(); // FIXME: should actually check the state here
                return true;
            }
            if (c != ' ' && c != '\t' && c != '\r')
                break;
        }

        // Then strip comments. Yep - no escaping is possible.
        const ushort *end; // End of this line
        const ushort *cptr; // Start of next line
        for (cptr = cur;; ++cptr) {
            c = *cptr;
            if (c == '#') {
                for (end = cptr; (c = *++cptr);) {
                    if (c == '\n') {
                        ++cptr;
                        break;
                    }
                }
                if (end == cur) { // Line with only a comment (sans whitespace)
                    // Qmake bizarreness: such lines do not affect line continuations
                    goto ignore;
                }
                break;
            }
            if (!c) {
                end = cptr;
                break;
            }
            if (c == '\n') {
                end = cptr++;
                break;
            }
        }

        // Then look for line continuations. Yep - no escaping here as well.
        bool lineCont;
        forever {
            // We don't have to check for underrun here, as we already determined
            // that the line is non-empty.
            ushort ec = *(end - 1);
            if (ec == '\\') {
                --end;
                lineCont = true;
                break;
            }
            if (ec != ' ' && ec != '\t' && ec != '\r') {
                lineCont = false;
                break;
            }
            --end;
        }

        if (!inError) {
            // Finally, do the tokenization
            if (!currAssignment) {
              newItem:
                do {
                    if (cur == end)
                        goto lineEnd;
                    c = *cur++;
                } while (c == ' ' || c == '\t');
                forever {
                    if (c == '"') {
                        quote = '"' - quote;
                    } else if (!quote) {
                        if (c == '(') {
                            ++parens;
                        } else if (c == ')') {
                            --parens;
                        } else if (!parens) {
                            if (c == ':') {
                                updateItem(buf, ptr);
                                enterScope(false);
                              nextItem:
                                ptr = buf;
                                putSpace = false;
                                goto newItem;
                            }
                            if (c == '{') {
                                updateItem(buf, ptr);
                                enterScope(true);
                                goto nextItem;
                            }
                            if (c == '}') {
                                updateItem(buf, ptr);
                                leaveScope();
                                goto nextItem;
                            }
                            if (c == '=') {
                                if ((currAssignment = startVariable(buf, ptr))) {
                                    ptr = buf;
                                    putSpace = false;
                                    break;
                                }
                                ptr = buf;
                                inError = true;
                                goto skip;
                            }
                            if (c == '|' || c == '!') {
                                updateItem(buf, ptr);
                                insertOperator(c);
                                goto nextItem;
                            }
                        }
                    }

                    if (putSpace) {
                        putSpace = false;
                        *ptr++ = ' ';
                    }
                    *ptr++ = c;

                    forever {
                        if (cur == end)
                            goto lineEnd;
                        c = *cur++;
                        if (c != ' ' && c != '\t')
                            break;
                        putSpace = true;
                    }
                }
            } // !currAssignment

            do {
                if (cur == end)
                    goto lineEnd;
                c = *cur++;
            } while (c == ' ' || c == '\t');
            forever {
                if (putSpace) {
                    putSpace = false;
                    *ptr++ = ' ';
                }
                *ptr++ = c;

                forever {
                    if (cur == end)
                        goto lineEnd;
                    c = *cur++;
                    if (c != ' ' && c != '\t')
                        break;
                    putSpace = true;
                }
            }
          lineEnd:
            if (lineCont) {
                putSpace = (ptr != buf);
            } else {
                if (currAssignment) {
                    finalizeVariable(currAssignment, buf, ptr);
                    currAssignment = 0;
                } else {
                    updateItem(buf, ptr);
                }
                ptr = buf;
                putSpace = false;
            }
        } // !inError
      skip:
        if (!lineCont) {
            finalizeBlock();
            cur = cptr;
            ++m_lineNo;
            goto freshLine;
        }
      ignore:
        cur = cptr;
        ++m_lineNo;
    }
}

void ProFileEvaluator::Private::finalizeBlock()
{
    if (m_blockstack.top().block->blockKind() & ProBlock::SingleLine)
        leaveScope();
    m_block.reset();
}

ProVariable *ProFileEvaluator::Private::startVariable(ushort *uc, ushort *ptr)
{
    ProVariable::VariableOperator opkind;

    if (ptr == uc) // Line starting with '=', like a conflict marker
        return 0;

    switch (*(ptr - 1)) {
        case '+':
            --ptr;
            opkind = ProVariable::AddOperator;
            break;
        case '-':
            --ptr;
            opkind = ProVariable::RemoveOperator;
            break;
        case '*':
            --ptr;
            opkind = ProVariable::UniqueAddOperator;
            break;
        case '~':
            --ptr;
            opkind = ProVariable::ReplaceOperator;
            break;
        default:
            opkind = ProVariable::SetOperator;
            goto skipTrunc;
    }

    if (ptr == uc) // Line starting with manipulation operator
        return 0;
    if (*(ptr - 1) == ' ')
        --ptr;

  skipTrunc:
    ProVariable *variable = new ProVariable(QString((QChar*)uc, ptr - uc));
    variable->setLineNumber(m_lineNo);
    variable->setVariableOperator(opkind);
    return variable;
}

void ProFileEvaluator::Private::finalizeVariable(ProVariable *variable, ushort *uc, ushort *ptr)
{
    variable->setValue(QString((QChar*)uc, ptr - uc));

    m_blockstack.top().append(variable);
}

void ProFileEvaluator::Private::insertOperator(const char op)
{
    ProOperator::OperatorKind opkind;
    switch (op) {
        case '!':
            opkind = ProOperator::NotOperator;
            break;
        case '|':
            opkind = ProOperator::OrOperator;
            break;
        default:
            opkind = ProOperator::OrOperator;
    }

    ProOperator * const proOp = new ProOperator(opkind);
    proOp->setLineNumber(m_lineNo);
    currentBlock().append(proOp);
}

void ProFileEvaluator::Private::enterScope(bool multiLine)
{
    BlockCursor &parent = currentBlock();

    ProBlock *block = new ProBlock();
    block->setLineNumber(m_lineNo);
    if (multiLine)
        block->setBlockKind(ProBlock::ScopeContentsKind);
    else
        block->setBlockKind(ProBlock::ScopeContentsKind|ProBlock::SingleLine);
    m_blockstack.push(BlockCursor(block));

    parent.block->setBlockKind(ProBlock::ScopeKind);
    parent.append(block);

    m_block.reset();
}

void ProFileEvaluator::Private::leaveScope()
{
    if (m_blockstack.count() == 1)
        errorMessage(format("Excess closing brace."));
    else
        m_blockstack.pop();
    finalizeBlock();
}

ProFileEvaluator::Private::BlockCursor &ProFileEvaluator::Private::currentBlock()
{
    if (!m_block.isValid()) {
        ProBlock *blk = new ProBlock();
        blk->setLineNumber(m_lineNo);
        m_blockstack.top().append(blk);
        m_block.set(blk);
    }
    return m_block;
}

void ProFileEvaluator::Private::updateItem(ushort *uc, ushort *ptr)
{
    if (ptr == uc)
        return;

    QString proItem = QString((QChar*)uc, ptr - uc);

    ProItem *item;
    if (proItem.endsWith(QLatin1Char(')'))) {
        item = new ProFunction(proItem);
    } else {
        item = new ProCondition(proItem);
    }
    item->setLineNumber(m_lineNo);
    currentBlock().append(item);
}

//////// Evaluator tools /////////

QStringList ProFileEvaluator::Private::split_value_list(const QString &vals)
{
    QString build;
    QStringList ret;
    QStack<char> quote;

    const ushort SPACE = ' ';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';
    const ushort BACKSLASH = '\\';

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
            ret << build;
            build.clear();
        } else {
            build += vals_data[x];
        }
    }
    if (!build.isEmpty())
        ret << build;
    return ret;
}

static void insertUnique(QStringList *varlist, const QStringList &value)
{
    foreach (const QString &str, value)
        if (!varlist->contains(str))
            varlist->append(str);
}

static void removeEach(QStringList *varlist, const QStringList &value)
{
    foreach (const QString &str, value)
        varlist->removeAll(str);
}

static void replaceInList(QStringList *varlist,
        const QRegExp &regexp, const QString &replace, bool global)
{
    for (QStringList::Iterator varit = varlist->begin(); varit != varlist->end(); ) {
        if ((*varit).contains(regexp)) {
            (*varit).replace(regexp, replace);
            if ((*varit).isEmpty())
                varit = varlist->erase(varit);
            else
                ++varit;
            if (!global)
                break;
        } else {
            ++varit;
        }
    }
}

static QString expandEnvVars(const QString &str)
{
    QString string = str;
    int rep;
    QRegExp reg_variableName = statics.reg_variableName; // Copy for thread safety
    while ((rep = reg_variableName.indexIn(string)) != -1)
        string.replace(rep, reg_variableName.matchedLength(),
                       QString::fromLocal8Bit(qgetenv(string.mid(rep + 2, reg_variableName.matchedLength() - 3).toLatin1().constData()).constData()));
    return string;
}

static QStringList expandEnvVars(const QStringList &x)
{
    QStringList ret;
    foreach (const QString &str, x)
        ret << expandEnvVars(str);
    return ret;
}

// This is braindead, but we want qmake compat
static QString fixPathToLocalOS(const QString &str)
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

ProItem::ProItemReturn ProFileEvaluator::Private::visitProItem(ProItem *item)
{
    switch (item->kind()) {
    case ProItem::BlockKind: // This is never a ProFile
        return visitProBlock(static_cast<ProBlock*>(item));
    case ProItem::VariableKind:
        visitProVariable(static_cast<ProVariable*>(item));
        break;
    case ProItem::ConditionKind:
        visitProCondition(static_cast<ProCondition*>(item));
        break;
    case ProItem::FunctionKind:
        return visitProFunction(static_cast<ProFunction*>(item));
    case ProItem::OperatorKind:
        visitProOperator(static_cast<ProOperator*>(item));
        break;
    }
    return ProItem::ReturnTrue;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitProBlock(ProBlock *block)
{
    if (block->blockKind() & ProBlock::ScopeContentsKind) {
        if (!m_definingFunc.isEmpty()) {
            if (!m_skipLevel || m_cumulative) {
                QHash<QString, ProBlock *> *hash =
                        (m_definingTest ? &m_functionDefs.testFunctions
                                        : &m_functionDefs.replaceFunctions);
                if (ProBlock *def = hash->value(m_definingFunc))
                    def->deref();
                hash->insert(m_definingFunc, block);
                block->ref();
                block->setBlockKind(block->blockKind() | ProBlock::FunctionBodyKind);
            }
            m_definingFunc.clear();
            return ProItem::ReturnTrue;
        } else if (!(block->blockKind() & ProBlock::FunctionBodyKind)) {
            if (!m_sts.condition) {
                if (m_skipLevel || m_hadCondition)
                    ++m_skipLevel;
            } else {
                Q_ASSERT(!m_skipLevel);
            }
        }
    } else {
        m_hadCondition = false;
        if (!m_skipLevel) {
            if (m_sts.condition) {
                m_sts.prevCondition = true;
                m_sts.condition = false;
            }
        } else {
            Q_ASSERT(!m_sts.condition);
        }
    }
    ProItem::ProItemReturn rt = ProItem::ReturnTrue;
    for (ProItem *item = block->items(); item; item = item->next()) {
        rt = visitProItem(item);
        if (rt != ProItem::ReturnTrue && rt != ProItem::ReturnFalse) {
            if (rt == ProItem::ReturnLoop) {
                rt = ProItem::ReturnTrue;
                while (visitProLoopIteration())
                    for (ProItem *lItem = item; (lItem = lItem->next()); ) {
                        rt = visitProItem(lItem);
                        if (rt != ProItem::ReturnTrue && rt != ProItem::ReturnFalse) {
                            if (rt == ProItem::ReturnNext) {
                                rt = ProItem::ReturnTrue;
                                break;
                            }
                            if (rt == ProItem::ReturnBreak)
                                rt = ProItem::ReturnTrue;
                            goto do_break;
                        }
                    }
              do_break:
                visitProLoopCleanup();
            }
            break;
        }
    }
    if ((block->blockKind() & ProBlock::ScopeContentsKind)
        && !(block->blockKind() & ProBlock::FunctionBodyKind)) {
        if (m_skipLevel) {
            Q_ASSERT(!m_sts.condition);
            --m_skipLevel;
        } else if (!(block->blockKind() & ProBlock::SingleLine)) {
            // Conditionals contained inside this block may have changed the state.
            // So we reset it here to make an else following us do the right thing.
            m_sts.condition = true;
        }
    }
    return rt;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitProLoopIteration()
{
    ProLoop &loop = m_loopStack.top();

    if (loop.infinite) {
        if (!loop.variable.isEmpty())
            m_valuemapStack.top()[loop.variable] = QStringList(QString::number(loop.index++));
        if (loop.index > 1000) {
            errorMessage(format("ran into infinite loop (> 1000 iterations)."));
            return ProItem::ReturnFalse;
        }
    } else {
        QString val;
        do {
            if (loop.index >= loop.list.count())
                return ProItem::ReturnFalse;
            val = loop.list.at(loop.index++);
        } while (val.isEmpty()); // stupid, but qmake is like that
        m_valuemapStack.top()[loop.variable] = QStringList(val);
    }
    return ProItem::ReturnTrue;
}

void ProFileEvaluator::Private::visitProLoopCleanup()
{
    ProLoop &loop = m_loopStack.top();
    m_valuemapStack.top()[loop.variable] = loop.oldVarVal;
    m_loopStack.pop_back();
}

void ProFileEvaluator::Private::visitProVariable(ProVariable *var)
{
    m_lineNo = var->lineNumber();
    const QString &varName = var->variable();

    if (var->variableOperator() == ProVariable::ReplaceOperator) {      // ~=
        // DEFINES ~= s/a/b/?[gqi]

        QString val = var->value();
        doVariableReplace(&val);
        if (val.length() < 4 || val.at(0) != QLatin1Char('s')) {
            logMessage(format("the ~= operator can handle only the s/// function."));
            return;
        }
        QChar sep = val.at(1);
        QStringList func = val.split(sep);
        if (func.count() < 3 || func.count() > 4) {
            logMessage(format("the s/// function expects 3 or 4 arguments."));
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
            replaceInList(&valuesRef(varName), regexp, replace, global);
            replaceInList(&m_filevaluemap[currentProFile()][varName], regexp, replace, global);
        }
    } else {
        bool doSemicolon = (varName == statics.deppath || varName == statics.incpath);
        QStringList varVal = expandVariableReferences(var->value(), doSemicolon);

        switch (var->variableOperator()) {
        default: // ReplaceOperator - cannot happen
        case ProVariable::SetOperator:          // =
            if (!m_cumulative) {
                if (!m_skipLevel) {
                    m_valuemapStack.top()[varName] = varVal;
                    m_filevaluemap[currentProFile()][varName] = varVal;
                }
            } else {
                // We are greedy for values.
                valuesRef(varName) += varVal;
                m_filevaluemap[currentProFile()][varName] += varVal;
            }
            break;
        case ProVariable::UniqueAddOperator:    // *=
            if (!m_skipLevel || m_cumulative) {
                insertUnique(&valuesRef(varName), varVal);
                insertUnique(&m_filevaluemap[currentProFile()][varName], varVal);
            }
            break;
        case ProVariable::AddOperator:          // +=
            if (!m_skipLevel || m_cumulative) {
                valuesRef(varName) += varVal;
                m_filevaluemap[currentProFile()][varName] += varVal;
            }
            break;
        case ProVariable::RemoveOperator:       // -=
            if (!m_cumulative) {
                if (!m_skipLevel) {
                    removeEach(&valuesRef(varName), varVal);
                    removeEach(&m_filevaluemap[currentProFile()][varName], varVal);
                }
            } else {
                // We are stingy with our values, too.
            }
            break;
        }
    }
}

void ProFileEvaluator::Private::visitProOperator(ProOperator *oper)
{
    m_invertNext = (oper->operatorKind() == ProOperator::NotOperator);
}

void ProFileEvaluator::Private::visitProCondition(ProCondition *cond)
{
    if (!m_skipLevel) {
        m_hadCondition = true;
        if (!cond->text().compare(statics.strelse, Qt::CaseInsensitive)) {
            m_sts.condition = !m_sts.prevCondition;
        } else {
            m_sts.prevCondition = false;
            if (!m_sts.condition && isActiveConfig(cond->text(), true) ^ m_invertNext)
                m_sts.condition = true;
        }
    }
    m_invertNext = false;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitProFile(ProFile *pro)
{
    m_lineNo = pro->lineNumber();

    m_profileStack.push(pro);
    if (m_profileStack.count() == 1) {
        // Do this only for the initial profile we visit, since
        // that is *the* profile. All the other times we reach this function will be due to
        // include(file) or load(file)

        if (m_parsePreAndPostFiles) {

            if (m_option->base_valuemap.isEmpty()) {
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
                    QHash<QString, QStringList> cache_valuemap;
                    if (evaluateFileInto(qmake_cache, &cache_valuemap, 0)) {
                        if (m_option->qmakespec.isEmpty()) {
                            const QStringList &vals = cache_valuemap.value(QLatin1String("QMAKESPEC"));
                            if (!vals.isEmpty())
                                m_option->qmakespec = vals.first();
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
                        errorMessage(format("Could not find qmake configuration directory"));
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
                        errorMessage(format("Could not find qmake configuration file"));
                        // Unlike in qmake, a missing config is not critical ...
                        qmakespec.clear();
                      cool: ;
                    }
                }

                if (!qmakespec.isEmpty()) {
                    m_option->qmakespec = QDir::cleanPath(qmakespec);

                    QString spec = m_option->qmakespec + QLatin1String("/qmake.conf");
                    if (!evaluateFileInto(spec,
                                          &m_option->base_valuemap, &m_option->base_functions)) {
                        errorMessage(format("Could not read qmake configuration file %1").arg(spec));
                    } else if (!m_option->cachefile.isEmpty()) {
                        evaluateFileInto(m_option->cachefile,
                                         &m_option->base_valuemap, &m_option->base_functions);
                    }
                    m_option->qmakespec_name = IoUtils::fileName(m_option->qmakespec).toString();
                    if (m_option->qmakespec_name == QLatin1String("default")) {
#ifdef Q_OS_UNIX
                        char buffer[1024];
                        int l = ::readlink(m_option->qmakespec.toLatin1().constData(), buffer, 1024);
                        if (l != -1)
                            m_option->qmakespec_name =
                                    IoUtils::fileName(QString::fromLatin1(buffer, l)).toString();
#else
                        // We can't resolve symlinks as they do on Unix, so configure.exe puts
                        // the source of the qmake.conf at the end of the default/qmake.conf in
                        // the QMAKESPEC_ORG variable.
                        const QStringList &spec_org =
                                m_option->base_valuemap.value(QLatin1String("QMAKESPEC_ORIGINAL"));
                        if (!spec_org.isEmpty())
                            m_option->qmakespec_name =
                                    IoUtils::fileName(spec_org.first()).toString();
#endif
                    }
                }

                evaluateFeatureFile(QLatin1String("default_pre.prf"),
                                    &m_option->base_valuemap, &m_option->base_functions);
            }

            m_valuemapStack.top() = m_option->base_valuemap;

            clearFunctions(&m_functionDefs);
            m_functionDefs = m_option->base_functions;
            refFunctions(&m_functionDefs.testFunctions);
            refFunctions(&m_functionDefs.replaceFunctions);

            QStringList &tgt = m_valuemapStack.top()[QLatin1String("TARGET")];
            if (tgt.isEmpty())
                tgt.append(QFileInfo(pro->fileName()).baseName());

            QStringList &tmp = m_valuemapStack.top()[QLatin1String("CONFIG")];
            tmp.append(m_addUserConfigCmdArgs);
            foreach (const QString &remove, m_removeUserConfigCmdArgs)
                tmp.removeAll(remove);
        }
    }

    visitProBlock(pro);

    m_lineNo = pro->lineNumber();

    if (m_profileStack.count() == 1) {
        if (m_parsePreAndPostFiles) {
            evaluateFeatureFile(QLatin1String("default_post.prf"));

            QSet<QString> processed;
            forever {
                bool finished = true;
                QStringList configs = valuesDirect(statics.strCONFIG);
                for (int i = configs.size() - 1; i >= 0; --i) {
                    const QString config = configs.at(i).toLower();
                    if (!processed.contains(config)) {
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
    }
    m_profileStack.pop();

    return ProItem::ReturnTrue;
}

ProItem::ProItemReturn ProFileEvaluator::Private::visitProFunction(ProFunction *func)
{
    // Make sure that called subblocks don't inherit & destroy the state
    bool invertThis = m_invertNext;
    m_invertNext = false;
    if (!m_skipLevel) {
        m_hadCondition = true;
        m_sts.prevCondition = false;
    }
    if (m_cumulative || !m_sts.condition) {
        QString text = func->text();
        int lparen = text.indexOf(QLatin1Char('('));
        int rparen = text.lastIndexOf(QLatin1Char(')'));
        Q_ASSERT(lparen < rparen);
        QString arguments = text.mid(lparen + 1, rparen - lparen - 1);
        QString funcName = text.left(lparen);
        m_lineNo = func->lineNumber();
        ProItem::ProItemReturn result = evaluateConditionalFunction(funcName.trimmed(), arguments);
        if (result != ProItem::ReturnFalse && result != ProItem::ReturnTrue)
            return result;
        if (!m_skipLevel && ((result == ProItem::ReturnTrue) ^ invertThis))
            m_sts.condition = true;
    }
    return ProItem::ReturnTrue;
}


QStringList ProFileEvaluator::Private::qmakeMkspecPaths() const
{
    QStringList ret;
    const QString concat = QLatin1String("/mkspecs");

    QByteArray qmakepath = qgetenv("QMAKEPATH");
    if (!qmakepath.isEmpty())
        foreach (const QString &it, QString::fromLocal8Bit(qmakepath).split(m_option->dirlist_sep))
            ret << QDir::cleanPath(it) + concat;

    QString builtIn = propertyValue(QLatin1String("QT_INSTALL_DATA")) + concat;
    if (!ret.contains(builtIn))
        ret << builtIn;

    return ret;
}

QStringList ProFileEvaluator::Private::qmakeFeaturePaths() const
{
    QString mkspecs_concat = QLatin1String("/mkspecs");
    QString features_concat = QLatin1String("/features");
    QStringList concat;
    switch (m_option->target_mode) {
    case ProFileOption::TARG_MACX_MODE:
        concat << QLatin1String("/features/mac");
        concat << QLatin1String("/features/macx");
        concat << QLatin1String("/features/unix");
        break;
    case ProFileOption::TARG_UNIX_MODE:
        concat << QLatin1String("/features/unix");
        break;
    case ProFileOption::TARG_WIN_MODE:
        concat << QLatin1String("/features/win32");
        break;
    case ProFileOption::TARG_MAC9_MODE:
        concat << QLatin1String("/features/mac");
        concat << QLatin1String("/features/mac9");
        break;
    case ProFileOption::TARG_QNX6_MODE:
        concat << QLatin1String("/features/qnx6");
        concat << QLatin1String("/features/unix");
        break;
    }
    concat << features_concat;

    QStringList feature_roots;

    QByteArray mkspec_path = qgetenv("QMAKEFEATURES");
    if (!mkspec_path.isEmpty())
        foreach (const QString &f, QString::fromLocal8Bit(mkspec_path).split(m_option->dirlist_sep))
            feature_roots += resolvePath(f);

    feature_roots += propertyValue(QLatin1String("QMAKEFEATURES"), false).split(
            m_option->dirlist_sep, QString::SkipEmptyParts);

    if (!m_option->cachefile.isEmpty()) {
        QString path = m_option->cachefile.left(m_option->cachefile.lastIndexOf((ushort)'/'));
        foreach (const QString &concat_it, concat)
            feature_roots << (path + concat_it);
    }

    QByteArray qmakepath = qgetenv("QMAKEPATH");
    if (!qmakepath.isNull()) {
        const QStringList lst = QString::fromLocal8Bit(qmakepath).split(m_option->dirlist_sep);
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
        feature_roots << (propertyValue(QLatin1String("QT_INSTALL_PREFIX")) +
                          mkspecs_concat + concat_it);
    foreach (const QString &concat_it, concat)
        feature_roots << (propertyValue(QLatin1String("QT_INSTALL_DATA")) +
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
        logMessage(format("Querying unknown property %1").arg(name));
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

void ProFileEvaluator::Private::doVariableReplace(QString *str)
{
    *str = expandVariableReferences(*str).join(statics.field_sep);
}

// Be fast even for debug builds
#ifdef __GNUC__
# define ALWAYS_INLINE __attribute__((always_inline))
#else
# define ALWAYS_INLINE
#endif

// The (QChar*)current->constData() constructs below avoid pointless detach() calls
static inline void ALWAYS_INLINE appendChar(ushort unicode,
    QString *current, QChar **ptr, QString *pending)
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

static void appendString(const QString &string,
    QString *current, QChar **ptr, QString *pending)
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
    ::memcpy(*ptr, string.data(), string.size() * 2);
    *ptr += string.size();
}

static void flushCurrent(QStringList *ret,
    QString *current, QChar **ptr, QString *pending)
{
    QChar *uc = (QChar*)current->constData();
    int len = *ptr - uc;
    if (len) {
        ret->append(QString(uc, len));
        *ptr = uc;
    } else if (!pending->isEmpty()) {
        ret->append(*pending);
        pending->clear();
    }
}

static inline void flushFinal(QStringList *ret,
    const QString &current, const QChar *ptr, const QString &pending,
    const QString &str, bool replaced)
{
    int len = ptr - current.data();
    if (len) {
        if (!replaced && len == str.size())
            ret->append(str);
        else
            ret->append(QString(current.data(), len));
    } else if (!pending.isEmpty()) {
        ret->append(pending);
    }
}

QStringList ProFileEvaluator::Private::expandVariableReferences(
        const QString &str, bool do_semicolon, int *pos)
{
    QStringList ret;
//    if (ok)
//        *ok = true;
    if (str.isEmpty())
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
    const ushort SEMICOLON = ';';
    const ushort COMMA = ',';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';

    ushort unicode, quote = 0, parens = 0;
    const ushort *str_data = (const ushort *)str.data();
    const int str_len = str.length();

    QString var, args;

    bool replaced = false;
    QString current; // Buffer for successively assembled string segments
    current.resize(str.size());
    QChar *ptr = current.data();
    QString pending; // Buffer for string segments from variables
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
                var = QString::fromRawData((QChar*)str_data + name_start, i - name_start);
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
                    args = QString((QChar*)str_data + name_start, i - name_start);
                    if (++i < str_len)
                        unicode = str_data[i];
                    else
                        unicode = 0;
                    // at this point i is pointing to the 'next' character (which is in unicode)
                    // this might actually be a term character since you can do $${func()}
                }
                if (term) {
                    if (unicode != term) {
                        logMessage(format("Missing %1 terminator [found %2]")
                            .arg(QChar(term))
                            .arg(unicode ? QString(unicode) : QString::fromLatin1(("end-of-line"))));
//                        if (ok)
//                            *ok = false;
                        if (pos)
                            *pos = str_len;
                        return QStringList();
                    }
                } else {
                    // move the 'cursor' back to the last char of the thing we were looking at
                    --i;
                }

                QStringList replacement;
                if (var_type == ENVIRON) {
                    replacement = split_value_list(QString::fromLocal8Bit(qgetenv(var.toLatin1().constData())));
                } else if (var_type == PROPERTY) {
                    replacement << propertyValue(var);
                } else if (var_type == FUNCTION) {
                    replacement << evaluateExpandFunction(var, args);
                } else if (var_type == VAR) {
                    replacement = values(var);
                }
                if (!replacement.isEmpty()) {
                    if (quote) {
                        appendString(replacement.join(statics.field_sep),
                                     &current, &ptr, &pending);
                    } else {
                        appendString(replacement.first(), &current, &ptr, &pending);
                        if (replacement.size() > 1) {
                            flushCurrent(&ret, &current, &ptr, &pending);
                            pending = replacement.last();
                            if (replacement.size() > 2) {
                                // FIXME: ret.reserve(ret.size() + replacement.size() - 2);
                                for (int i = 1; i < replacement.size() - 1; ++i)
                                    ret << replacement.at(i);
                            }
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
            } else if ((do_semicolon && unicode == SEMICOLON) ||
                       unicode == SPACE || unicode == TAB) {
                flushCurrent(&ret, &current, &ptr, &pending);
                continue;
            } else if (pos) {
                if (unicode == LPAREN) {
                    ++parens;
                } else if (unicode == RPAREN) {
                    --parens;
                } else if (!parens && unicode == COMMA) {
                    *pos = i + 1;
                    flushFinal(&ret, current, ptr, pending, str, replaced);
                    return ret;
                }
            }
        }
        appendChar(unicode, &current, &ptr, &pending);
    }
    if (pos)
        *pos = str_len;
    flushFinal(&ret, current, ptr, pending, str, replaced);
    return ret;
}

bool ProFileEvaluator::Private::isActiveConfig(const QString &config, bool regex)
{
    // magic types for easy flipping
    if (config == statics.strtrue)
        return true;
    if (config == statics.strfalse)
        return false;

    // mkspecs
    if ((m_option->target_mode == m_option->TARG_MACX_MODE
            || m_option->target_mode == m_option->TARG_QNX6_MODE
            || m_option->target_mode == m_option->TARG_UNIX_MODE)
          && config == statics.strunix)
        return true;
    if (m_option->target_mode == m_option->TARG_MACX_MODE && config == statics.strmacx)
        return true;
    if (m_option->target_mode == m_option->TARG_QNX6_MODE && config == statics.strqnx6)
        return true;
    if (m_option->target_mode == m_option->TARG_MAC9_MODE && config == statics.strmac9)
        return true;
    if ((m_option->target_mode == m_option->TARG_MAC9_MODE
            || m_option->target_mode == m_option->TARG_MACX_MODE)
          && config == statics.strmac)
        return true;
    if (m_option->target_mode == m_option->TARG_WIN_MODE && config == statics.strwin32)
        return true;

    if (regex && (config.contains(QLatin1Char('*')) || config.contains(QLatin1Char('?')))) {
        QRegExp re(config, Qt::CaseSensitive, QRegExp::Wildcard);

        if (re.exactMatch(m_option->qmakespec_name))
            return true;

        // CONFIG variable
        foreach (const QString &configValue, valuesDirect(statics.strCONFIG)) {
            if (re.exactMatch(configValue))
                return true;
        }
    } else {
        // mkspecs
        if (m_option->qmakespec_name == config)
            return true;

        // CONFIG variable
        foreach (const QString &configValue, valuesDirect(statics.strCONFIG)) {
            if (configValue == config)
                return true;
        }
    }

    return false;
}

QList<QStringList> ProFileEvaluator::Private::prepareFunctionArgs(const QString &arguments)
{
    QList<QStringList> args_list;
    for (int pos = 0; pos < arguments.length(); )
        args_list << expandVariableReferences(arguments, false, &pos);
    return args_list;
}

QStringList ProFileEvaluator::Private::evaluateFunction(
        ProBlock *funcPtr, const QList<QStringList> &argumentsList, bool *ok)
{
    bool oki;
    QStringList ret;

    if (m_valuemapStack.count() >= 100) {
        errorMessage(format("ran into infinite recursion (depth > 100)."));
        oki = false;
    } else {
        State sts = m_sts;
        m_valuemapStack.push(QHash<QString, QStringList>());

        QStringList args;
        for (int i = 0; i < argumentsList.count(); ++i) {
            args += argumentsList[i];
            m_valuemapStack.top()[QString::number(i+1)] = argumentsList[i];
        }
        m_valuemapStack.top()[statics.strARGS] = args;
        oki = (visitProBlock(funcPtr) != ProItem::ReturnFalse); // True || Return
        ret = m_returnValue;
        m_returnValue.clear();

        m_valuemapStack.pop();
        m_sts = sts;
    }
    if (ok)
        *ok = oki;
    if (oki)
        return ret;
    return QStringList();
}

QStringList ProFileEvaluator::Private::evaluateExpandFunction(const QString &func, const QString &arguments)
{
    QList<QStringList> args_list = prepareFunctionArgs(arguments);

    if (ProBlock *funcPtr = m_functionDefs.replaceFunctions.value(func, 0))
        return evaluateFunction(funcPtr, args_list, 0);

    QStringList args; //why don't the builtin functions just use args_list? --Sam
    foreach (const QStringList &arg, args_list)
        args += arg.join(statics.field_sep);

    ExpandFunc func_t = ExpandFunc(statics.expands.value(func.toLower()));

    QStringList ret;

    switch (func_t) {
        case E_BASENAME:
        case E_DIRNAME:
        case E_SECTION: {
            bool regexp = false;
            QString sep, var;
            int beg = 0;
            int end = -1;
            if (func_t == E_SECTION) {
                if (args.count() != 3 && args.count() != 4) {
                    logMessage(format("%1(var) section(var, sep, begin, end) "
                        "requires three or four arguments.").arg(func));
                } else {
                    var = args[0];
                    sep = args[1];
                    beg = args[2].toInt();
                    if (args.count() == 4)
                        end = args[3].toInt();
                }
            } else {
                if (args.count() != 1) {
                    logMessage(format("%1(var) requires one argument.").arg(func));
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
            if (!var.isNull()) {
                if (regexp) {
                    QRegExp sepRx(sep);
                    foreach (const QString &str, values(var))
                        ret += str.section(sepRx, beg, end);
                } else {
                    foreach (const QString &str, values(var))
                        ret += str.section(sep, beg, end);
                }
            }
            break;
        }
        case E_SPRINTF:
            if(args.count() < 1) {
                logMessage(format("sprintf(format, ...) requires at least one argument"));
            } else {
                QString tmp = args.at(0);
                for (int i = 1; i < args.count(); ++i)
                    tmp = tmp.arg(args.at(i));
                ret = split_value_list(tmp);
            }
            break;
        case E_JOIN: {
            if (args.count() < 1 || args.count() > 4) {
                logMessage(format("join(var, glue, before, after) requires one to four arguments."));
            } else {
                QString glue, before, after;
                if (args.count() >= 2)
                    glue = args[1];
                if (args.count() >= 3)
                    before = args[2];
                if (args.count() == 4)
                    after = args[3];
                const QStringList &var = values(args.first());
                if (!var.isEmpty())
                    ret.append(before + var.join(glue) + after);
            }
            break;
        }
        case E_SPLIT:
            if (args.count() != 2) {
                logMessage(format("split(var, sep) requires one or two arguments"));
            } else {
                const QString &sep = (args.count() == 2) ? args[1] : statics.field_sep;
                foreach (const QString &var, values(args.first()))
                    foreach (const QString &splt, var.split(sep))
                        ret.append(splt);
            }
            break;
        case E_MEMBER:
            if (args.count() < 1 || args.count() > 3) {
                logMessage(format("member(var, start, end) requires one to three arguments."));
            } else {
                bool ok = true;
                const QStringList var = values(args.first());
                int start = 0, end = 0;
                if (args.count() >= 2) {
                    QString start_str = args[1];
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
                            logMessage(format("member() argument 2 (start) '%2' invalid.")
                                .arg(start_str));
                    } else {
                        end = start;
                        if (args.count() == 3)
                            end = args[2].toInt(&ok);
                        if (!ok)
                            logMessage(format("member() argument 3 (end) '%2' invalid.\n")
                                .arg(args[2]));
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
                logMessage(format("%1(var) requires one argument.").arg(func));
            } else {
                const QStringList var = values(args.first());
                if (!var.isEmpty()) {
                    if (func_t == E_FIRST)
                        ret.append(var[0]);
                    else
                        ret.append(var.last());
                }
            }
            break;
        case E_CAT:
            if (args.count() < 1 || args.count() > 2) {
                logMessage(format("cat(file, singleline=true) requires one or two arguments."));
            } else {
                QString file = args[0];

                bool singleLine = true;
                if (args.count() > 1)
                    singleLine = (!args[1].compare(statics.strtrue, Qt::CaseInsensitive));

                QFile qfile(resolvePath(expandEnvVars(file)));
                if (qfile.open(QIODevice::ReadOnly)) {
                    QTextStream stream(&qfile);
                    while (!stream.atEnd()) {
                        ret += split_value_list(stream.readLine().trimmed());
                        if (!singleLine)
                            ret += QLatin1String("\n");
                    }
                    qfile.close();
                }
            }
            break;
        case E_FROMFILE:
            if (args.count() != 2) {
                logMessage(format("fromfile(file, variable) requires two arguments."));
            } else {
                QHash<QString, QStringList> vars;
                if (evaluateFileInto(resolvePath(expandEnvVars(args.at(0))), &vars, 0))
                    ret = vars.value(args.at(1));
            }
            break;
        case E_EVAL:
            if (args.count() != 1) {
                logMessage(format("eval(variable) requires one argument"));
            } else {
                ret += values(args.at(0));
            }
            break;
        case E_LIST: {
            QString tmp;
            tmp.sprintf(".QMAKE_INTERNAL_TMP_variableName_%d", m_listCount++);
            ret = QStringList(tmp);
            QStringList lst;
            foreach (const QString &arg, args)
                lst += split_value_list(arg);
            m_valuemapStack.top()[tmp] = lst;
            break; }
        case E_FIND:
            if (args.count() != 2) {
                logMessage(format("find(var, str) requires two arguments."));
            } else {
                QRegExp regx(args[1]);
                foreach (const QString &val, values(args.first()))
                    if (regx.indexIn(val) != -1)
                        ret += val;
            }
            break;
        case E_SYSTEM:
            if (!m_skipLevel) {
                if (args.count() < 1 || args.count() > 2) {
                    logMessage(format("system(execute) requires one or two arguments."));
                } else {
                    char buff[256];
                    FILE *proc = QT_POPEN((QLatin1String("cd ")
                                           + IoUtils::shellQuote(currentDirectory())
                                           + QLatin1String(" && ") + args[0]).toLatin1(), "r");
                    bool singleLine = true;
                    if (args.count() > 1)
                        singleLine = (!args[1].compare(QLatin1String("true"), Qt::CaseInsensitive));
                    QString output;
                    while (proc && !feof(proc)) {
                        int read_in = int(fread(buff, 1, 255, proc));
                        if (!read_in)
                            break;
                        for (int i = 0; i < read_in; i++) {
                            if ((singleLine && buff[i] == '\n') || buff[i] == '\t')
                                buff[i] = ' ';
                        }
                        buff[read_in] = '\0';
                        output += QLatin1String(buff);
                    }
                    ret += split_value_list(output);
                    if (proc)
                        QT_PCLOSE(proc);
                }
            }
            break;
        case E_UNIQUE:
            if(args.count() != 1) {
                logMessage(format("unique(var) requires one argument."));
            } else {
                ret = values(args.first());
                ret.removeDuplicates();
            }
            break;
        case E_QUOTE:
            for (int i = 0; i < args.count(); ++i)
                ret += QStringList(args.at(i));
            break;
        case E_ESCAPE_EXPAND:
            for (int i = 0; i < args.size(); ++i) {
                QChar *i_data = args[i].data();
                int i_len = args[i].length();
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
                ret.append(QString(i_data, i_len));
            }
            break;
        case E_RE_ESCAPE:
            for (int i = 0; i < args.size(); ++i)
                ret += QRegExp::escape(args[i]);
            break;
        case E_UPPER:
        case E_LOWER:
            for (int i = 0; i < args.count(); ++i)
                if (func_t == E_UPPER)
                    ret += args[i].toUpper();
                else
                    ret += args[i].toLower();
            break;
        case E_FILES:
            if (args.count() != 1 && args.count() != 2) {
                logMessage(format("files(pattern, recursive=false) requires one or two arguments"));
            } else {
                bool recursive = false;
                if (args.count() == 2)
                    recursive = (!args[1].compare(statics.strtrue, Qt::CaseInsensitive) || args[1].toInt());
                QStringList dirs;
                QString r = fixPathToLocalOS(args[0]);
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

                const QRegExp regex(r, Qt::CaseSensitive, QRegExp::Wildcard);
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
                            ret += fname;
                    }
                }
            }
            break;
        case E_REPLACE:
            if(args.count() != 3 ) {
                logMessage(format("replace(var, before, after) requires three arguments"));
            } else {
                const QRegExp before(args[1]);
                const QString after(args[2]);
                foreach (QString val, values(args.first()))
                    ret += val.replace(before, after);
            }
            break;
        case 0:
            logMessage(format("'%1' is not a recognized replace function").arg(func));
            break;
        default:
            logMessage(format("Function '%1' is not implemented").arg(func));
            break;
    }

    return ret;
}

ProItem::ProItemReturn ProFileEvaluator::Private::evaluateConditionalFunction(
        const QString &function, const QString &arguments)
{
    QList<QStringList> args_list = prepareFunctionArgs(arguments);

    if (ProBlock *funcPtr = m_functionDefs.testFunctions.value(function, 0)) {
        bool ok;
        QStringList ret = evaluateFunction(funcPtr, args_list, &ok);
        if (ok) {
            if (ret.isEmpty()) {
                return ProItem::ReturnTrue;
            } else {
                if (ret.first() != statics.strfalse) {
                    if (ret.first() == statics.strtrue) {
                        return ProItem::ReturnTrue;
                    } else {
                        bool ok;
                        int val = ret.first().toInt(&ok);
                        if (ok) {
                            if (val)
                                return ProItem::ReturnTrue;
                        } else {
                            logMessage(format("Unexpected return value from test '%1': %2")
                                          .arg(function).arg(ret.join(QLatin1String(" :: "))));
                        }
                    }
                }
            }
        }
        return ProItem::ReturnFalse;
    }

    QStringList args; //why don't the builtin functions just use args_list? --Sam
    foreach (const QStringList &arg, args_list)
        args += arg.join(statics.field_sep);

    TestFunc func_t = (TestFunc)statics.functions.value(function);

    switch (func_t) {
        case T_DEFINE_TEST:
            m_definingTest = true;
            goto defineFunc;
        case T_DEFINE_REPLACE:
            m_definingTest = false;
          defineFunc:
            if (args.count() != 1) {
                logMessage(format("%s(function) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            m_definingFunc = args.first();
            return ProItem::ReturnTrue;
        case T_DEFINED:
            if (args.count() < 1 || args.count() > 2) {
                logMessage(format("defined(function, [\"test\"|\"replace\"])"
                                     " requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            if (args.count() > 1) {
                if (args[1] == QLatin1String("test"))
                    return returnBool(m_functionDefs.testFunctions.contains(args[0]));
                else if (args[1] == QLatin1String("replace"))
                    return returnBool(m_functionDefs.replaceFunctions.contains(args[0]));
                logMessage(format("defined(function, type):"
                                     " unexpected type [%1].\n").arg(args[1]));
                return ProItem::ReturnFalse;
            }
            return returnBool(m_functionDefs.replaceFunctions.contains(args[0])
                              || m_functionDefs.testFunctions.contains(args[0]));
        case T_RETURN:
            m_returnValue = args;
            // It is "safe" to ignore returns - due to qmake brokeness
            // they cannot be used to terminate loops anyway.
            if (m_skipLevel || m_cumulative)
                return ProItem::ReturnTrue;
            if (m_valuemapStack.isEmpty()) {
                logMessage(format("unexpected return()."));
                return ProItem::ReturnFalse;
            }
            return ProItem::ReturnReturn;
        case T_EXPORT:
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnTrue;
            if (args.count() != 1) {
                logMessage(format("export(variable) requires one argument."));
                return ProItem::ReturnFalse;
            }
            for (int i = m_valuemapStack.size(); --i > 0; ) {
                QHash<QString, QStringList>::Iterator it = m_valuemapStack[i].find(args.at(0));
                if (it != m_valuemapStack.at(i).end()) {
                    if (it->constBegin() == statics.fakeValue.constBegin()) {
                        // This is stupid, but qmake doesn't propagate deletions
                        m_valuemapStack[0][args.at(0)] = QStringList();
                    } else {
                        m_valuemapStack[0][args.at(0)] = *it;
                    }
                    m_valuemapStack[i].erase(it);
                    while (--i)
                        m_valuemapStack[i].remove(args.at(0));
                    break;
                }
            }
            return ProItem::ReturnTrue;
        case T_INFILE:
            if (args.count() < 2 || args.count() > 3) {
                logMessage(format("infile(file, var, [values]) requires two or three arguments."));
            } else {
                QHash<QString, QStringList> vars;
                if (!evaluateFileInto(resolvePath(expandEnvVars(args.at(0))), &vars, 0))
                    return ProItem::ReturnFalse;
                if (args.count() == 2)
                    return returnBool(vars.contains(args.at(1)));
                QRegExp regx;
                const QString &qry = args.at(2);
                if (qry != QRegExp::escape(qry))
                    regx.setPattern(qry);
                foreach (const QString &s, vars.value(args.at(1)))
                    if ((!regx.isEmpty() && regx.exactMatch(s)) || s == qry)
                        return ProItem::ReturnTrue;
            }
            return ProItem::ReturnFalse;
#if 0
        case T_REQUIRES:
#endif
        case T_EVAL: {
                ProBlock *pro = new ProBlock();
                QString buf = args.join(QLatin1String(" "));
                if (!readInternal(pro, buf, (ushort*)buf.data())) {
                    delete pro;
                    return ProItem::ReturnFalse;
                }
                bool ret = visitProBlock(pro);
                pro->deref();
                return returnBool(ret);
            }
        case T_FOR: {
            if (m_cumulative) // This is a no-win situation, so just pretend it's no loop
                return ProItem::ReturnTrue;
            if (m_skipLevel)
                return ProItem::ReturnFalse;
            if (args.count() > 2 || args.count() < 1) {
                logMessage(format("for({var, list|var, forever|ever})"
                                     " requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            ProLoop loop;
            loop.infinite = false;
            loop.index = 0;
            QString it_list;
            if (args.count() == 1) {
                doVariableReplace(&args[0]);
                it_list = args[0];
                if (args[0] != statics.strever) {
                    logMessage(format("for({var, list|var, forever|ever})"
                                         " requires one or two arguments."));
                    return ProItem::ReturnFalse;
                }
                it_list = statics.strforever;
            } else {
                loop.variable = args[0];
                loop.oldVarVal = valuesDirect(loop.variable);
                doVariableReplace(&args[1]);
                it_list = args[1];
            }
            loop.list = valuesDirect(it_list);
            if (loop.list.isEmpty()) {
                if (it_list == statics.strforever) {
                    loop.infinite = true;
                } else {
                    int dotdot = it_list.indexOf(statics.strDotDot);
                    if (dotdot != -1) {
                        bool ok;
                        int start = it_list.left(dotdot).toInt(&ok);
                        if (ok) {
                            int end = it_list.mid(dotdot+2).toInt(&ok);
                            if (ok) {
                                if (start < end) {
                                    for (int i = start; i <= end; i++)
                                        loop.list << QString::number(i);
                                } else {
                                    for (int i = start; i >= end; i--)
                                        loop.list << QString::number(i);
                                }
                            }
                        }
                    }
                }
            }
            m_loopStack.push(loop);
            m_sts.condition = true;
            return ProItem::ReturnLoop;
        }
        case T_BREAK:
            if (m_skipLevel)
                return ProItem::ReturnFalse;
            if (!m_loopStack.isEmpty())
                return ProItem::ReturnBreak;
            // ### missing: breaking out of multiline blocks
            logMessage(format("unexpected break()."));
            return ProItem::ReturnFalse;
        case T_NEXT:
            if (m_skipLevel)
                return ProItem::ReturnFalse;
            if (!m_loopStack.isEmpty())
                return ProItem::ReturnNext;
            logMessage(format("unexpected next()."));
            return ProItem::ReturnFalse;
        case T_IF: {
            if (args.count() != 1) {
                logMessage(format("if(condition) requires one argument."));
                return ProItem::ReturnFalse;
            }
            QString cond = args.first();
            bool escaped = false; // This is more than qmake does
            bool quoted = false;
            bool ret = true;
            bool orOp = false;
            bool invert = false;
            bool isFunc = false;
            int parens = 0;
            QString test;
            test.reserve(20);
            QString args;
            args.reserve(50);
            const QChar *d = cond.unicode();
            const QChar *ed = d + cond.length();
            while (d < ed) {
                ushort c = (d++)->unicode();
                if (!escaped) {
                    if (c == '\\') {
                        escaped = true;
                        args += c; // Assume no-one quotes the test name
                        continue;
                    } else if (c == '"') {
                        quoted = !quoted;
                        args += c; // Ditto
                        continue;
                    }
                } else {
                    escaped = false;
                }
                if (quoted) {
                    args += c; // Ditto
                } else {
                    bool isOp = false;
                    if (c == '(') {
                        isFunc = true;
                        if (parens)
                            args += c;
                        ++parens;
                    } else if (c == ')') {
                        --parens;
                        if (parens)
                            args += c;
                    } else if (!parens) {
                        if (c == ':' || c == '|')
                            isOp = true;
                        else if (c == '!')
                            invert = true;
                        else
                            test += c;
                    } else {
                        args += c;
                    }
                    if (!parens && (isOp || d == ed)) {
                        // Yes, qmake doesn't shortcut evaluations here. We can't, either,
                        // as some test functions have side effects.
                        bool success;
                        if (isFunc) {
                            success = evaluateConditionalFunction(test, args);
                        } else {
                            success = isActiveConfig(test, true);
                        }
                        success ^= invert;
                        if (orOp)
                            ret |= success;
                        else
                            ret &= success;
                        orOp = (c == '|');
                        invert = false;
                        isFunc = false;
                        test.clear();
                        args.clear();
                    }
                }
            }
            return returnBool(ret);
        }
        case T_CONFIG: {
            if (args.count() < 1 || args.count() > 2) {
                logMessage(format("CONFIG(config) requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            if (args.count() == 1)
                return returnBool(isActiveConfig(args.first()));
            const QStringList mutuals = args[1].split(QLatin1Char('|'));
            const QStringList &configs = valuesDirect(statics.strCONFIG);

            for (int i = configs.size() - 1; i >= 0; i--) {
                for (int mut = 0; mut < mutuals.count(); mut++) {
                    if (configs[i] == mutuals[mut].trimmed()) {
                        return returnBool(configs[i] == args[0]);
                    }
                }
            }
            return ProItem::ReturnFalse;
        }
        case T_CONTAINS: {
            if (args.count() < 2 || args.count() > 3) {
                logMessage(format("contains(var, val) requires two or three arguments."));
                return ProItem::ReturnFalse;
            }

            const QString &qry = args.at(1);
            QRegExp regx;
            if (qry != QRegExp::escape(qry))
                regx.setPattern(qry);
            const QStringList &l = values(args.first());
            if (args.count() == 2) {
                for (int i = 0; i < l.size(); ++i) {
                    const QString val = l[i];
                    if ((!regx.isEmpty() && regx.exactMatch(val)) || val == qry)
                        return ProItem::ReturnTrue;
                }
            } else {
                const QStringList mutuals = args[2].split(QLatin1Char('|'));
                for (int i = l.size() - 1; i >= 0; i--) {
                    const QString val = l[i];
                    for (int mut = 0; mut < mutuals.count(); mut++) {
                        if (val == mutuals[mut].trimmed()) {
                            return returnBool((!regx.isEmpty() && regx.exactMatch(val))
                                              || val == qry);
                        }
                    }
                }
            }
            return ProItem::ReturnFalse;
        }
        case T_COUNT: {
            if (args.count() != 2 && args.count() != 3) {
                logMessage(format("count(var, count, op=\"equals\") requires two or three arguments."));
                return ProItem::ReturnFalse;
            }
            int cnt = values(args.first()).count();
            if (args.count() == 3) {
                QString comp = args[2];
                if (comp == QLatin1String(">") || comp == QLatin1String("greaterThan")) {
                    return returnBool(cnt > args[1].toInt());
                } else if (comp == QLatin1String(">=")) {
                    return returnBool(cnt >= args[1].toInt());
                } else if (comp == QLatin1String("<") || comp == QLatin1String("lessThan")) {
                    return returnBool(cnt < args[1].toInt());
                } else if (comp == QLatin1String("<=")) {
                    return returnBool(cnt <= args[1].toInt());
                } else if (comp == QLatin1String("equals") || comp == QLatin1String("isEqual")
                           || comp == QLatin1String("=") || comp == QLatin1String("==")) {
                    return returnBool(cnt == args[1].toInt());
                } else {
                    logMessage(format("unexpected modifier to count(%2)").arg(comp));
                    return ProItem::ReturnFalse;
                }
            }
            return returnBool(cnt == args[1].toInt());
        }
        case T_GREATERTHAN:
        case T_LESSTHAN: {
            if (args.count() != 2) {
                logMessage(format("%1(variable, value) requires two arguments.").arg(function));
                return ProItem::ReturnFalse;
            }
            QString rhs(args[1]), lhs(values(args[0]).join(statics.field_sep));
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
                logMessage(format("%1(variable, value) requires two arguments.").arg(function));
                return ProItem::ReturnFalse;
            }
            return returnBool(values(args[0]).join(statics.field_sep) == args[1]);
        case T_CLEAR: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            if (args.count() != 1) {
                logMessage(format("%1(variable) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            QHash<QString, QStringList> *hsh;
            QHash<QString, QStringList>::Iterator it;
            if (!(hsh = findValues(args.at(0), &it)))
                return ProItem::ReturnFalse;
            if (hsh == &m_valuemapStack.top())
                it->clear();
            else
                m_valuemapStack.top()[args.at(0)].clear();
            return ProItem::ReturnTrue;
        }
        case T_UNSET: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            if (args.count() != 1) {
                logMessage(format("%1(variable) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            QHash<QString, QStringList> *hsh;
            QHash<QString, QStringList>::Iterator it;
            if (!(hsh = findValues(args.at(0), &it)))
                return ProItem::ReturnFalse;
            if (m_valuemapStack.size() == 1)
                hsh->erase(it);
            else if (hsh == &m_valuemapStack.top())
                *it = statics.fakeValue;
            else
                m_valuemapStack.top()[args.at(0)] = statics.fakeValue;
            return ProItem::ReturnTrue;
        }
        case T_INCLUDE: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            QString parseInto;
            // the third optional argument to include() controls warnings
            //      and is not used here
            if ((args.count() == 2) || (args.count() == 3) ) {
                parseInto = args[1];
            } else if (args.count() != 1) {
                logMessage(format("include(file, into, silent) requires one, two or three arguments."));
                return ProItem::ReturnFalse;
            }
            QString fn = resolvePath(expandEnvVars(args.first()));
            bool ok;
            if (parseInto.isEmpty()) {
                State sts = m_sts;
                ok = evaluateFile(fn);
                m_sts = sts;
            } else {
                QHash<QString, QStringList> symbols;
                if ((ok = evaluateFileInto(fn, &symbols, 0)))
                    for (QHash<QString, QStringList>::ConstIterator it = symbols.constBegin();
                         it != symbols.constEnd(); ++it)
                        if (!it.key().startsWith(QLatin1Char('.')))
                            m_valuemapStack.top().insert(parseInto + QLatin1Char('.') + it.key(),
                                                         it.value());
            }
            return returnBool(ok);
        }
        case T_LOAD: {
            if (m_skipLevel && !m_cumulative)
                return ProItem::ReturnFalse;
            bool ignore_error = false;
            if (args.count() == 2) {
                QString sarg = args[1];
                ignore_error = (!sarg.compare(statics.strtrue, Qt::CaseInsensitive) || sarg.toInt());
            } else if (args.count() != 1) {
                logMessage(format("load(feature) requires one or two arguments."));
                return ProItem::ReturnFalse;
            }
            // XXX ignore_error unused
            return returnBool(evaluateFeatureFile(expandEnvVars(args.first())));
        }
        case T_DEBUG:
            // Yup - do nothing. Nothing is going to enable debug output anyway.
            return ProItem::ReturnFalse;
        case T_MESSAGE: {
            if (args.count() != 1) {
                logMessage(format("%1(message) requires one argument.").arg(function));
                return ProItem::ReturnFalse;
            }
            QString msg = expandEnvVars(args.first());
            fileMessage(QString::fromLatin1("Project %1: %2").arg(function.toUpper(), msg));
            // ### Consider real termination in non-cumulative mode
            return returnBool(function != QLatin1String("error"));
        }
#if 0 // Way too dangerous to enable.
        case T_SYSTEM: {
            if (args.count() != 1) {
                logMessage(format("system(exec) requires one argument."));
                ProItem::ReturnFalse;
            }
            return returnBool(system((QLatin1String("cd ")
                                      + IoUtils::shellQuote(currentDirectory())
                                      + QLatin1String(" && ") + args.first()).toLatin1().constData()) == 0);
        }
#endif
        case T_ISEMPTY: {
            if (args.count() != 1) {
                logMessage(format("isEmpty(var) requires one argument."));
                return ProItem::ReturnFalse;
            }
            QStringList sl = values(args.first());
            if (sl.count() == 0) {
                return ProItem::ReturnTrue;
            } else if (sl.count() > 0) {
                QString var = sl.first();
                if (var.isEmpty())
                    return ProItem::ReturnTrue;
            }
            return ProItem::ReturnFalse;
        }
        case T_EXISTS: {
            if (args.count() != 1) {
                logMessage(format("exists(file) requires one argument."));
                return ProItem::ReturnFalse;
            }
            QString file = resolvePath(expandEnvVars(args.first()));

            if (IoUtils::exists(file)) {
                return ProItem::ReturnTrue;
            }
            int slsh = file.lastIndexOf(QLatin1Char('/'));
            QString fn = file.mid(slsh+1);
            if (fn.contains(QLatin1Char('*')) || fn.contains(QLatin1Char('?'))) {
                QString dirstr = file.left(slsh+1);
                if (!QDir(dirstr).entryList(QStringList(fn)).isEmpty())
                    return ProItem::ReturnTrue;
            }

            return ProItem::ReturnFalse;
        }
        case 0:
            logMessage(format("'%1' is not a recognized test function").arg(function));
            return ProItem::ReturnFalse;
        default:
            logMessage(format("Function '%1' is not implemented").arg(function));
            return ProItem::ReturnFalse;
    }
}

QHash<QString, QStringList> *ProFileEvaluator::Private::findValues(
        const QString &variableName, QHash<QString, QStringList>::Iterator *rit)
{
    for (int i = m_valuemapStack.size(); --i >= 0; ) {
        QHash<QString, QStringList>::Iterator it = m_valuemapStack[i].find(variableName);
        if (it != m_valuemapStack[i].end()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                return 0;
            *rit = it;
            return &m_valuemapStack[i];
        }
    }
    return 0;
}

QStringList &ProFileEvaluator::Private::valuesRef(const QString &variableName)
{
    QHash<QString, QStringList>::Iterator it = m_valuemapStack.top().find(variableName);
    if (it != m_valuemapStack.top().end())
        return *it;
    for (int i = m_valuemapStack.size() - 1; --i >= 0; ) {
        QHash<QString, QStringList>::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            QStringList &ret = m_valuemapStack.top()[variableName];
            ret = *it;
            return ret;
        }
    }
    return m_valuemapStack.top()[variableName];
}

QStringList ProFileEvaluator::Private::valuesDirect(const QString &variableName) const
{
    for (int i = m_valuemapStack.size(); --i >= 0; ) {
        QHash<QString, QStringList>::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            if (it->constBegin() == statics.fakeValue.constBegin())
                break;
            return *it;
        }
    }
    return QStringList();
}

QStringList ProFileEvaluator::Private::values(const QString &variableName) const
{
    QHash<QString, int>::ConstIterator vli = statics.varList.find(variableName);
    if (vli != statics.varList.constEnd()) {
        int vlidx = *vli;
        QString ret;
        switch ((VarName)vlidx) {
        case V_LITERAL_WHITESPACE: ret = QLatin1String("\t"); break;
        case V_LITERAL_DOLLAR: ret = QLatin1String("$"); break;
        case V_LITERAL_HASH: ret = QLatin1String("#"); break;
        case V_OUT_PWD: //the outgoing dir
            ret = m_outputDir;
            break;
        case V_PWD: //current working dir (of _FILE_)
        case V_IN_PWD:
            ret = currentDirectory();
            break;
        case V_DIR_SEPARATOR:
            ret = m_option->dir_sep;
            break;
        case V_DIRLIST_SEPARATOR:
            ret = m_option->dirlist_sep;
            break;
        case V__LINE_: //parser line number
            ret = QString::number(m_lineNo);
            break;
        case V__FILE_: //parser file; qmake is a bit weird here
            ret = m_profileStack.size() == 1 ? currentFileName() : currentProFile()->displayFileName();
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
                    ret = QString::fromLatin1(what);
                }
            }
#endif
        }
        return QStringList(ret);
    }

    QStringList result = valuesDirect(variableName);
    if (result.isEmpty()) {
        if (variableName == statics.strTEMPLATE) {
            result.append(QLatin1String("app"));
        } else if (variableName == statics.strQMAKE_DIR_SEP) {
            result.append(m_option->dirlist_sep);
        }
    }
    return result;
}

ProFile *ProFileEvaluator::Private::parsedProFile(const QString &fileName, bool cache,
                                                  const QString &contents)
{
    ProFile *pro;
    if (!m_option->cache || !(pro = m_option->cache->getFile(fileName))) {
        pro = new ProFile(fileName);
        if (!(contents.isNull() ? read(pro) : read(pro, contents))) {
            delete pro;
            return 0;
        }
        if (m_option->cache && cache)
            m_option->cache->addFile(pro);
    }
    return pro;
}

bool ProFileEvaluator::Private::evaluateFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    foreach (const ProFile *pf, m_profileStack)
        if (pf->fileName() == fileName) {
            errorMessage(format("circular inclusion of %1").arg(fileName));
            return false;
        }
    if (ProFile *pro = parsedProFile(fileName, true)) {
        q->aboutToEval(pro);
        bool ok = (visitProFile(pro) == ProItem::ReturnTrue);
        pro->deref();
        return ok;
    } else {
        return false;
    }
}

bool ProFileEvaluator::Private::evaluateFeatureFile(
        const QString &fileName, QHash<QString, QStringList> *values, FunctionDefs *funcs)
{
    QString fn = fileName;
    if (!fn.endsWith(QLatin1String(".prf")))
        fn += QLatin1String(".prf");

    if (!fileName.contains((ushort)'/') || !IoUtils::exists(resolvePath(fn))) {
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
        QStringList &already = valuesRef(QLatin1String("QMAKE_INTERNAL_INCLUDED_FEATURES"));
        if (already.contains(fn))
            return true;
        already.append(fn);
    } else {
        fn = resolvePath(fn);
    }

    if (values) {
        return evaluateFileInto(fn, values, funcs);
    } else {
        bool cumulative = m_cumulative;
        m_cumulative = false;

        // Don't use evaluateFile() here to avoid calling aboutToEval().
        // The path is fully normalized already.
        bool ok = false;
        if (ProFile *pro = parsedProFile(fn, true)) {
            ok = (visitProFile(pro) == ProItem::ReturnTrue);
            pro->deref();
        }

        m_cumulative = cumulative;
        return ok;
    }
}

bool ProFileEvaluator::Private::evaluateFileInto(
        const QString &fileName, QHash<QString, QStringList> *values, FunctionDefs *funcs)
{
    ProFileEvaluator visitor(m_option);
    visitor.d->m_cumulative = false;
    visitor.d->m_parsePreAndPostFiles = false;
    visitor.d->m_verbose = m_verbose;
    visitor.d->m_valuemapStack.top() = *values;
    if (funcs)
        visitor.d->m_functionDefs = *funcs;
    if (!visitor.d->evaluateFile(fileName))
        return false;
    *values = visitor.d->m_valuemapStack.top();
    if (funcs) {
        *funcs = visitor.d->m_functionDefs;
        // So they are not unref'd
        visitor.d->m_functionDefs.testFunctions.clear();
        visitor.d->m_functionDefs.replaceFunctions.clear();
    }
    return true;
}

QString ProFileEvaluator::Private::format(const char *fmt) const
{
    ProFile *pro = currentProFile();
    QString fileName = pro ? pro->fileName() : QLatin1String("Not a file");
    int lineNumber = pro ? m_lineNo : 0;
    return QString::fromLatin1("%1(%2):").arg(fileName).arg(lineNumber) + QString::fromAscii(fmt);
}

void ProFileEvaluator::Private::logMessage(const QString &message) const
{
    if (m_verbose && !m_skipLevel)
        q->logMessage(message);
}

void ProFileEvaluator::Private::fileMessage(const QString &message) const
{
    if (!m_skipLevel)
        q->fileMessage(message);
}

void ProFileEvaluator::Private::errorMessage(const QString &message) const
{
    if (!m_skipLevel)
        q->errorMessage(message);
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

ProFileEvaluator::ProFileEvaluator(ProFileOption *option)
  : d(new Private(this, option))
{
}

ProFileEvaluator::~ProFileEvaluator()
{
    delete d;
}

bool ProFileEvaluator::contains(const QString &variableName) const
{
    return d->m_valuemapStack.top().contains(variableName);
}

QStringList ProFileEvaluator::values(const QString &variableName) const
{
    return expandEnvVars(d->values(variableName));
}

QStringList ProFileEvaluator::values(const QString &variableName, const ProFile *pro) const
{
    // It makes no sense to put any kind of magic into expanding these
    return expandEnvVars(d->m_filevaluemap.value(pro).value(variableName));
}

QStringList ProFileEvaluator::absolutePathValues(
        const QString &variable, const QString &baseDirectory) const
{
    QStringList result;
    foreach (const QString &el, values(variable)) {
        QString absEl = IoUtils::resolvePath(baseDirectory, el);
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
            if (IoUtils::exists(el)) {
                result << QDir::cleanPath(el);
                goto next;
            }
            absEl = el;
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
            QString absDir = QString::fromRawData(absEl.constData(), nameOff);
            if (IoUtils::exists(absDir)) {
                QString wildcard = QString::fromRawData(absEl.constData() + nameOff + 1,
                                                        absEl.length() - nameOff - 1);
                if (wildcard.contains(QLatin1Char('*')) || wildcard.contains(QLatin1Char('?'))) {
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

ProFileEvaluator::TemplateType ProFileEvaluator::templateType()
{
    QStringList templ = values(statics.strTEMPLATE);
    if (templ.count() >= 1) {
        const QString &t = templ.last();
        if (!t.compare(QLatin1String("app"), Qt::CaseInsensitive))
            return TT_Application;
        if (!t.compare(QLatin1String("lib"), Qt::CaseInsensitive))
            return TT_Library;
        if (!t.compare(QLatin1String("script"), Qt::CaseInsensitive))
            return TT_Script;
        if (!t.compare(QLatin1String("subdirs"), Qt::CaseInsensitive))
            return TT_Subdirs;
    }
    return TT_Unknown;
}

ProFile *ProFileEvaluator::parsedProFile(const QString &fileName, const QString &contents)
{
    return d->parsedProFile(fileName, false, contents);
}

bool ProFileEvaluator::accept(ProFile *pro)
{
    return d->visitProFile(pro);
}

QString ProFileEvaluator::propertyValue(const QString &name) const
{
    return d->propertyValue(name);
}

void ProFileEvaluator::aboutToEval(ProFile *)
{
}

void ProFileEvaluator::logMessage(const QString &message)
{
    qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::fileMessage(const QString &message)
{
    qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::errorMessage(const QString &message)
{
    qWarning("%s", qPrintable(message));
}

void ProFileEvaluator::setVerbose(bool on)
{
    d->m_verbose = on;
}

void ProFileEvaluator::setCumulative(bool on)
{
    d->m_cumulative = on;
}

void ProFileEvaluator::setOutputDir(const QString &dir)
{
    d->m_outputDir = dir;
}

void ProFileEvaluator::setConfigCommandLineArguments(const QStringList &addUserConfigCmdArgs, const QStringList &removeUserConfigCmdArgs)
{
    d->m_addUserConfigCmdArgs = addUserConfigCmdArgs;
    d->m_removeUserConfigCmdArgs = removeUserConfigCmdArgs;
}

void ProFileEvaluator::setParsePreAndPostFiles(bool on)
{
    d->m_parsePreAndPostFiles = on;
}

QT_END_NAMESPACE
