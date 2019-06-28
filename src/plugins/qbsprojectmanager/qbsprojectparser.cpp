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

#include "qbsprojectparser.h"

#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"
#include "qbssettings.h"

#include <utils/qtcassert.h>

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>

using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// QbsProjectParser:
// --------------------------------------------------------------------

QbsProjectParser::QbsProjectParser(QbsBuildSystem *buildSystem, QFutureInterface<bool> *fi)
    : m_projectFilePath(buildSystem->project()->projectFilePath().toString()),
      m_session(buildSystem->session()),
      m_fi(fi)
{
    auto * const watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::canceled, this, &QbsProjectParser::cancel);
    watcher->setFuture(fi->future());
}

QbsProjectParser::~QbsProjectParser()
{
    if (m_session && m_parsing)
        m_session->cancelCurrentJob();
    m_fi = nullptr; // we do not own m_fi, do not delete
}

void QbsProjectParser::parse(const QVariantMap &config, const Environment &env, const QString &dir,
                             const QString &configName)
{
    QTC_ASSERT(m_session, return);
    QTC_ASSERT(!dir.isEmpty(), return);

    m_environment = env;
    QJsonObject request;
    request.insert("type", "resolve-project");
    QVariantMap userConfig = config;
    request.insert("top-level-profile",
                   userConfig.take(Constants::QBS_CONFIG_PROFILE_KEY).toString());
    request.insert("configuration-name", configName);
    request.insert("force-probe-execution",
                   userConfig.take(Constants::QBS_FORCE_PROBES_KEY).toBool());
    if (QbsSettings::useCreatorSettingsDirForQbs())
        request.insert("settings-directory", QbsSettings::qbsSettingsBaseDir());
    request.insert("overridden-properties", QJsonObject::fromVariantMap(userConfig));

    // People don't like it when files are created as a side effect of opening a project,
    // so do not store the build graph if the build directory does not exist yet.
    request.insert("dry-run", !QFileInfo::exists(dir));

    request.insert("build-root", dir);
    request.insert("project-file-path", m_projectFilePath);
    request.insert("override-build-graph-data", true);
    static const auto envToJson = [](const Environment &env) {
        QJsonObject envObj;
        for (auto it = env.constBegin(); it != env.constEnd(); ++it) {
            if (env.isEnabled(it))
                envObj.insert(env.key(it), env.value(it));
        }
        return envObj;
    };
    request.insert("environment", envToJson(env));
    request.insert("error-handling-mode", "relaxed");
    request.insert("data-mode", "only-if-changed");
    QbsSession::insertRequestedModuleProperties(request);

    connect(m_session, &QbsSession::projectResolved, this, [this](const ErrorInfo &error) {
        m_error = error;
        m_projectData = m_session->projectData();
        emit done(!m_error.hasError());
    });
    connect(m_session, &QbsSession::errorOccurred, this, [this] { emit done(false); });
    connect(m_session, &QbsSession::taskStarted, this,
            [this](const QString &description, int maxProgress) {
        Q_UNUSED(description)
        if (m_fi)
            m_fi->setProgressRange(0, maxProgress);
    });
    connect(m_session, &QbsSession::maxProgressChanged, this, [this](int maxProgress) {
        if (m_fi)
            m_fi->setProgressRange(0, maxProgress);
    });
    connect(m_session, &QbsSession::taskProgress, this, [this](int progress) {
        if (m_fi)
            m_fi->setProgressValue(progress);
    });
    m_session->sendRequest(request);
}

void QbsProjectParser::cancel()
{
    if (m_session)
        m_session->cancelCurrentJob();
}

} // namespace Internal
} // namespace QbsProjectManager
