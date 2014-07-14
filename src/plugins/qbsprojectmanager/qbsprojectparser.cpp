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

#include "qbsprojectparser.h"

#include "qbslogsink.h"
#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <qbs.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsProjectParser:
// --------------------------------------------------------------------

QbsProjectParser::QbsProjectParser(QbsProject *project, QFutureInterface<bool> *fi) :
    m_qbsSetupProjectJob(0),
    m_fi(fi),
    m_currentProgressBase(0)
{
    m_project = project->qbsProject();
    m_projectFilePath = project->projectFilePath().toString();
}

QbsProjectParser::~QbsProjectParser()
{
    if (m_qbsSetupProjectJob) {
        m_qbsSetupProjectJob->disconnect(this);
        m_qbsSetupProjectJob->cancel();
        m_qbsSetupProjectJob->deleteLater();
        m_qbsSetupProjectJob = 0;
    }
    m_fi = 0; // we do not own m_fi, do not delete
}

void QbsProjectParser::setForced(bool f)
{
    m_wasForced = f;
}

bool QbsProjectParser::parse(const QVariantMap &config, const Environment &env, const QString &dir)
{
    QTC_ASSERT(!m_qbsSetupProjectJob, return false);
    QTC_ASSERT(!dir.isNull(), return false);

    m_currentProgressBase = 0;

    qbs::SetupProjectParameters params;
    QVariantMap userConfig = config;
    QString specialKey = QLatin1String(Constants::QBS_CONFIG_PROFILE_KEY);
    const QString profileName = userConfig.take(specialKey).toString();
    params.setTopLevelProfile(profileName);
    specialKey = QLatin1String(Constants::QBS_CONFIG_VARIANT_KEY);
    params.setBuildVariant(userConfig.take(specialKey).toString());
    params.setSettingsDirectory(QbsManager::settings()->baseDirectoy());
    params.setOverriddenValues(userConfig);
    m_error = params.expandBuildConfiguration();
    if (m_error.hasError()) {
        emit done(false);
        return false;
    }

    // Avoid useless reparsing:
    if (!m_wasForced
            && m_project.isValid()
            && m_project.projectConfiguration() == params.finalBuildConfigurationTree()) {
        QHash<QString, QString> usedEnv = m_project.usedEnvironment();
        for (QHash<QString, QString>::const_iterator i = usedEnv.constBegin();
             i != usedEnv.constEnd(); ++i) {
            if (env.value(i.key()) != i.value()) {
                emit done(true);
                return true;
            }
        }
    }

    // Some people don't like it when files are created as a side effect of opening a project,
    // so do not store the build graph if the build directory does not exist yet.
    params.setDryRun(!QFileInfo(dir).exists());

    params.setBuildRoot(dir);
    params.setProjectFilePath(m_projectFilePath);
    params.setIgnoreDifferentProjectFilePath(false);
    params.setEnvironment(env.toProcessEnvironment());
    const qbs::Preferences prefs(QbsManager::settings(), profileName);
    params.setSearchPaths(prefs.searchPaths(resourcesBaseDirectory()));
    params.setPluginPaths(prefs.pluginPaths(pluginsBaseDirectory()));

    m_qbsSetupProjectJob = qbs::Project::setupProject(params, QbsManager::logSink(), 0);

    connect(m_qbsSetupProjectJob, SIGNAL(finished(bool,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingDone(bool)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingTaskSetup(QString,int)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingProgress(int)));

    return true;
}

void QbsProjectParser::cancel()
{
    QTC_ASSERT(m_qbsSetupProjectJob, return);
    m_qbsSetupProjectJob->cancel();
}

qbs::Project QbsProjectParser::qbsProject() const
{
    return m_project;
}

qbs::ErrorInfo QbsProjectParser::error()
{
    return m_error;
}

void QbsProjectParser::handleQbsParsingDone(bool success)
{
    QTC_ASSERT(m_qbsSetupProjectJob, return);

    m_project = m_qbsSetupProjectJob->project();
    m_error = m_qbsSetupProjectJob->error();

    // Do not report the operation as canceled here, as we might want to make overlapping
    // parses appear atomic to the user.

    emit done(success);
}

void QbsProjectParser::handleQbsParsingProgress(int progress)
{
    if (m_fi)
        m_fi->setProgressValue(m_currentProgressBase + progress);
}

void QbsProjectParser::handleQbsParsingTaskSetup(const QString &description, int maximumProgressValue)
{
    Q_UNUSED(description);
    if (m_fi) {
        m_currentProgressBase = m_fi->progressValue();
        m_fi->setProgressRange(0, m_currentProgressBase + maximumProgressValue);
    }
}

QString QbsProjectParser::resourcesBaseDirectory() const
{
    const QString qbsInstallDir = QLatin1String(QBS_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir;
    return Core::ICore::resourcePath() + QLatin1String("/qbs");
}

QString QbsProjectParser::pluginsBaseDirectory() const
{
    const QString qbsInstallDir = QLatin1String(QBS_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir + QLatin1String("/lib/");
    if (Utils::HostOsInfo::isMacHost())
        return QDir::cleanPath(QCoreApplication::applicationDirPath()
                               + QLatin1String("/../PlugIns"));
    else
        return QDir::cleanPath(QCoreApplication::applicationDirPath()
                               + QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator"));
}

} // namespace Internal
} // namespace QbsProjectManager
