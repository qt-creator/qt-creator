/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
