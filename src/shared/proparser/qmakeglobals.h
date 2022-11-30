// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmake_global.h"
#include "proitems.h"

#ifdef QT_BUILD_QMAKE
#  include <property.h>
#endif

#include <qhash.h>
#include <qstringlist.h>
#ifndef QT_BOOTSTRAPPED
# include <qprocess.h>
#endif
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qmutex.h>
# include <qwaitcondition.h>
#endif

QT_BEGIN_NAMESPACE

class QMakeEvaluator;

enum QMakeEvalPhase { QMakeEvalEarly, QMakeEvalBefore, QMakeEvalAfter, QMakeEvalLate };

class QMakeBaseKey
{
public:
    QMakeBaseKey(const QString &_root, const QString &_stash, bool _hostBuild);

    friend size_t qHash(const QMakeBaseKey &key);
    friend bool operator==(const QMakeBaseKey &one, const QMakeBaseKey &two);

    QString root;
    QString stash;
    bool hostBuild;
};

class QMakeBaseEnv
{
public:
    QMakeBaseEnv();
    ~QMakeBaseEnv();

#ifdef PROEVALUATOR_THREAD_SAFE
    QMutex mutex;
    QWaitCondition cond;
    bool inProgress;
    // The coupling of this flag to thread safety exists because for other
    // use cases failure is immediately fatal anyway.
    bool isOk;
#endif
    QMakeEvaluator *evaluator;
};

class QMAKE_EXPORT QMakeCmdLineParserState
{
public:
    QMakeCmdLineParserState(const QString &_pwd) : pwd(_pwd), phase(QMakeEvalBefore) {}
    QString pwd;
    QStringList cmds[4], configs[4];
    QStringList extraargs;
    QMakeEvalPhase phase;

    void flush() { phase = QMakeEvalBefore; }
};

class QMAKE_EXPORT QMakeGlobals
{
public:
    QMakeGlobals();
    ~QMakeGlobals();

    bool do_cache;
    QString dir_sep;
    QString dirlist_sep;
    QString cachefile;
#ifdef PROEVALUATOR_SETENV
    QProcessEnvironment environment;
#endif
    QString device_root;
    QString qmake_abslocation;
    QStringList qmake_args, qmake_extra_args;

    QString qtconf;
    QString qmakespec, xqmakespec;
    QString user_template, user_template_prefix;
    QString extra_cmds[4];
    bool runSystemFunction = false;

    void killProcesses();

#ifdef PROEVALUATOR_DEBUG
    int debugLevel;
#endif

    enum ArgumentReturn { ArgumentUnknown, ArgumentMalformed, ArgumentsOk };
    ArgumentReturn addCommandLineArguments(QMakeCmdLineParserState &state,
                                           QStringList &args, int *pos);
    void commitCommandLineArguments(QMakeCmdLineParserState &state);
    void setCommandLineArguments(const QString &pwd, const QStringList &args);
    void useEnvironment();
    void setDirectories(const QString &input_dir, const QString &output_dir,
                        const QString &device_root);
#ifdef QT_BUILD_QMAKE
    void setQMakeProperty(QMakeProperty *prop) { property = prop; }
    void reloadProperties() { property->reload(); }
    ProString propertyValue(const ProKey &name) const { return property->value(name); }
#else
    static void parseProperties(const QByteArray &data, QHash<ProKey, ProString> &props);
#  ifdef PROEVALUATOR_INIT_PROPS
    bool initProperties();
#  else
    void setProperties(const QHash<ProKey, ProString> &props) { properties = props; }
#  endif
    ProString propertyValue(const ProKey &name) const { return properties.value(name); }
#endif

    QString expandEnvVars(const QString &str) const;
    QString shadowedPath(const QString &fileName) const;
    QStringList splitPathList(const QString &value) const;

private:
    QString getEnv(const QString &) const;
    QStringList getPathListEnv(const QString &var) const;

    QString cleanSpec(QMakeCmdLineParserState &state, const QString &spec);

    QString source_root, build_root;

#ifdef QT_BUILD_QMAKE
    QMakeProperty *property;
#else
    QHash<ProKey, ProString> properties;
#endif

#ifdef PROEVALUATOR_THREAD_SAFE
    QMutex mutex;
    std::atomic_bool canceled = false;
#endif
    QHash<QMakeBaseKey, QMakeBaseEnv *> baseEnvs;

    friend class QMakeEvaluator;
};

QT_END_NAMESPACE
