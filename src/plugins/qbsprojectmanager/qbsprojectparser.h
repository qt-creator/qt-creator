/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

    void parse(const QVariantMap &config, const Utils::Environment &env, const QString &dir);
    void cancel();

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
    QString libExecDirectory() const;

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
