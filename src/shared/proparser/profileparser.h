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

#ifndef PROFILEPARSER_H
#define PROFILEPARSER_H

#include "proitems.h"

#include <QtCore/QHash>
#include <QtCore/QStack>
#ifdef PROPARSER_THREAD_SAFE
# include <QtCore/QMutex>
# include <QtCore/QWaitCondition>
#endif

// Be fast even for debug builds
#ifdef __GNUC__
# define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define ALWAYS_INLINE __forceinline
#else
# define ALWAYS_INLINE inline
#endif

QT_BEGIN_NAMESPACE

class ProFileParserHandler
{
public:
    // Some error during parsing
    virtual void parseError(const QString &filename, int lineNo, const QString &msg) = 0;
};

class ProFileCache;

class ProFileParser
{
public:
    // Call this from a concurrency-free context
    static void initialize();

    ProFileParser(ProFileCache *cache, ProFileParserHandler *handler);

    // fileName is expected to be absolute and cleanPath()ed.
    // If contents is non-null, it will be used instead of the file's actual content
    ProFile *parsedProFile(const QString &fileName, bool cache = false,
                           const QString &contents = QString());

private:
    struct BlockScope {
        BlockScope() : start(0), braceLevel(0), special(false), inBranch(false) {}
        BlockScope(BlockScope &other) :
                start(other.start), braceLevel(other.braceLevel),
                special(other.special), inBranch(other.inBranch) {}
        ushort *start; // Where this block started; store length here
        int braceLevel; // Nesting of braces in scope
        bool special; // Single-line conditionals cannot have else branches
        bool inBranch; // The 'else' branch of the previous TokBranch is still open
    };

    enum ScopeState {
        StNew,  // Fresh scope
        StCtrl, // Control statement (for or else) met on current line
        StCond  // Conditionals met on current line
    };

    enum Context { CtxTest, CtxValue, CtxArgs };
    struct ParseCtx {
        int parens; // Nesting of non-functional parentheses
        int argc; // Number of arguments in current function call
        int litCount; // Number of literals in current expression
        int expCount; // Number of expansions in current expression
        Context context;
        ushort quote; // Enclosing quote type
        ushort terminator; // '}' if replace function call is braced, ':' if test function
    };

    bool read(ProFile *pro);
    bool read(ProFile *pro, const QString &content);

    ALWAYS_INLINE void putTok(ushort *&tokPtr, ushort tok);
    ALWAYS_INLINE void putBlockLen(ushort *&tokPtr, uint len);
    ALWAYS_INLINE void putBlock(ushort *&tokPtr, const ushort *buf, uint len);
    void putHashStr(ushort *&pTokPtr, const ushort *buf, uint len);
    void finalizeHashStr(ushort *buf, uint len);
    void putLineMarker(ushort *&tokPtr);
    void finalizeCond(ushort *&tokPtr, ushort *uc, ushort *ptr);
    void finalizeCall(ushort *&tokPtr, ushort *uc, ushort *ptr, int argc);
    void finalizeTest(ushort *&tokPtr);
    void enterScope(ushort *&tokPtr, bool special, ScopeState state);
    void leaveScope(ushort *&tokPtr);
    void flushCond(ushort *&tokPtr);
    void flushScopes(ushort *&tokPtr);

    void parseError(const QString &msg) const;

    // Current location
    QString m_fileName;
    int m_lineNo;

    QStack<BlockScope> m_blockstack;
    ScopeState m_state;
    int m_markLine; // Put marker for this line
    bool m_canElse; // Conditionals met on previous line, but no scope was opened
    bool m_invert; // Pending conditional is negated
    enum { NoOperator, AndOperator, OrOperator } m_operator; // Pending conditional is ORed/ANDed

    QString m_tmp; // Temporary for efficient toQString

    ProFileCache *m_cache;
    ProFileParserHandler *m_handler;

    // This doesn't help gcc 3.3 ...
    template<typename T> friend class QTypeInfo;

    friend class ProFileCache;
};

#if !defined(__GNUC__) || __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 3)
Q_DECLARE_TYPEINFO(ProFileParser::BlockScope, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(ProFileParser::Context, Q_PRIMITIVE_TYPE);
#endif

class ProFileCache
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

    friend class ProFileParser;
};

#endif // PROFILEPARSER_H
