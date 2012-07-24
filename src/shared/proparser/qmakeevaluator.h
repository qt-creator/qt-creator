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

#ifndef QMAKEEVALUATOR_H
#define QMAKEEVALUATOR_H

#include "qmakeparser.h"
#include "ioutils.h"

#include <QList>
#include <QSet>
#include <QStack>
#include <QString>
#include <QStringList>
#ifndef QT_BOOTSTRAPPED
# include <QProcess>
#endif

QT_BEGIN_NAMESPACE

class QMakeGlobals;

class QMAKE_EXPORT QMakeHandler : public QMakeParserHandler
{
public:
    enum {
        SourceEvaluator = 0x10,

        EvalWarnLanguage = SourceEvaluator |  WarningMessage | WarnLanguage,
        EvalWarnDeprecated = SourceEvaluator | WarningMessage | WarnDeprecated,

        EvalError = ErrorMessage | SourceEvaluator
    };

    // error(), warning() and message() from .pro file
    virtual void fileMessage(const QString &msg) = 0;

    enum EvalFileType { EvalProjectFile, EvalIncludeFile, EvalConfigFile, EvalFeatureFile, EvalAuxFile };
    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type) = 0;
    virtual void doneWithEval(ProFile *parent) = 0;
};

class QMAKE_EXPORT QMakeEvaluator
{
public:
    enum LoadFlag {
        LoadProOnly = 0,
        LoadPreFiles = 1,
        LoadPostFiles = 2,
        LoadAll = LoadPreFiles|LoadPostFiles,
        LoadSilent = 0x10
    };
    Q_DECLARE_FLAGS(LoadFlags, LoadFlag)

    static void initStatics();
    static void initFunctionStatics();
    QMakeEvaluator(QMakeGlobals *option, QMakeParser *parser,
                   QMakeHandler *handler);
    ~QMakeEvaluator();

    ProStringList values(const ProString &variableName) const;
    ProStringList &valuesRef(const ProString &variableName);
    ProString first(const ProString &variableName) const;
    ProString propertyValue(const ProString &val) const;

    enum VisitReturn {
        ReturnFalse,
        ReturnTrue,
        ReturnError,
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

    void loadDefaults();
    bool prepareProject(const QString &inDir);
    bool loadSpec();
    void initFrom(const QMakeEvaluator &other);
    void setupProject();
    void visitCmdLine(const QString &cmds);
    VisitReturn visitProFile(ProFile *pro, QMakeHandler::EvalFileType type,
                             LoadFlags flags);
    VisitReturn visitProBlock(ProFile *pro, const ushort *tokPtr);
    VisitReturn visitProBlock(const ushort *tokPtr);
    VisitReturn visitProLoop(const ProString &variable, const ushort *exprPtr,
                             const ushort *tokPtr);
    void visitProFunctionDef(ushort tok, const ProString &name, const ushort *tokPtr);
    void visitProVariable(ushort tok, const ProStringList &curr, const ushort *&tokPtr);

    const ProString &map(const ProString &var);
    ProValueMap *findValues(const ProString &variableName, ProValueMap::Iterator *it);

    void setTemplate();

    ProStringList split_value_list(const QString &vals, const ProFile *source = 0);
    ProStringList expandVariableReferences(const ProString &value, int *pos = 0, bool joined = false);
    ProStringList expandVariableReferences(const ushort *&tokPtr, int sizeHint = 0, bool joined = false);

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;
    QString resolvePath(const QString &fileName) const
        { return ProFileEvaluatorInternal::IoUtils::resolvePath(currentDirectory(), fileName); }

    bool evaluateFileDirect(const QString &fileName, QMakeHandler::EvalFileType type,
                            LoadFlags flags);
    bool evaluateFile(const QString &fileName, QMakeHandler::EvalFileType type,
                      LoadFlags flags);
    bool evaluateFeatureFile(const QString &fileName, bool silent = false);
    bool evaluateFileInto(const QString &fileName, QMakeHandler::EvalFileType type,
                          ProValueMap *values, // output-only
                          LoadFlags flags);
    void message(int type, const QString &msg) const;
    void evalError(const QString &msg) const
            { message(QMakeHandler::EvalError, msg); }
    void languageWarning(const QString &msg) const
            { message(QMakeHandler::EvalWarnLanguage, msg); }
    void deprecationWarning(const QString &msg) const
            { message(QMakeHandler::EvalWarnDeprecated, msg); }

    QList<ProStringList> prepareFunctionArgs(const ushort *&tokPtr);
    ProStringList evaluateFunction(const ProFunctionDef &func,
                                   const QList<ProStringList> &argumentsList, bool *ok);
    VisitReturn evaluateBoolFunction(const ProFunctionDef &func,
                                     const QList<ProStringList> &argumentsList,
                                     const ProString &function);

    ProStringList evaluateExpandFunction(const ProString &function, const ushort *&tokPtr);
    VisitReturn evaluateConditionalFunction(const ProString &function, const ushort *&tokPtr);

    bool evaluateConditional(const QString &cond, const QString &context);
#ifdef PROEVALUATOR_FULL
    void checkRequirements(const ProStringList &deps);
#endif

    QStringList qmakeMkspecPaths() const;
    QStringList qmakeFeaturePaths() const;

    bool isActiveConfig(const QString &config, bool regex = false);

    void populateDeps(
            const ProStringList &deps, const ProString &prefix,
            QHash<ProString, QSet<ProString> > &dependencies,
            ProValueMap &dependees, ProStringList &rootSet) const;

    VisitReturn writeFile(const QString &ctx, const QString &fn, QIODevice::OpenMode mode,
                          const QString &contents);
#ifndef QT_BOOTSTRAPPED
    void runProcess(QProcess *proc, const QString &command) const;
#endif
    QByteArray getCommandOutput(const QString &args) const;

    int m_loopLevel; // To report unexpected break() and next()s
#ifdef PROEVALUATOR_CUMULATIVE
    bool m_cumulative;
    int m_skipLevel;
#else
    enum { m_cumulative = 0 };
    enum { m_skipLevel = 0 };
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
    bool m_hostBuild;
    QString m_qmakespec;
    QString m_qmakespecFull;
    QString m_qmakespecName;
    QString m_superfile;
    QString m_conffile;
    QString m_cachefile;
    QString m_sourceRoot;
    QString m_buildRoot;
    QStringList m_qmakepath;
    QStringList m_qmakefeatures;
    QStringList m_featureRoots;
    ProFunctionDefs m_functionDefs;
    ProStringList m_returnValue;
    QStack<ProValueMap> m_valuemapStack; // VariableName must be us-ascii, the content however can be non-us-ascii.
    QString m_tmp1, m_tmp2, m_tmp3, m_tmp[2]; // Temporaries for efficient toQString
    mutable QString m_mtmp;

    QMakeGlobals *m_option;
    QMakeParser *m_parser;
    QMakeHandler *m_handler;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMakeEvaluator::LoadFlags)

QT_END_NAMESPACE

#endif // QMAKEEVALUATOR_H
