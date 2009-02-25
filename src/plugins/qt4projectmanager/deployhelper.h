/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DEPLOYHELPER_H
#define DEPLOYHELPER_H

#include <projectexplorer/buildstep.h>

#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

class Qt4Project;

namespace Internal {

class DeployHelperRunStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    DeployHelperRunStep(Qt4Project * pro);
    ~DeployHelperRunStep();

    virtual bool init(const QString &configuration);
    virtual void run(QFutureInterface<bool> &);

    virtual QString name();
    virtual QString displayName();

    virtual QString binary();
    virtual QString id();

    virtual ProjectExplorer::BuildStepConfigWidget *configWidget();

private:
    bool started();

    void stop();
    void cleanup();

private slots:
    void readyRead();
    void processFinished();

private:
    QString m_qtdir;
    QString m_appdir;
    QString m_exec;
    QString m_skin;
    QString m_binary;
    QStringList m_extraargs;
    QString m_id;
    bool m_started;
    Qt4Project *m_pro;
    QEventLoop *m_eventLoop;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // DEPLOYHELPER_H
