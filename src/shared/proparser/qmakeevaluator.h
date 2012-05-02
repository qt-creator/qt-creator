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

#ifndef QMAKEEVALUATOR_H
#define QMAKEEVALUATOR_H

#include "profileevaluator.h"

#include "ioutils.h"

#include <QList>
#include <QSet>
#include <QStack>
#include <QString>
#include <QStringList>

QT_BEGIN_NAMESPACE

class QMakeParser;

class QMAKE_EXPORT QMakeEvaluatorHandler
{
public:
    // qmake/project configuration error
    virtual void configError(const QString &msg) = 0;
    // Some error during evaluation
    virtual void evalError(const QString &filename, int lineNo, const QString &msg) = 0;
    // error() and message() from .pro file
    virtual void fileMessage(const QString &msg) = 0;

    enum EvalFileType { EvalProjectFile, EvalIncludeFile, EvalConfigFile, EvalFeatureFile, EvalAuxFile };
    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type) = 0;
    virtual void doneWithEval(ProFile *parent) = 0;
};

class QMAKE_EXPORT QMakeEvaluator
{
public:
    static void initStatics();
    QMakeEvaluator(QMakeGlobals *option, QMakeParser *parser,
                   QMakeEvaluatorHandler *handler);
    ~QMakeEvaluator();

    ProStringList values(const ProString &variableName) const;
    ProStringList &valuesRef(const ProString &variableName);
    QString propertyValue(const QString &val, bool complain) const;

    enum VisitReturn {
        ReturnFalse,
        ReturnTrue,
        ReturnBreak,
        ReturnNext,
        ReturnReturn
    };

    static ALWAYS_INLINE VisitReturn returnBool(bool b)
        { return b ? ReturnTrue : ReturnFalse; }

    static ALWAYS_INLINE uint getBlockLen(const ushort *&tokPtr);
    ProString getStr(const ushort *&tokPtr);
    ProString getHashStr(const ushort *&tokPtr);
    void evaluateExpression(const ushort *&tokPtr, ProStringList *ret, bool joined);
    static ALWAYS_INLINE void skipStr(const ushort *&tokPtr);
    static ALWAYS_INLINE void skipHashStr(const ushort *&tokPtr);
    void skipExpression(const ushort *&tokPtr);

    void visitCmdLine(const QString &cmds);
    VisitReturn visitProFile(ProFile *pro, QMakeEvaluatorHandler::EvalFileType type,
                             ProFileEvaluator::LoadFlags flags);
    VisitReturn visitProBlock(ProFile *pro, const ushort *tokPtr);
    VisitReturn visitProBlock(const ushort *tokPtr);
    VisitReturn visitProLoop(const ProString &variable, const ushort *exprPtr,
                             const ushort *tokPtr);
    void visitProFunctionDef(ushort tok, const ProString &name, const ushort *tokPtr);
    void visitProVariable(ushort tok, const ProStringList &curr, const ushort *&tokPtr);

    static const ProString &map(const ProString &var);
    QHash<ProString, ProStringList> *findValues(const ProString &variableName,
                                                QHash<ProString, ProStringList>::Iterator *it);
    ProStringList valuesDirect(const ProString &variableName) const;

    ProStringList split_value_list(const QString &vals, const ProFile *source = 0);
    ProStringList expandVariableReferences(const ProString &value, int *pos = 0, bool joined = false);
    ProStringList expandVariableReferences(const ushort *&tokPtr, int sizeHint = 0, bool joined = false);

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;
    QString resolvePath(const QString &fileName) const
        { return ProFileEvaluatorInternal::IoUtils::resolvePath(currentDirectory(), fileName); }

    bool evaluateFileDirect(const QString &fileName, QMakeEvaluatorHandler::EvalFileType type,
                            ProFileEvaluator::LoadFlags flags);
    bool evaluateFile(const QString &fileName, QMakeEvaluatorHandler::EvalFileType type,
                      ProFileEvaluator::LoadFlags flags);
    bool evaluateFeatureFile(const QString &fileName);
    enum EvalIntoMode { EvalProOnly, EvalWithDefaults, EvalWithSetup };
    bool evaluateFileInto(const QString &fileName, QMakeEvaluatorHandler::EvalFileType type,
                          QHash<ProString, ProStringList> *values, ProFunctionDefs *defs,
                          EvalIntoMode mode); // values are output-only, defs are input-only
    void evalError(const QString &msg) const;

    QList<ProStringList> prepareFunctionArgs(const ushort *&tokPtr);
    QList<ProStringList> prepareFunctionArgs(const ProString &arguments);
    ProStringList evaluateFunction(const ProFunctionDef &func,
                                   const QList<ProStringList> &argumentsList, bool *ok);
    VisitReturn evaluateBoolFunction(const ProFunctionDef &func,
                                     const QList<ProStringList> &argumentsList,
                                     const ProString &function);

    ProStringList evaluateExpandFunction(const ProString &function, const ProString &arguments);
    ProStringList evaluateExpandFunction(const ProString &function, const ushort *&tokPtr);
    ProStringList evaluateExpandFunction(const ProString &function, const ProStringList &args);
    VisitReturn evaluateConditionalFunction(const ProString &function, const ProString &arguments);
    VisitReturn evaluateConditionalFunction(const ProString &function, const ushort *&tokPtr);
    VisitReturn evaluateConditionalFunction(const ProString &function, const ProStringList &args);

    bool modesForGenerator(const QString &gen,
            QMakeGlobals::HOST_MODE *host_mode, QMakeGlobals::TARG_MODE *target_mode) const;
    void validateModes() const;
    QStringList qmakeMkspecPaths() const;
    QStringList qmakeFeaturePaths() const;

    bool isActiveConfig(const QString &config, bool regex = false);

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
    QMakeEvaluatorHandler *m_handler;

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

QT_END_NAMESPACE

#endif // QMAKEEVALUATOR_H
