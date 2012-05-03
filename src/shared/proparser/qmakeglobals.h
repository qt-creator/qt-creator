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

class QMAKE_EXPORT QMakeGlobals
{
public:
    QMakeGlobals();
    ~QMakeGlobals();

    QString dir_sep;
    QString dirlist_sep;
    QString qmakespec;
    QString cachefile;
    QHash<QString, QString> properties;
#ifndef QT_BOOTSTRAPPED
    QProcessEnvironment environment;
#endif
    QString sysroot;

    // -nocache, -cache, -spec, QMAKESPEC
    // -set persistent value
    void setCommandLineArguments(const QStringList &args);
#ifdef PROEVALUATOR_INIT_PROPS
    bool initProperties(const QString &qmake);
#endif

private:
    QString getEnv(const QString &) const;

    QHash<ProString, ProStringList> base_valuemap; // Cached results of qmake.conf, .qmake.cache & default_pre.prf
    ProFunctionDefs base_functions;
    QStringList feature_roots;
    QString qmakespec_name;
    QString precmds, postcmds;
    enum TARG_MODE { TARG_UNKNOWN_MODE, TARG_UNIX_MODE, TARG_WIN_MODE, TARG_MACX_MODE,
                     TARG_SYMBIAN_MODE };
    TARG_MODE target_mode;
#ifdef PROEVALUATOR_THREAD_SAFE
    QMutex mutex;
    QWaitCondition cond;
    bool base_inProgress;
#endif

    friend class QMakeEvaluator;
};

QT_END_NAMESPACE

#endif // QMAKEGLOBALS_H
