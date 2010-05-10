/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#ifdef PROPARSER_THREAD_SAFE
# include <QtCore/QThreadPool>
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

// Be fast even for debug builds
#ifdef __GNUC__
# define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define ALWAYS_INLINE __forceinline
#else
# define ALWAYS_INLINE inline
#endif

using namespace ProFileEvaluatorInternal;

QT_BEGIN_NAMESPACE

using namespace ProStringConstants;


static void refFunctions(QHash<ProString, ProFunctionDef *> *defs)
{
    foreach (ProFunctionDef *itm, *defs)
        itm->ref();
}

static void clearFunctions(QHash<ProString, ProFunctionDef *> *defs)
{
    foreach (ProFunctionDef *itm, *defs)
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
    foreach (const Entry &ent, parsed_files)
        if (ent.pro)
            ent.pro->deref();
}

void ProFileCache::discardFile(const QString &fileName)
{
#ifdef PROPARSER_THREAD_SAFE
    QMutexLocker lck(&mutex);
#endif
    QHash<QString, Entry>::Iterator it = parsed_files.find(fileName);
    if (it != parsed_files.end()) {
        if (it->pro)
            it->pro->deref();
        parsed_files.erase(it);
    }
}

void ProFileCache::discardFiles(const QString &prefix)
{
#ifdef PROPARSER_THREAD_SAFE
    QMutexLocker lck(&mutex);
#endif
    QHash<QString, Entry>::Iterator
            it = parsed_files.begin(),
            end = parsed_files.end();
    while (it != end)
        if (it.key().startsWith(prefix)) {
            if (it->pro)
                it->pro->deref();
            it = parsed_files.erase(it);
        } else {
            ++it;
        }
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
        BlockCursor(BlockCursor &other) : cursor(other.cursor) { other.cursor = 0; }
        BlockCursor(ProItem **itp) : cursor(itp) {}
        ~BlockCursor() { if (cursor) *cursor = 0; }
        BlockCursor &operator=(BlockCursor &other) { cursor = other.cursor; other.cursor = 0; return *this; }
        void set(ProItem **itp) { if (cursor) *cursor = 0; cursor = itp; }
        void reset() { if (cursor) { *cursor = 0; cursor = 0; } }
        void append(ProItem *item) { *cursor = item; cursor = item->nextRef(); }
        bool isValid() const { return cursor != 0; }
    private:
        ProItem **cursor;
    };

    struct BlockScope {
        BlockScope() : braceLevel(0), special(false) {}
        BlockScope(BlockScope &other) :
                cursor(other.cursor), elseCursor(other.elseCursor),
                braceLevel(other.braceLevel), special(other.special) {}
        BlockCursor cursor; // Current appending position
        BlockCursor elseCursor; // Appending position for else branch of last conditional in scope
        int braceLevel; // Nesting of braces in scope
        bool special; // Single-line conditionals cannot have else branches
    };

    enum ScopeState {
        StNew,  // Fresh scope
        StCtrl, // Control statement (for or else) met on current line
        StCond  // Conditionals met on current line
    };

    bool read(ProFile *pro);
    bool read(ProFile *pro, const QString &content);
    bool read(ProItem **itp, const QString &content);
    bool readInternal(ProItem **itp, const QString &content, ushort *buf);

    void updateItem(ushort *uc, ushort *ptr);
    ProVariable *startVariable(ushort *uc, ushort *ptr);
    void finalizeVariable(ProVariable *var, ushort *uc, ushort *ptr);
    void insertOperator(ProItem::ProItemKind opkind);
    void enterScope(BlockCursor &cursor, bool special, ScopeState state);
    void enterScope(ProItem **itp, bool special, ScopeState state)
        { BlockCursor curs(itp); enterScope(curs, special, state); }
    void flushCond();
    void flushScopes();

    QStack<BlockScope> m_blockstack;
    ScopeState m_state;
    bool m_canElse; // Conditionals met on previous line, but no scope was opened
    bool m_invert; // Pending conditional is negated
    enum { NoOperator, AndOperator, OrOperator } m_operator; // Pending conditional is ORed/ANDed

    /////////////// Evaluating pro file contents

    enum VisitReturn {
        ReturnFalse,
        ReturnTrue,
        ReturnBreak,
        ReturnNext,
        ReturnReturn
    };

    VisitReturn visitProFile(ProFile *pro);
    VisitReturn visitProBlock(ProItem *items);
    VisitReturn visitProLoop(ProLoop *loop);
    void visitProFunctionDef(ProFunctionDef *def);
    void visitProVariable(ProVariable *variable);
    VisitReturn visitProCondition(ProCondition *condition);

    static inline ProString map(const ProString &var);
    QHash<ProString, ProStringList> *findValues(const ProString &variableName,
                                                QHash<ProString, ProStringList>::Iterator *it);
    ProStringList &valuesRef(const ProString &variableName);
    ProStringList valuesDirect(const ProString &variableName) const;
    ProStringList values(const ProString &variableName) const;
    QString propertyValue(const QString &val, bool complain = true) const;

    static ProStringList split_value_list(const QString &vals);
    bool isActiveConfig(const QString &config, bool regex = false);
    ProStringList expandVariableReferences(const ProString &value, int *pos = 0, bool joined = false);
    ProStringList evaluateExpandFunction(const ProString &function, const ProString &arguments);
    QString format(const char *format) const;
    void logMessage(const QString &msg) const;
    void errorMessage(const QString &msg) const;
    void fileMessage(const QString &msg) const;

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;
    QString resolvePath(const QString &fileName) const
        { return IoUtils::resolvePath(currentDirectory(), fileName); }

    VisitReturn evaluateConditionalFunction(const ProString &function, const ProString &arguments);
    ProFile *parsedProFile(const QString &fileName, bool cache,
                           const QString &contents = QString());
    bool evaluateFile(const QString &fileName);
    bool evaluateFeatureFile(const QString &fileName,
                             QHash<ProString, ProStringList> *values = 0, FunctionDefs *defs = 0);
    bool evaluateFileInto(const QString &fileName,
                          QHash<ProString, ProStringList> *values, FunctionDefs *defs);

    static ALWAYS_INLINE VisitReturn returnBool(bool b)
        { return b ? ReturnTrue : ReturnFalse; }

    QList<ProStringList> prepareFunctionArgs(const ProString &arguments);
    ProStringList evaluateFunction(ProFunctionDef *func, const QList<ProStringList> &argumentsList, bool *ok);

    QStringList qmakeMkspecPaths() const;
    QStringList qmakeFeaturePaths() const;

    int m_skipLevel;
    int m_loopLevel; // To report unexpected break() and next()s
    bool m_cumulative;
    QStack<ProFile*> m_profileStack;                // To handle 'include(a.pri), so we can track back to 'a.pro' when finished with 'a.pri'

    QString m_outputDir;

    int m_listCount;
    FunctionDefs m_functionDefs;
    ProStringList m_returnValue;
    QStack<QHash<ProString, ProStringList> > m_valuemapStack;         // VariableName must be us-ascii, the content however can be non-us-ascii.
    QHash<const ProFile*, QHash<ProString, ProStringList> > m_filevaluemap; // Variables per include file
    QString m_tmp1, m_tmp2, m_tmp3, m_tmp[2]; // Temporaries for efficient toQString

    QStringList m_addUserConfigCmdArgs;
    QStringList m_removeUserConfigCmdArgs;
    bool m_parsePreAndPostFiles;

    ProFileOption *m_option;

    enum ExpandFunc {
        E_MEMBER=1, E_FIRST, E_LAST, E_SIZE, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
        E_SPRINTF, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
        E_FIND, E_SYSTEM, E_UNIQUE, E_QUOTE, E_ESCAPE_EXPAND,
        E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE,
        E_REPLACE
    };

    enum TestFunc {
        T_REQUIRES=1, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
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

#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::BlockCursor, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(ProFileEvaluator::Private::BlockScope, Q_MOVABLE_TYPE);
#endif

static struct {
    QString field_sep;
    QString strelse;
    QString strtrue;
    QString strfalse;
    QString strunix;
    QString strmacx;
    QString strqnx6;
    QString strmac9;
    QString strmac;
    QString strwin32;
    ProString strCONFIG;
    ProString strARGS;
    QString strDot;
    QString strDotDot;
    QString strfor;
    QString strever;
    QString strforever;
    QString strdefineTest;
    QString strdefineReplace;
    ProString strTEMPLATE;
    ProString strQMAKE_DIR_SEP;
    QHash<QString, int> expands;
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
    statics.strelse = QLatin1String("else");
    statics.strtrue = QLatin1String("true");
    statics.strfalse = QLatin1String("false");
    statics.strunix = QLatin1String("unix");
    statics.strmacx = QLatin1String("macx");
    statics.strqnx6 = QLatin1String("qnx6");
    statics.strmac9 = QLatin1String("mac9");
    statics.strmac = QLatin1String("mac");
    statics.strwin32 = QLatin1String("win32");
    statics.strCONFIG = ProString("CONFIG");
    statics.strARGS = ProString("ARGS");
    statics.strDot = QLatin1String(".");
    statics.strDotDot = QLatin1String("..");
    statics.strfor = QLatin1String("for");
    statics.strever = QLatin1String("ever");
    statics.strforever = QLatin1String("forever");
    statics.strdefineTest = QLatin1String("defineTest");
    statics.strdefineReplace = QLatin1String("defineReplace");
    statics.strTEMPLATE = ProString("TEMPLATE");
    statics.strQMAKE_DIR_SEP = ProString("QMAKE_DIR_SEP");

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

ProString ProFileEvaluator::Private::map(const ProString &var)
{
    return statics.varMap.value(var, var);
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
    m_skipLevel = 0;
    m_loopLevel = 0;
    m_listCount = 0;
    m_valuemapStack.push(QHash<ProString, ProStringList>());
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
    m_profileStack.push(pro);
    bool ret = readInternal(pro->itemsRef(), content, (ushort*)content.data());
    m_profileStack.pop();
    return ret;
}

bool ProFileEvaluator::Private::read(ProFile *pro, const QString &content)
{
    QString buf;
    buf.reserve(content.size());
    m_lineNo = 1;
    m_profileStack.push(pro);
    bool ret = readInternal(pro->itemsRef(), content, (ushort*)buf.data());
    m_profileStack.pop();
    return ret;
}

bool ProFileEvaluator::Private::read(ProItem **itp, const QString &content)
{
    QString buf;
    buf.reserve(content.size());
    m_lineNo = 0;
    return readInternal(itp, content, (ushort*)buf.data());
}

// We know that the buffer cannot grow larger than the input string,
// and the read() functions rely on it.
bool ProFileEvaluator::Private::readInternal(ProItem **itp, const QString &in, ushort *buf)
{
    // Parser state
    m_blockstack.resize(m_blockstack.size() + 1);
    m_blockstack.top().cursor.set(itp);

    // We rely on QStrings being null-terminated, so don't maintain a global end pointer.
    const ushort *cur = (const ushort *)in.unicode();
    m_canElse = false;
  freshLine:
    m_state = StNew;
    m_invert = false;
    m_operator = NoOperator;
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
                if (c) {
                    ++cur;
                    ++m_lineNo;
                    goto freshLine;
                }
                if (m_blockstack.size() > 1)
                    logMessage(format("Missing closing brace(s)."));
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
                    } else if (c == '!' && ptr == buf) {
                        m_invert ^= true;
                        goto nextItem;
                    } else if (!quote) {
                        if (c == '(') {
                            ++parens;
                        } else if (c == ')') {
                            --parens;
                        } else if (!parens) {
                            if (c == ':') {
                                updateItem(buf, ptr);
                                if (m_state == StNew)
                                    logMessage(format("And operator without prior condition."));
                                else
                                    m_operator = AndOperator;
                              nextItem:
                                ptr = buf;
                                putSpace = false;
                                goto newItem;
                            }
                            if (c == '|') {
                                updateItem(buf, ptr);
                                if (m_state != StCond)
                                    logMessage(format("Or operator without prior condition."));
                                else
                                    m_operator = OrOperator;
                                goto nextItem;
                            }
                            if (c == '{') {
                                updateItem(buf, ptr);
                                flushCond();
                                ++m_blockstack.top().braceLevel;
                                goto nextItem;
                            }
                            if (c == '}') {
                                updateItem(buf, ptr);
                                flushScopes();
                                if (!m_blockstack.top().braceLevel)
                                    errorMessage(format("Excess closing brace."));
                                else if (!--m_blockstack.top().braceLevel
                                         && m_blockstack.count() != 1) {
                                    m_blockstack.resize(m_blockstack.size() - 1);
                                    m_state = StNew;
                                    m_canElse = false;
                                }
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
            cur = cptr;
            ++m_lineNo;
            goto freshLine;
        }
      ignore:
        cur = cptr;
        ++m_lineNo;
    }
}

ProVariable *ProFileEvaluator::Private::startVariable(ushort *uc, ushort *ptr)
{
    flushCond();

    if (ptr == uc) // Line starting with '=', like a conflict marker
        return 0;

    ProVariable::VariableOperator opkind;
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
    ProVariable *variable = new ProVariable(map(ProString(QString((QChar*)uc, ptr - uc))));
    variable->setLineNumber(m_lineNo);
    variable->setVariableOperator(opkind);
    return variable;
}

void ProFileEvaluator::Private::finalizeVariable(ProVariable *variable, ushort *uc, ushort *ptr)
{
    variable->setValue(ProString(QString((QChar*)uc, ptr - uc), NoHash));

    m_blockstack.top().cursor.append(variable);
}

void ProFileEvaluator::Private::enterScope(BlockCursor &cursor, bool special, ScopeState state)
{
    m_blockstack.resize(m_blockstack.size() + 1);
    m_blockstack.top().cursor = cursor;
    m_blockstack.top().special = special;
    m_state = state;
    m_canElse = false;
}

void ProFileEvaluator::Private::flushScopes()
{
    if (m_state == StNew) {
        while (!m_blockstack.top().braceLevel && m_blockstack.size() > 1)
            m_blockstack.resize(m_blockstack.size() - 1);
        m_blockstack.top().elseCursor.reset();
        m_canElse = false;
    }
}

void ProFileEvaluator::Private::flushCond()
{
    if (m_state == StCond) {
        ProBranch *blk = new ProBranch();
        blk->setLineNumber(m_lineNo);
        BlockScope &top = m_blockstack.top();
        top.cursor.append(blk);
        if (!top.special || top.braceLevel) {
            top.elseCursor.set(blk->elseItemsRef());
        } else {
            *blk->elseItemsRef() = 0;
            // elseCursor was either never set in this scope
            // or the last flushScopes reset it.
        }
        enterScope(blk->thenItemsRef(), false, StNew);
    } else {
        flushScopes();
    }
}

void ProFileEvaluator::Private::insertOperator(ProItem::ProItemKind opkind)
{
    ProItem *proOp = new ProItem(opkind);
    proOp->setLineNumber(m_lineNo);
    m_blockstack.top().cursor.append(proOp);
}

static bool get_next_arg(const QString &params, int *pos, QString *out)
{
    int quote = 0;
    int parens = 0;
    const ushort *uc = (const ushort *)params.constData();
    const ushort *params_start = uc + *pos;
    if (*params_start == ' ')
        ++params_start;
    const ushort *params_data = params_start;
    const ushort *params_next = 0;
    for (;; params_data++) {
        ushort unicode = *params_data;
        if (!unicode) { // Huh?
            params_next = params_data;
            break;
        } else if (unicode == '(') {
            ++parens;
        } else if (unicode == ')') {
            if (--parens < 0) {
                params_next = params_data;
                break;
            }
        } else if (quote && unicode == quote) {
            quote = 0;
        } else if (!quote && (unicode == '\'' || unicode == '"')) {
            quote = unicode;
        }
        if (!parens && !quote && unicode == ',') {
            params_next = params_data + 1;
            break;
        }
    }
    if (params_data == params_start)
        return false;
    if (out) {
        if (params_data[-1] == ' ')
            --params_data;
        *out = params.mid(params_start - uc, params_data - params_start);
    }
    *pos = params_next - uc;
    return true;
}

static bool isKeyFunc(const QString &str, const QString &key, int *pos)
{
    if (!str.startsWith(key))
        return false;
    const ushort *uc = (const ushort *)str.constData() + key.length();
    if (*uc == ' ')
        uc++;
    if (*uc != '(')
        return false;
    *pos = uc - (const ushort *)str.constData() + 1;
    return true;
}

void ProFileEvaluator::Private::updateItem(ushort *uc, ushort *ptr)
{
    if (ptr == uc)
        return;

    QString str((QChar*)uc, ptr - uc);
    int pos;
    const QString *defName;
    ProFunctionDef::FunctionType defType;
    if (!str.compare(statics.strelse, Qt::CaseInsensitive)) {
        if (m_invert || m_operator != NoOperator) {
            logMessage(format("Unexpected operator in front of else."));
            return;
        }
        BlockScope &top = m_blockstack.top();
        if (m_canElse && (!top.special || top.braceLevel)) {
            ProBranch *blk = new ProBranch();
            blk->setLineNumber(m_lineNo);
            *blk->thenItemsRef() = 0;
            top.cursor.append(blk);
            enterScope(blk->elseItemsRef(), false, StCtrl);
            return;
        }
        forever {
            BlockScope &top = m_blockstack.top();
            if (top.elseCursor.isValid()) {
                BlockCursor cursor(top.elseCursor); // ref into stack becomes stale
                enterScope(cursor, false, StCtrl);
                return;
            }
            if (top.braceLevel || m_blockstack.size() == 1)
                break;
            m_blockstack.resize(m_blockstack.size() - 1);
        }
        errorMessage(format("Unexpected 'else'."));
    } else if (isKeyFunc(str, statics.strfor, &pos)) {
        flushCond();
        if (m_invert || m_operator == OrOperator) {
            // '|' could actually work reasonably, but qmake does nonsense here.
            logMessage(format("Unexpected operator in front of for()."));
            return;
        }
        QString var, expr;
        if (!get_next_arg(str, &pos, &var)
            || (get_next_arg(str, &pos, &expr) && get_next_arg(str, &pos, 0))) {
            logMessage(format("Syntax is for(var, list), for(var, forever) or for(ever)."));
            return;
        }
        if (expr.isEmpty()) {
            expr = var;
            var.clear();
        }
        ProLoop *loop = new ProLoop(var, expr);
        m_blockstack.top().cursor.append(loop);
        enterScope(loop->itemsRef(), true, StCtrl);
    } else if (isKeyFunc(str, statics.strdefineReplace, &pos)) {
        defName = &statics.strdefineReplace;
        defType = ProFunctionDef::ReplaceFunction;
        goto deffunc;
    } else if (isKeyFunc(str, statics.strdefineTest, &pos)) {
        defName = &statics.strdefineTest;
        defType = ProFunctionDef::TestFunction;
      deffunc:
        flushScopes();
        if (m_invert) {
            logMessage(format("Unexpected operator in front of function definition."));
            return;
        }
        QString func;
        if (!get_next_arg(str, &pos, &func) || get_next_arg(str, &pos, 0)) {
            logMessage(format("%s(function) requires one argument.").arg(*defName));
            return;
        }
        if (m_operator != NoOperator) {
            insertOperator(m_operator == AndOperator ? ProItem::OpAndKind : ProItem::OpOrKind);
            m_operator = NoOperator;
        }
        ProFunctionDef *def = new ProFunctionDef(func, defType);
        m_blockstack.top().cursor.append(def);
        enterScope(def->itemsRef(), true, StCtrl);
    } else {
        flushScopes();
        if (m_operator != NoOperator) {
            insertOperator(m_operator == AndOperator ? ProItem::OpAndKind : ProItem::OpOrKind);
            m_operator = NoOperator;
        }
        if (m_invert) {
            insertOperator(ProItem::OpNotKind);
            m_invert = false;
        }
        ProItem *item = new ProCondition(str);
        item->setLineNumber(m_lineNo);
        m_blockstack.top().cursor.append(item);
        m_state = StCond;
        m_canElse = true;
    }
}

//////// Evaluator tools /////////

// FIXME: this should not build new strings for direct sections.
// Note that the E_SPRINTF and E_LIST implementations rely on the deep copy.
ProStringList ProFileEvaluator::Private::split_value_list(const QString &vals)
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
            ret << ProString(build, NoHash);
            build.clear();
        } else {
            build += vals_data[x];
        }
    }
    if (!build.isEmpty())
        ret << ProString(build, NoHash);
    return ret;
}

static void insertUnique(ProStringList *varlist, const ProStringList &value)
{
    foreach (const ProString &str, value)
        if (!varlist->contains(str))
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
                *varit = ProString(val);
                ++varit;
            }
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

static bool isTrue(const ProString &_str, QString &tmp)
{
    const QString &str = _str.toQString(tmp);
    return !str.compare(statics.strtrue, Qt::CaseInsensitive) || str.toInt();
}

//////// Evaluator /////////

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProBlock(ProItem *items)
{
    bool okey = true, or_op = false, invert = false;
    VisitReturn ret = ReturnTrue;
    for (ProItem *itm = items; itm; itm = itm->next()) {
        switch (itm->kind()) {
        case ProItem::VariableKind:
            visitProVariable(static_cast<ProVariable*>(itm));
            continue;
        case ProItem::BranchKind:
            if (m_cumulative) {
                if (!okey)
                    m_skipLevel++;
                ret = visitProBlock(static_cast<ProBranch*>(itm)->thenItems());
                if (!okey)
                    m_skipLevel--;
                else
                    m_skipLevel++;
                if (ret == ReturnTrue || ret == ReturnFalse)
                    ret = visitProBlock(static_cast<ProBranch*>(itm)->elseItems());
                if (okey)
                    m_skipLevel--;
            } else {
                ret = visitProBlock(okey ? static_cast<ProBranch*>(itm)->thenItems()
                                         : static_cast<ProBranch*>(itm)->elseItems());
            }
            okey = true, or_op = false; // force next evaluation
            break;
        case ProItem::LoopKind:
            if (m_cumulative) // This is a no-win situation, so just pretend it's no loop
                ret = visitProBlock(static_cast<ProLoop*>(itm)->items());
            else if (okey != or_op)
                ret = visitProLoop(static_cast<ProLoop*>(itm));
            okey = true, or_op = false; // force next evaluation
            break;
        case ProItem::FunctionDefKind:
            if (m_cumulative || okey != or_op)
                visitProFunctionDef(static_cast<ProFunctionDef *>(itm));
            okey = true, or_op = false; // force next evaluation
            continue;
        case ProItem::OpNotKind:
            invert ^= true;
            break;
        case ProItem::OpAndKind:
            or_op = false;
            break;
        case ProItem::OpOrKind:
            or_op = true;
            break;
        case ProItem::ConditionKind:
            if (!m_skipLevel && okey != or_op) {
                ret = visitProCondition(static_cast<ProCondition*>(itm));
                switch (ret) {
                case ReturnTrue: okey = true; break;
                case ReturnFalse: okey = false; break;
                default: return ret;
                }
                okey ^= invert;
            } else if (m_cumulative) {
                m_skipLevel++;
                visitProCondition(static_cast<ProCondition*>(itm));
                m_skipLevel--;
            }
            or_op = !okey; // tentatively force next evaluation
            invert = false;
            break;
        default: Q_ASSERT_X(false, "visitProBlock", "unexpected item type");
        }
        if (ret != ReturnTrue && ret != ReturnFalse)
            break;
    }
    return ret;
}


void ProFileEvaluator::Private::visitProFunctionDef(ProFunctionDef *def)
{
    QHash<ProString, ProFunctionDef *> *hash =
            (def->type() == ProFunctionDef::TestFunction
             ? &m_functionDefs.testFunctions
             : &m_functionDefs.replaceFunctions);
    if (ProFunctionDef *xdef = hash->value(def->name()))
        xdef->deref();
    hash->insert(def->name(), def);
    def->ref();
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProLoop(ProLoop *loop)
{
    VisitReturn ret = ReturnTrue;
    bool infinite = false;
    int index = 0;
    ProString variable;
    ProStringList oldVarVal;
    ProString it_list = expandVariableReferences(loop->expression(), 0, true).first();
    if (loop->variable().isEmpty()) {
        if (it_list != statics.strever) {
            logMessage(format("Invalid loop expression."));
            return ReturnFalse;
        }
        it_list = ProString(statics.strforever);
    } else {
        variable = map(loop->variable());
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
                errorMessage(format("ran into infinite loop (> 1000 iterations)."));
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

        ret = visitProBlock(loop->items());
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

void ProFileEvaluator::Private::visitProVariable(ProVariable *var)
{
    m_lineNo = var->lineNumber();
    const ProString &varName = var->variable();

    if (var->variableOperator() == ProVariable::ReplaceOperator) {      // ~=
        // DEFINES ~= s/a/b/?[gqi]

        const ProStringList &varVal = expandVariableReferences(var->value(), 0, true);
        const QString &val = varVal.at(0).toQString(m_tmp1);
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
            replaceInList(&valuesRef(varName), regexp, replace, global, m_tmp2);
            replaceInList(&m_filevaluemap[currentProFile()][varName], regexp, replace, global, m_tmp2);
        }
    } else {
        const ProStringList &varVal = expandVariableReferences(var->value());
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

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProCondition(ProCondition *cond)
{
    if (cond->text().endsWith(QLatin1Char(')'))) {
        if (!m_cumulative && m_skipLevel)
            return ReturnTrue;
        int lparen = cond->text().indexOf(QLatin1Char('('));
        ProString text(cond->text());
        ProString arguments = text.mid(lparen + 1, text.size() - lparen - 2);
        ProString funcName = text.left(lparen).trimmed();
        m_lineNo = cond->lineNumber();
        return evaluateConditionalFunction(funcName, arguments);
    } else {
        if (m_skipLevel)
            return ReturnTrue;
        return returnBool(isActiveConfig(cond->text(), true));
    }
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::visitProFile(ProFile *pro)
{
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
                    QHash<ProString, ProStringList> cache_valuemap;
                    if (evaluateFileInto(qmake_cache, &cache_valuemap, 0)) {
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

            ProStringList &tgt = m_valuemapStack.top()[ProString("TARGET")];
            if (tgt.isEmpty())
                tgt.append(ProString(QFileInfo(pro->fileName()).baseName(), NoHash));

            ProStringList &tmp = m_valuemapStack.top()[ProString("CONFIG")];
            foreach (const QString &add, m_addUserConfigCmdArgs)
                tmp.append(ProString(add, NoHash));
            foreach (const QString &remove, m_removeUserConfigCmdArgs)
                removeAll(&tmp, ProString(remove, NoHash));
        }
    }

    visitProBlock(pro->items());

    if (m_profileStack.count() == 1) {
        if (m_parsePreAndPostFiles) {
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
    }
    m_profileStack.pop();

    return ReturnTrue;
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
                        logMessage(format("Missing %1 terminator [found %2]")
                            .arg(QChar(term))
                            .arg(unicode ? QString(unicode) : QString::fromLatin1(("end-of-line"))));
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
                    replacement = split_value_list(QString::fromLocal8Bit(qgetenv(
                            var.toQString(m_tmp1).toLatin1().constData())));
                } else if (var_type == PROPERTY) {
                    replacement << ProString(propertyValue(var.toQString(m_tmp1)), NoHash);
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
        QString cfg = config;
        cfg.detach(); // Keep m_tmp out of QRegExp's cache
        QRegExp re(cfg, Qt::CaseSensitive, QRegExp::Wildcard);

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

QList<ProStringList> ProFileEvaluator::Private::prepareFunctionArgs(const ProString &arguments)
{
    QList<ProStringList> args_list;
    for (int pos = 0; pos < arguments.size(); )
        args_list << expandVariableReferences(arguments, &pos);
    return args_list;
}

ProStringList ProFileEvaluator::Private::evaluateFunction(
        ProFunctionDef *funcPtr, const QList<ProStringList> &argumentsList, bool *ok)
{
    bool oki;
    ProStringList ret;

    if (m_valuemapStack.count() >= 100) {
        errorMessage(format("ran into infinite recursion (depth > 100)."));
        oki = false;
    } else {
        m_valuemapStack.push(QHash<ProString, ProStringList>());
        int loopLevel = m_loopLevel;
        m_loopLevel = 0;

        ProStringList args;
        for (int i = 0; i < argumentsList.count(); ++i) {
            args += argumentsList[i];
            m_valuemapStack.top()[ProString(QString::number(i+1))] = argumentsList[i];
        }
        m_valuemapStack.top()[statics.strARGS] = args;
        oki = (visitProBlock(funcPtr->items()) != ReturnFalse); // True || Return
        ret = m_returnValue;
        m_returnValue.clear();

        m_loopLevel = loopLevel;
        m_valuemapStack.pop();
    }
    if (ok)
        *ok = oki;
    if (oki)
        return ret;
    return ProStringList();
}

ProStringList ProFileEvaluator::Private::evaluateExpandFunction(
        const ProString &func, const ProString &arguments)
{
    if (ProFunctionDef *funcPtr = m_functionDefs.replaceFunctions.value(func, 0))
        return evaluateFunction(funcPtr, prepareFunctionArgs(arguments), 0);

    //why don't the builtin functions just use args_list? --Sam
    int pos = 0;
    ProStringList args = expandVariableReferences(arguments, &pos, true);

    ExpandFunc func_t = ExpandFunc(statics.expands.value(func.toQString(m_tmp1).toLower()));

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
                    logMessage(format("%1(var) section(var, sep, begin, end) "
                        "requires three or four arguments.").arg(func.toQString(m_tmp1)));
                } else {
                    var = args[0];
                    sep = args.at(1).toQString();
                    beg = args.at(2).toQString(m_tmp2).toInt();
                    if (args.count() == 4)
                        end = args.at(3).toQString(m_tmp2).toInt();
                }
            } else {
                if (args.count() != 1) {
                    logMessage(format("%1(var) requires one argument.")
                               .arg(func.toQString(m_tmp1)));
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
                        ret << (rstr.isSharedWith(m_tmp1) ? str : ProString(rstr, NoHash));
                    }
                } else {
                    foreach (const ProString &str, values(map(var))) {
                        const QString &rstr = str.toQString(m_tmp1).section(sep, beg, end);
                        ret << (rstr.isSharedWith(m_tmp1) ? str : ProString(rstr, NoHash));
                    }
                }
            }
            break;
        }
        case E_SPRINTF:
            if(args.count() < 1) {
                logMessage(format("sprintf(format, ...) requires at least one argument"));
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
                logMessage(format("join(var, glue, before, after) requires one to four arguments."));
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
                if (!var.isEmpty())
                    ret.append(ProString(before + var.join(glue) + after, NoHash));
            }
            break;
        }
        case E_SPLIT:
            if (args.count() != 2) {
                logMessage(format("split(var, sep) requires one or two arguments"));
            } else {
                const QString &sep = (args.count() == 2) ? args.at(1).toQString(m_tmp1) : statics.field_sep;
                foreach (const ProString &var, values(map(args.at(0))))
                    foreach (const QString &splt, var.toQString(m_tmp2).split(sep))
                        ret << (splt.isSharedWith(m_tmp2) ? var : ProString(splt, NoHash));
            }
            break;
        case E_MEMBER:
            if (args.count() < 1 || args.count() > 3) {
                logMessage(format("member(var, start, end) requires one to three arguments."));
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
                            logMessage(format("member() argument 2 (start) '%2' invalid.")
                                .arg(start_str));
                    } else {
                        end = start;
                        if (args.count() == 3)
                            end = args.at(2).toQString(m_tmp1).toInt(&ok);
                        if (!ok)
                            logMessage(format("member() argument 3 (end) '%2' invalid.\n")
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
                logMessage(format("%1(var) requires one argument.").arg(func.toQString(m_tmp1)));
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
            if(args.count() != 1)
                logMessage(format("size(var) requires one argument."));
            else
                ret.append(ProString(QString::number(values(map(args.at(0))).size()), NoHash));
            break;
        case E_CAT:
            if (args.count() < 1 || args.count() > 2) {
                logMessage(format("cat(file, singleline=true) requires one or two arguments."));
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
                logMessage(format("fromfile(file, variable) requires two arguments."));
            } else {
                QHash<ProString, ProStringList> vars;
                if (evaluateFileInto(resolvePath(expandEnvVars(args.at(0).toQString(m_tmp1))), &vars, 0))
                    ret = vars.value(map(args.at(1)));
            }
            break;
        case E_EVAL:
            if (args.count() != 1) {
                logMessage(format("eval(variable) requires one argument"));
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
                lst += split_value_list(arg.toQString(m_tmp1)); // Relies on deep copy
            m_valuemapStack.top()[ret.at(0)] = lst;
            break; }
        case E_FIND:
            if (args.count() != 2) {
                logMessage(format("find(var, str) requires two arguments."));
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
                    logMessage(format("system(execute) requires one or two arguments."));
                } else {
                    char buff[256];
                    FILE *proc = QT_POPEN((QLatin1String("cd ")
                                           + IoUtils::shellQuote(currentDirectory())
                                           + QLatin1String(" && ") + args[0]).toLatin1(), "r");
                    bool singleLine = true;
                    if (args.count() > 1)
                        singleLine = isTrue(args.at(1), m_tmp2);
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
                ret.append(ProString(QString(i_data, i_len), NoHash));
            }
            break;
        case E_RE_ESCAPE:
            for (int i = 0; i < args.size(); ++i) {
                const QString &rstr = QRegExp::escape(args.at(i).toQString(m_tmp1));
                ret << (rstr.isSharedWith(m_tmp1) ? args.at(i) : ProString(rstr, NoHash));
            }
            break;
        case E_UPPER:
        case E_LOWER:
            for (int i = 0; i < args.count(); ++i) {
                QString rstr = args.at(i).toQString(m_tmp1);
                rstr = (func_t == E_UPPER) ? rstr.toUpper() : rstr.toLower();
                ret << (rstr.isSharedWith(m_tmp1) ? args.at(i) : ProString(rstr, NoHash));
            }
            break;
        case E_FILES:
            if (args.count() != 1 && args.count() != 2) {
                logMessage(format("files(pattern, recursive=false) requires one or two arguments"));
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
                            ret += ProString(fname, NoHash);
                    }
                }
            }
            break;
        case E_REPLACE:
            if(args.count() != 3 ) {
                logMessage(format("replace(var, before, after) requires three arguments"));
            } else {
                const QRegExp before(args.at(1).toQString());
                const QString &after(args.at(2).toQString(m_tmp2));
                foreach (const ProString &val, values(map(args.at(0)))) {
                    QString rstr = val.toQString(m_tmp1);
                    QString copy = rstr; // Force a detach on modify
                    rstr.replace(before, after);
                    ret << (rstr.isSharedWith(m_tmp1) ? val : ProString(rstr, NoHash));
                }
            }
            break;
        case 0:
            logMessage(format("'%1' is not a recognized replace function")
                       .arg(func.toQString(m_tmp1)));
            break;
        default:
            logMessage(format("Function '%1' is not implemented").arg(func.toQString(m_tmp1)));
            break;
    }

    return ret;
}

ProFileEvaluator::Private::VisitReturn ProFileEvaluator::Private::evaluateConditionalFunction(
        const ProString &function, const ProString &arguments)
{
    if (ProFunctionDef *funcPtr = m_functionDefs.testFunctions.value(function, 0)) {
        bool ok;
        ProStringList ret = evaluateFunction(funcPtr, prepareFunctionArgs(arguments), &ok);
        if (ok) {
            if (ret.isEmpty()) {
                return ReturnTrue;
            } else {
                if (ret.at(0) != statics.strfalse) {
                    if (ret.at(0) == statics.strtrue) {
                        return ReturnTrue;
                    } else {
                        bool ok;
                        int val = ret.at(0).toQString(m_tmp1).toInt(&ok);
                        if (ok) {
                            if (val)
                                return ReturnTrue;
                        } else {
                            logMessage(format("Unexpected return value from test '%1': %2")
                                          .arg(function.toQString(m_tmp1))
                                          .arg(ret.join(QLatin1String(" :: "))));
                        }
                    }
                }
            }
        }
        return ReturnFalse;
    }

    //why don't the builtin functions just use args_list? --Sam
    int pos = 0;
    ProStringList args = expandVariableReferences(arguments, &pos, true);

    TestFunc func_t = (TestFunc)statics.functions.value(function);

    switch (func_t) {
        case T_DEFINED:
            if (args.count() < 1 || args.count() > 2) {
                logMessage(format("defined(function, [\"test\"|\"replace\"])"
                                     " requires one or two arguments."));
                return ReturnFalse;
            }
            if (args.count() > 1) {
                if (args[1] == QLatin1String("test"))
                    return returnBool(m_functionDefs.testFunctions.contains(args[0]));
                else if (args[1] == QLatin1String("replace"))
                    return returnBool(m_functionDefs.replaceFunctions.contains(args[0]));
                logMessage(format("defined(function, type):"
                                  " unexpected type [%1].\n").arg(args.at(1).toQString(m_tmp1)));
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
                logMessage(format("unexpected return()."));
                return ReturnFalse;
            }
            return ReturnReturn;
        case T_EXPORT: {
            if (m_skipLevel && !m_cumulative)
                return ReturnTrue;
            if (args.count() != 1) {
                logMessage(format("export(variable) requires one argument."));
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
                logMessage(format("infile(file, var, [values]) requires two or three arguments."));
            } else {
                QHash<ProString, ProStringList> vars;
                if (!evaluateFileInto(resolvePath(expandEnvVars(args.at(0).toQString(m_tmp1))), &vars, 0))
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
                ProItem *pro;
                QString buf = args.join(QLatin1String(" "));
                if (!readInternal(&pro, buf, (ushort*)buf.data()))
                    return ReturnFalse;
                VisitReturn ret = visitProBlock(pro);
                ProItem::disposeItems(pro);
                return ret;
            }
        case T_BREAK:
            if (m_skipLevel)
                return ReturnFalse;
            if (m_loopLevel)
                return ReturnBreak;
            logMessage(format("unexpected break()."));
            return ReturnFalse;
        case T_NEXT:
            if (m_skipLevel)
                return ReturnFalse;
            if (m_loopLevel)
                return ReturnNext;
            logMessage(format("unexpected next()."));
            return ReturnFalse;
        case T_IF: {
            if (m_skipLevel && !m_cumulative)
                return ReturnFalse;
            if (args.count() != 1) {
                logMessage(format("if(condition) requires one argument."));
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
            QString args;
            args.reserve(50);
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
                        args += c;
                    ++parens;
                } else if (c == ')') {
                    --parens;
                    if (parens)
                        args += c;
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
                    args += c;
                }
                if (!quoted && !parens && (isOp || d == ed)) {
                    if (m_cumulative || (orOp != ret)) {
                        if (isFunc)
                            ret = evaluateConditionalFunction(ProString(test), ProString(args, NoHash));
                        else
                            ret = isActiveConfig(test, true);
                        ret ^= invert;
                    }
                    orOp = (c == '|');
                    invert = false;
                    isFunc = false;
                    test.clear();
                    args.clear();
                }
            }
            return returnBool(ret);
        }
        case T_CONFIG: {
            if (args.count() < 1 || args.count() > 2) {
                logMessage(format("CONFIG(config) requires one or two arguments."));
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
                logMessage(format("contains(var, val) requires two or three arguments."));
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
                logMessage(format("count(var, count, op=\"equals\") requires two or three arguments."));
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
                    logMessage(format("unexpected modifier to count(%2)")
                               .arg(comp.toQString(m_tmp1)));
                    return ReturnFalse;
                }
            }
            return returnBool(cnt == args.at(1).toQString(m_tmp1).toInt());
        }
        case T_GREATERTHAN:
        case T_LESSTHAN: {
            if (args.count() != 2) {
                logMessage(format("%1(variable, value) requires two arguments.")
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
                logMessage(format("%1(variable, value) requires two arguments.")
                           .arg(function.toQString(m_tmp1)));
                return ReturnFalse;
            }
            return returnBool(values(map(args.at(0))).join(statics.field_sep)
                              == args.at(1).toQString(m_tmp1));
        case T_CLEAR: {
            if (m_skipLevel && !m_cumulative)
                return ReturnFalse;
            if (args.count() != 1) {
                logMessage(format("%1(variable) requires one argument.")
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
                logMessage(format("%1(variable) requires one argument.")
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
                logMessage(format("include(file, into, silent) requires one, two or three arguments."));
                return ReturnFalse;
            }
            QString fn = resolvePath(expandEnvVars(args.at(0).toQString()));
            bool ok;
            if (parseInto.isEmpty()) {
                ok = evaluateFile(fn);
            } else {
                QHash<ProString, ProStringList> symbols;
                if ((ok = evaluateFileInto(fn, &symbols, 0))) {
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
            bool ignore_error = false;
            if (args.count() == 2) {
                ignore_error = isTrue(args.at(1), m_tmp2);
            } else if (args.count() != 1) {
                logMessage(format("load(feature) requires one or two arguments."));
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
                logMessage(format("%1(message) requires one argument.")
                           .arg(function.toQString(m_tmp1)));
                return ReturnFalse;
            }
            const QString &msg = expandEnvVars(args.at(0).toQString(m_tmp2));
            fileMessage(QString::fromLatin1("Project %1: %2")
                        .arg(function.toQString(m_tmp1).toUpper(), msg));
            // ### Consider real termination in non-cumulative mode
            return returnBool(function != QLatin1String("error"));
        }
#if 0 // Way too dangerous to enable.
        case T_SYSTEM: {
            if (args.count() != 1) {
                logMessage(format("system(exec) requires one argument."));
                ReturnFalse;
            }
            return returnBool(system((QLatin1String("cd ")
                                      + IoUtils::shellQuote(currentDirectory())
                                      + QLatin1String(" && ") + args.at(0)).toLatin1().constData()) == 0);
        }
#endif
        case T_ISEMPTY: {
            if (args.count() != 1) {
                logMessage(format("isEmpty(var) requires one argument."));
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
                logMessage(format("exists(file) requires one argument."));
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
        case 0:
            logMessage(format("'%1' is not a recognized test function")
                       .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        default:
            logMessage(format("Function '%1' is not implemented").arg(function.toQString(m_tmp1)));
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
    if (it != m_valuemapStack.top().end())
        return *it;
    for (int i = m_valuemapStack.size() - 1; --i >= 0; ) {
        QHash<ProString, ProStringList>::ConstIterator it = m_valuemapStack.at(i).constFind(variableName);
        if (it != m_valuemapStack.at(i).constEnd()) {
            ProStringList &ret = m_valuemapStack.top()[variableName];
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

ProFile *ProFileEvaluator::Private::parsedProFile(const QString &fileName, bool cache,
                                                  const QString &contents)
{
    ProFile *pro;
    if (cache && m_option->cache) {
        ProFileCache::Entry *ent;
#ifdef PROPARSER_THREAD_SAFE
        QMutexLocker locker(&m_option->cache->mutex);
#endif
        QHash<QString, ProFileCache::Entry>::Iterator it =
                m_option->cache->parsed_files.find(fileName);
        if (it != m_option->cache->parsed_files.end()) {
            ent = &*it;
#ifdef PROPARSER_THREAD_SAFE
            if (ent->locker && !ent->locker->done) {
                ++ent->locker->waiters;
                QThreadPool::globalInstance()->releaseThread();
                ent->locker->cond.wait(locker.mutex());
                QThreadPool::globalInstance()->reserveThread();
                if (!--ent->locker->waiters) {
                    delete ent->locker;
                    ent->locker = 0;
                }
            }
#endif
            if ((pro = ent->pro))
                pro->ref();
        } else {
            ent = &m_option->cache->parsed_files[fileName];
#ifdef PROPARSER_THREAD_SAFE
            ent->locker = new ProFileCache::Entry::Locker;
            locker.unlock();
#endif
            pro = new ProFile(fileName);
            if (!(contents.isNull() ? read(pro) : read(pro, contents))) {
                delete pro;
                pro = 0;
            } else {
                pro->ref();
            }
            ent->pro = pro;
#ifdef PROPARSER_THREAD_SAFE
            locker.relock();
            if (ent->locker->waiters) {
                ent->locker->done = true;
                ent->locker->cond.wakeAll();
            } else {
                delete ent->locker;
                ent->locker = 0;
            }
#endif
        }
    } else {
        pro = new ProFile(fileName);
        if (!(contents.isNull() ? read(pro) : read(pro, contents))) {
            delete pro;
            pro = 0;
        }
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
        bool ok = (visitProFile(pro) == ReturnTrue);
        pro->deref();
        return ok;
    } else {
        return false;
    }
}

bool ProFileEvaluator::Private::evaluateFeatureFile(
        const QString &fileName, QHash<ProString, ProStringList> *values, FunctionDefs *funcs)
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
        ProStringList &already = valuesRef(ProString("QMAKE_INTERNAL_INCLUDED_FEATURES"));
        ProString afn(fn, NoHash);
        if (already.contains(afn))
            return true;
        already.append(afn);
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
            ok = (visitProFile(pro) == ReturnTrue);
            pro->deref();
        }

        m_cumulative = cumulative;
        return ok;
    }
}

bool ProFileEvaluator::Private::evaluateFileInto(
        const QString &fileName, QHash<ProString, ProStringList> *values, FunctionDefs *funcs)
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
    return d->m_valuemapStack.top().contains(ProString(variableName));
}

static QStringList expandEnvVars(const ProStringList &x)
{
    QStringList ret;
    foreach (const ProString &str, x)
        ret << expandEnvVars(str.toQString());
    return ret;
}

QStringList ProFileEvaluator::values(const QString &variableName) const
{
    return expandEnvVars(d->values(ProString(variableName)));
}

QStringList ProFileEvaluator::values(const QString &variableName, const ProFile *pro) const
{
    // It makes no sense to put any kind of magic into expanding these
    return expandEnvVars(d->m_filevaluemap.value(pro).value(ProString(variableName)));
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
    const ProStringList &templ = d->values(statics.strTEMPLATE);
    if (templ.count() >= 1) {
        const QString &t = templ.last().toQString();
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
