/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMAKEGLOBALS_H
#define QMAKEGLOBALS_H

#include "qmake_global.h"
#include "proitems.h"

#include <QHash>
#include <QStringList>
#ifndef QT_BOOTSTRAPPED
# include <QProcess>
#endif
#ifdef PROEVALUATOR_THREAD_SAFE
# include <QMutex>
# include <QWaitCondition>
#endif

QT_BEGIN_NAMESPACE

class QMakeEvaluator;

class QMakeBaseKey
{
public:
    QMakeBaseKey(const QString &_root, bool _hostBuild);

    QString root;
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

class QMAKE_EXPORT QMakeGlobals
{
public:
    QMakeGlobals();
    ~QMakeGlobals();

    bool do_cache;
    QString dir_sep;
    QString dirlist_sep;
    QString qmakespec;
    QString xqmakespec;
    QString cachefile;
#ifndef QT_BOOTSTRAPPED
    QProcessEnvironment environment;
#endif
    QString sysroot;
    QString qmake_abslocation;
    QString user_template, user_template_prefix;

    // -nocache, -cache, -spec, QMAKESPEC
    // -set persistent value
    void setCommandLineArguments(const QStringList &args);
    void setDirectories(const QString &input_dir, const QString &output_dir);
#ifdef PROEVALUATOR_INIT_PROPS
    bool initProperties();
#else
    void setProperties(const QHash<QString, QString> &props);
#endif
    ProString propertyValue(const ProKey &name) const { return properties.value(name); }

    QString expandEnvVars(const QString &str) const;

private:
    QString getEnv(const QString &) const;
    QStringList getPathListEnv(const QString &var) const;

    QString source_root, build_root;

    QString precmds, postcmds;
    QHash<ProKey, ProString> properties;

#ifdef PROEVALUATOR_THREAD_SAFE
    QMutex mutex;
#endif
    QHash<QMakeBaseKey, QMakeBaseEnv *> baseEnvs;

    friend class QMakeEvaluator;
};

QT_END_NAMESPACE

#endif // QMAKEGLOBALS_H
