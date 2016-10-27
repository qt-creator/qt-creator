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

#include "qmake_global.h"
#include "qmakevfs.h"
#include "proitems.h"

#include <qhash.h>
#include <qstack.h>
#ifdef PROPARSER_THREAD_SAFE
# include <qmutex.h>
# include <qwaitcondition.h>
#endif

QT_BEGIN_NAMESPACE
class QMAKE_EXPORT QMakeParserHandler
{
public:
    enum {
        CategoryMask = 0xf00,
        InfoMessage = 0x100,
        WarningMessage = 0x200,
        ErrorMessage = 0x300,

        SourceMask = 0xf0,
        SourceParser = 0,

        CodeMask = 0xf,
        WarnLanguage = 0,
        WarnDeprecated,

        ParserWarnLanguage = SourceParser | WarningMessage | WarnLanguage,
        ParserWarnDeprecated = SourceParser | WarningMessage | WarnDeprecated,

        ParserIoError = ErrorMessage | SourceParser,
        ParserError
    };
    virtual void message(int type, const QString &msg,
                         const QString &fileName = QString(), int lineNo = 0) = 0;
};

class ProFileCache;
class QMakeVfs;

class QMAKE_EXPORT QMakeParser
{
public:
    // Call this from a concurrency-free context
    static void initialize();

    enum ParseFlag {
        ParseDefault = 0,
        ParseUseCache = 1,
        ParseOnlyCached = 2,
        ParseReportMissing = 4,
#ifdef PROEVALUATOR_DUAL_VFS
        ParseCumulative = 8
#else
        ParseCumulative = 0
#endif
    };
    Q_DECLARE_FLAGS(ParseFlags, ParseFlag)

    QMakeParser(ProFileCache *cache, QMakeVfs *vfs, QMakeParserHandler *handler);

    enum SubGrammar { FullGrammar, TestGrammar, ValueGrammar };
    // fileName is expected to be absolute and cleanPath()ed.
    ProFile *parsedProFile(const QString &fileName, ParseFlags flags = ParseDefault);
    ProFile *parsedProBlock(const QStringRef &contents, const QString &name, int line = 0,
                            SubGrammar grammar = FullGrammar);

    void discardFileFromCache(const QString &fileName);

#ifdef PROPARSER_DEBUG
    static QString formatProBlock(const QString &block);
#endif

private:
    enum ScopeNesting {
        NestNone = 0,
        NestLoop = 1,
        NestFunction = 2
    };

    struct BlockScope {
        BlockScope() : start(0), braceLevel(0), special(false), inBranch(false), nest(NestNone) {}
        BlockScope(const BlockScope &other) { *this = other; }
        ushort *start; // Where this block started; store length here
        int braceLevel; // Nesting of braces in scope
        bool special; // Single-line conditionals inside loops, etc. cannot have else branches
        bool inBranch; // The 'else' branch of the previous TokBranch is still open
        uchar nest; // Into what control structures we are nested
    };

    enum ScopeState {
        StNew,  // Fresh scope
        StCtrl, // Control statement (for or else) met on current line
        StCond  // Conditionals met on current line
    };

    enum Context { CtxTest, CtxValue, CtxPureValue, CtxArgs };
    struct ParseCtx {
        int parens; // Nesting of non-functional parentheses
        int argc; // Number of arguments in current function call
        int wordCount; // Number of words in current expression
        Context context;
        ushort quote; // Enclosing quote type
        ushort terminator; // '}' if replace function call is braced, ':' if test function
    };

    bool readFile(const QString &fn, QMakeVfs::VfsFlags vfsFlags, QMakeParser::ParseFlags flags, QString *contents);
    void read(ProFile *pro, const QStringRef &content, int line, SubGrammar grammar);

    ALWAYS_INLINE void putTok(ushort *&tokPtr, ushort tok);
    ALWAYS_INLINE void putBlockLen(ushort *&tokPtr, uint len);
    ALWAYS_INLINE void putBlock(ushort *&tokPtr, const ushort *buf, uint len);
    void putHashStr(ushort *&pTokPtr, const ushort *buf, uint len);
    void finalizeHashStr(ushort *buf, uint len);
    void putLineMarker(ushort *&tokPtr);
    ALWAYS_INLINE bool resolveVariable(ushort *xprPtr, int tlen, int needSep, ushort **ptr,
                                       ushort **buf, QString *xprBuff,
                                       ushort **tokPtr, QString *tokBuff,
                                       const ushort *cur, const QStringRef &in);
    void finalizeCond(ushort *&tokPtr, ushort *uc, ushort *ptr, int wordCount);
    void finalizeCall(ushort *&tokPtr, ushort *uc, ushort *ptr, int argc);
    void warnOperator(const char *msg);
    bool failOperator(const char *msg);
    bool acceptColon(const char *msg);
    void putOperator(ushort *&tokPtr);
    void finalizeTest(ushort *&tokPtr);
    void bogusTest(ushort *&tokPtr, const QString &msg);
    void enterScope(ushort *&tokPtr, bool special, ScopeState state);
    void leaveScope(ushort *&tokPtr);
    void flushCond(ushort *&tokPtr);
    void flushScopes(ushort *&tokPtr);

    void message(int type, const QString &msg) const;
    void parseError(const QString &msg) const
    {
        message(QMakeParserHandler::ParserError, msg);
        m_proFile->setOk(false);
    }
    void languageWarning(const QString &msg) const
            { message(QMakeParserHandler::ParserWarnLanguage, msg); }
    void deprecationWarning(const QString &msg) const
            { message(QMakeParserHandler::ParserWarnDeprecated, msg); }

    // Current location
    ProFile *m_proFile;
    int m_lineNo;

    QStack<BlockScope> m_blockstack;
    ScopeState m_state;
    int m_markLine; // Put marker for this line
    bool m_inError; // Current line had a parsing error; suppress followup error messages
    bool m_canElse; // Conditionals met on previous line, but no scope was opened
    int m_invert; // Pending conditional is negated
    enum { NoOperator, AndOperator, OrOperator } m_operator; // Pending conditional is ORed/ANDed

    QString m_tmp; // Temporary for efficient toQString

    ProFileCache *m_cache;
    QMakeParserHandler *m_handler;
    QMakeVfs *m_vfs;

    // This doesn't help gcc 3.3 ...
    template<typename T> friend class QTypeInfo;

    friend class ProFileCache;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMakeParser::ParseFlags)

class QMAKE_EXPORT ProFileCache
{
public:
    ProFileCache() {}
    ~ProFileCache();

    void discardFile(const QString &fileName);
    void discardFiles(const QString &prefix);

private:
    struct Entry {
        ProFile *pro;
#ifdef PROPARSER_THREAD_SAFE
        struct Locker {
            Locker() : waiters(0), done(false) {}
            QWaitCondition cond;
            int waiters;
            bool done;
        };
        Locker *locker;
#endif
    };

    QHash<QString, Entry> parsed_files;
#ifdef PROPARSER_THREAD_SAFE
    QMutex mutex;
#endif

    friend class QMakeParser;
};

#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
Q_DECLARE_TYPEINFO(QMakeParser::BlockScope, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QMakeParser::Context, Q_PRIMITIVE_TYPE);
#endif

QT_END_NAMESPACE
