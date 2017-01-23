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

class QMakeBaseKey
{
public:
    QMakeBaseKey(const QString &_root, const QString &_stash, bool _hostBuild);

    QString root;
    QString stash;
    bool hostBuild;
};

uint qHash(const QMakeBaseKey &key);
bool operator==(const QMakeBaseKey &one, const QMakeBaseKey &two);

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
    QMakeCmdLineParserState(const QString &_pwd) : pwd(_pwd), after(false) {}
    QString pwd;
    QStringList precmds, preconfigs, postcmds, postconfigs, extraargs;
    bool after;

    void flush() { after = false; }
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
    QString qmake_abslocation;
    QStringList qmake_args;

    QString qtconf;
    QString qmakespec, xqmakespec;
    QString user_template, user_template_prefix;
    QString precmds, postcmds;

#ifdef PROEVALUATOR_DEBUG
    int debugLevel;
#endif

    enum ArgumentReturn { ArgumentUnknown, ArgumentMalformed, ArgumentsOk };
    ArgumentReturn addCommandLineArguments(QMakeCmdLineParserState &state,
                                           QStringList &args, int *pos);
    void commitCommandLineArguments(QMakeCmdLineParserState &state);
    void setCommandLineArguments(const QString &pwd, const QStringList &args);
    void useEnvironment();
    void setDirectories(const QString &input_dir, const QString &output_dir);
#ifdef QT_BUILD_QMAKE
    void setQMakeProperty(QMakeProperty *prop) { property = prop; }
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
#endif
    QHash<QMakeBaseKey, QMakeBaseEnv *> baseEnvs;

    friend class QMakeEvaluator;
};

QT_END_NAMESPACE
