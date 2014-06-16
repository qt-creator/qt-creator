/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QBSPROJECTPARSER_H
#define QBSPROJECTPARSER_H

#include <utils/environment.h>

#include <QFutureInterface>
#include <QObject>

#include <qbs.h>

namespace QbsProjectManager {
namespace Internal {

class QbsProject;

class QbsProjectParser : public QObject
{
    Q_OBJECT

public:
    QbsProjectParser(QbsProjectManager::Internal::QbsProject *project,
                     QFutureInterface<bool> *fi);
    ~QbsProjectParser();

    void setForced(bool);

    bool parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir);

    qbs::Project qbsProject() const;
    qbs::ErrorInfo error();

signals:
    void done(bool success);

private slots:
    void handleQbsParsingDone(bool success);
    void handleQbsParsingProgress(int progress);
    void handleQbsParsingTaskSetup(const QString &description, int maximumProgressValue);

private:
    QString pluginsBaseDirectory() const;
    QString resourcesBaseDirectory() const;

    bool m_wasForced;
    QString m_projectFilePath;
    qbs::SetupProjectJob *m_qbsSetupProjectJob;
    qbs::ErrorInfo m_error;
    qbs::Project m_project;

    QFutureInterface<bool> *m_fi;
    int m_currentProgressBase;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSPROJECTPARSER_H
