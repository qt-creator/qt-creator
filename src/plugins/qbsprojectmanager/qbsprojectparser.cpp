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

void QbsProjectParser::parse(const QVariantMap &config, const Environment &env, const QString &dir)
{
    QTC_ASSERT(!m_qbsSetupProjectJob, return);
    QTC_ASSERT(!dir.isEmpty(), return);

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

    // Some people don't like it when files are created as a side effect of opening a project,
    // so do not store the build graph if the build directory does not exist yet.
    params.setDryRun(!QFileInfo::exists(dir));

    params.setBuildRoot(dir);
    params.setProjectFilePath(m_projectFilePath);
    params.setIgnoreDifferentProjectFilePath(false);
    params.setEnvironment(env.toProcessEnvironment());
    const qbs::Preferences prefs(QbsManager::settings(), profileName);
    params.setSearchPaths(prefs.searchPaths(resourcesBaseDirectory()));
    params.setPluginPaths(prefs.pluginPaths(pluginsBaseDirectory()));
    params.setLibexecPath(libExecDirectory());

    m_qbsSetupProjectJob = m_project.setupProject(params, QbsManager::logSink(), 0);

    connect(m_qbsSetupProjectJob, SIGNAL(finished(bool,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingDone(bool)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskStarted(QString,int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingTaskSetup(QString,int)));
    connect(m_qbsSetupProjectJob, SIGNAL(taskProgress(int,qbs::AbstractJob*)),
            this, SLOT(handleQbsParsingProgress(int)));
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

QString QbsProjectParser::libExecDirectory() const
{
    const QString qbsInstallDir = QLatin1String(QBS_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir + QLatin1String("/libexec");
    return Core::ICore::libexecPath();
}

QString QbsProjectParser::pluginsBaseDirectory() const
{
    const QString qbsInstallDir = QLatin1String(QBS_INSTALL_DIR);
    if (!qbsInstallDir.isEmpty())
        return qbsInstallDir + QLatin1String("/lib/");
    if (HostOsInfo::isMacHost())
        return QDir::cleanPath(QCoreApplication::applicationDirPath()
                               + QLatin1String("/../PlugIns"));
    else
        return QDir::cleanPath(QCoreApplication::applicationDirPath()
                               + QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator/plugins"));
}

} // namespace Internal
} // namespace QbsProjectManager
