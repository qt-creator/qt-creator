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

#if defined(PROEVALUATOR_FULL) && defined(PROEVALUATOR_THREAD_SAFE)
#  error PROEVALUATOR_FULL is incompatible with PROEVALUATOR_THREAD_SAFE due to cache() implementation
#endif

#include "qmakeparser.h"
#include "qmakevfs.h"
#include "ioutils.h"

#include <qlist.h>
#include <qlinkedlist.h>
#include <qmap.h>
#include <qset.h>
#include <qstack.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qshareddata.h>
#ifndef QT_BOOTSTRAPPED
# include <qprocess.h>
#else
# include <qiodevice.h>
#endif
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qmutex.h>
#endif

QT_BEGIN_NAMESPACE

class QMakeGlobals;

class QMAKE_EXPORT QMakeHandler : public QMakeParserHandler
{
public:
    enum {
        SourceEvaluator = 0x10,

        CumulativeEvalMessage = 0x1000,

        EvalWarnLanguage = SourceEvaluator |  WarningMessage | WarnLanguage,
        EvalWarnDeprecated = SourceEvaluator | WarningMessage | WarnDeprecated,

        EvalError = ErrorMessage | SourceEvaluator
    };

    // error(), warning() and message() from .pro file
    virtual void fileMessage(int type, const QString &msg) = 0;

    enum EvalFileType { EvalProjectFile, EvalIncludeFile, EvalConfigFile, EvalFeatureFile, EvalAuxFile };
    virtual void aboutToEval(ProFile *parent, ProFile *proFile, EvalFileType type) = 0;
    virtual void doneWithEval(ProFile *parent) = 0;
};

typedef QPair<QString, QString> QMakeFeatureKey; // key, parent
typedef QHash<QMakeFeatureKey, QString> QMakeFeatureHash;

class QMAKE_EXPORT QMakeFeatureRoots : public QSharedData
{
public:
    QMakeFeatureRoots(const QStringList &_paths) : paths(_paths) {}
    const QStringList paths;
    mutable QMakeFeatureHash cache;
#ifdef PROEVALUATOR_THREAD_SAFE
    mutable QMutex mutex;
#endif
};

// We use a QLinkedList based stack instead of a QVector based one (QStack), so that
// the addresses of value maps stay constant. The qmake generators rely on that.
class QMAKE_EXPORT ProValueMapStack : public QLinkedList<ProValueMap>
{
public:
    inline void push(const ProValueMap &t) { append(t); }
    inline ProValueMap pop() { return takeLast(); }
    ProValueMap &top() { return last(); }
    const ProValueMap &top() const { return last(); }
};

class QMAKE_EXPORT QMakeEvaluator
{
public:
    enum LoadFlag {
        LoadProOnly = 0,
        LoadPreFiles = 1,
        LoadPostFiles = 2,
        LoadAll = LoadPreFiles|LoadPostFiles,
        LoadSilent = 0x10,
        LoadHidden = 0x20
    };
    Q_DECLARE_FLAGS(LoadFlags, LoadFlag)

    static void initStatics();
    static void initFunctionStatics();
    QMakeEvaluator(QMakeGlobals *option, QMakeParser *parser, QMakeVfs *vfs,
                   QMakeHandler *handler);
    ~QMakeEvaluator();

    void setExtraVars(const ProValueMap &extraVars) { m_extraVars = extraVars; }
    void setExtraConfigs(const ProStringList &extraConfigs) { m_extraConfigs = extraConfigs; }
    void setOutputDir(const QString &outputDir) { m_outputDir = outputDir; }

    ProStringList values(const ProKey &variableName) const;
    ProStringList &valuesRef(const ProKey &variableName);
    ProString first(const ProKey &variableName) const;
    ProString propertyValue(const ProKey &val) const;

    ProString dirSep() const { return m_dirSep; }
    bool isHostBuild() const { return m_hostBuild; }

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
    VisitReturn evaluateExpression(const ushort *&tokPtr, ProStringList *ret, bool joined);
    static ALWAYS_INLINE void skipStr(const ushort *&tokPtr);
    static ALWAYS_INLINE void skipHashStr(const ushort *&tokPtr);
    void skipExpression(const ushort *&tokPtr);

    void loadDefaults();
    bool prepareProject(const QString &inDir);
    bool loadSpecInternal();
    bool loadSpec();
    void initFrom(const QMakeEvaluator *other);
    void setupProject();
    void evaluateCommand(const QString &cmds, const QString &where);
    void applyExtraConfigs();
    VisitReturn visitProFile(ProFile *pro, QMakeHandler::EvalFileType type,
                             LoadFlags flags);
    VisitReturn visitProBlock(ProFile *pro, const ushort *tokPtr);
    VisitReturn visitProBlock(const ushort *tokPtr);
    VisitReturn visitProLoop(const ProKey &variable, const ushort *exprPtr,
                             const ushort *tokPtr);
    void visitProFunctionDef(ushort tok, const ProKey &name, const ushort *tokPtr);
    VisitReturn visitProVariable(ushort tok, const ProStringList &curr, const ushort *&tokPtr);

    ALWAYS_INLINE const ProKey &map(const ProString &var) { return map(var.toKey()); }
    const ProKey &map(const ProKey &var);
    ProValueMap *findValues(const ProKey &variableName, ProValueMap::Iterator *it);

    void setTemplate();

    ProStringList split_value_list(const QStringRef &vals, int source = 0);
    VisitReturn expandVariableReferences(const ushort *&tokPtr, int sizeHint, ProStringList *ret, bool joined);

    QString currentFileName() const;
    QString currentDirectory() const;
    ProFile *currentProFile() const;
    int currentFileId() const;
    QString resolvePath(const QString &fileName) const
        { return QMakeInternal::IoUtils::resolvePath(currentDirectory(), fileName); }

    VisitReturn evaluateFile(const QString &fileName, QMakeHandler::EvalFileType type,
                             LoadFlags flags);
    VisitReturn evaluateFileChecked(const QString &fileName, QMakeHandler::EvalFileType type,
                                    LoadFlags flags);
    VisitReturn evaluateFeatureFile(const QString &fileName, bool silent = false);
    VisitReturn evaluateFileInto(const QString &fileName,
                                 ProValueMap *values, // output-only
                                 LoadFlags flags);
    VisitReturn evaluateConfigFeatures();
    void message(int type, const QString &msg) const;
    void evalError(const QString &msg) const
            { message(QMakeHandler::EvalError, msg); }
    void languageWarning(const QString &msg) const
            { message(QMakeHandler::EvalWarnLanguage, msg); }
    void deprecationWarning(const QString &msg) const
            { message(QMakeHandler::EvalWarnDeprecated, msg); }

    VisitReturn prepareFunctionArgs(const ushort *&tokPtr, QList<ProStringList> *ret);
    VisitReturn evaluateFunction(const ProFunctionDef &func,
                                 const QList<ProStringList> &argumentsList, ProStringList *ret);
    VisitReturn evaluateBoolFunction(const ProFunctionDef &func,
                                     const QList<ProStringList> &argumentsList,
                                     const ProString &function);

    VisitReturn evaluateExpandFunction(const ProKey &function, const ushort *&tokPtr, ProStringList *ret);
    VisitReturn evaluateConditionalFunction(const ProKey &function, const ushort *&tokPtr);

    VisitReturn evaluateBuiltinExpand(int func_t, const ProKey &function, const ProStringList &args, ProStringList &ret);
    VisitReturn evaluateBuiltinConditional(int func_t, const ProKey &function, const ProStringList &args);

    VisitReturn evaluateConditional(const QStringRef &cond, const QString &where, int line = -1);
#ifdef PROEVALUATOR_FULL
    VisitReturn checkRequirements(const ProStringList &deps);
#endif

    void updateMkspecPaths();
    void updateFeaturePaths();

    bool isActiveConfig(const QStringRef &config, bool regex = false);

    void populateDeps(
            const ProStringList &deps, const ProString &prefix, const ProStringList &suffixes,
            const ProString &priosfx,
            QHash<ProKey, QSet<ProKey> > &dependencies, ProValueMap &dependees,
            QMultiMap<int, ProString> &rootSet) const;

    bool getMemberArgs(const ProKey &name, int srclen, const ProStringList &args,
                       int *start, int *end);
    VisitReturn parseJsonInto(const QByteArray &json, const QString &into, ProValueMap *value);

    VisitReturn writeFile(const QString &ctx, const QString &fn, QIODevice::OpenMode mode,
                          QMakeVfs::VfsFlags flags, const QString &contents);
#ifndef QT_BOOTSTRAPPED
    void runProcess(QProcess *proc, const QString &command) const;
#endif
    QByteArray getCommandOutput(const QString &args, int *exitCode) const;

    QMakeEvaluator *m_caller;
#ifdef PROEVALUATOR_CUMULATIVE
    bool m_cumulative;
    int m_skipLevel;
#else
    enum { m_cumulative = 0 };
    enum { m_skipLevel = 0 };
#endif

    static QString quoteValue(const ProString &val);

#ifdef PROEVALUATOR_DEBUG
    void debugMsgInternal(int level, const char *fmt, ...) const;
    void traceMsgInternal(const char *fmt, ...) const;
    static QString formatValue(const ProString &val, bool forceQuote = false);
    static QString formatValueList(const ProStringList &vals, bool commas = false);
    static QString formatValueListList(const QList<ProStringList> &vals);

    const int m_debugLevel;
#else
    ALWAYS_INLINE void debugMsgInternal(int, const char *, ...) const {}
    ALWAYS_INLINE void traceMsgInternal(const char *, ...) const {}

    enum { m_debugLevel = 0 };
#endif

    struct Location {
        Location() : pro(0), line(0) {}
        Location(ProFile *_pro, ushort _line) : pro(_pro), line(_line) {}
        void clear() { pro = 0; line = 0; }
        ProFile *pro;
        ushort line;
    };

    Location m_current; // Currently evaluated location
    QStack<Location> m_locationStack; // All execution location changes
    QStack<ProFile *> m_profileStack; // Includes only

    ProValueMap m_extraVars;
    ProStringList m_extraConfigs;
    QString m_outputDir;

    int m_listCount;
    int m_toggle;
    bool m_valuemapInited;
    bool m_hostBuild;
    QString m_qmakespec;
    QString m_qmakespecName;
    QString m_superfile;
    QString m_conffile;
    QString m_cachefile;
    QString m_stashfile;
    QString m_sourceRoot;
    QString m_buildRoot;
    QStringList m_qmakepath;
    QStringList m_qmakefeatures;
    QStringList m_mkspecPaths;
    QExplicitlySharedDataPointer<QMakeFeatureRoots> m_featureRoots;
    ProString m_dirSep;
    ProFunctionDefs m_functionDefs;
    ProStringList m_returnValue;
    ProValueMapStack m_valuemapStack; // VariableName must be us-ascii, the content however can be non-us-ascii.
    QString m_tmp1, m_tmp2, m_tmp3, m_tmp[2]; // Temporaries for efficient toQString

    QMakeGlobals *m_option;
    QMakeParser *m_parser;
    QMakeHandler *m_handler;
    QMakeVfs *m_vfs;
};
Q_DECLARE_TYPEINFO(QMakeEvaluator::Location, Q_PRIMITIVE_TYPE);

Q_DECLARE_OPERATORS_FOR_FLAGS(QMakeEvaluator::LoadFlags)

QT_END_NAMESPACE
