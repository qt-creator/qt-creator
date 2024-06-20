// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllssettings.h"
#include "qmljseditorsettings.h"

#include <utils/hostosinfo.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QLoggingCategory>
#include <qtsupport/qtversionmanager.h>

#include <nanotrace/nanotrace.h>

#include <limits>

using namespace QtSupport;
using namespace Utils;

namespace QmlJSEditor::Internal {

namespace {
Q_LOGGING_CATEGORY(qmllsLog, "qtc.qmlls.settings", QtWarningMsg);
}

static FilePath evaluateLatestQmlls()
{
    // find latest qmlls, i.e. vals
    if (!QtVersionManager::isLoaded())
        return {};
    const QtVersions versions = QtVersionManager::versions();
    FilePath latestQmlls;
    QVersionNumber latestVersion;
    FilePath latestQmakeFilePath;
    int latestUniqueId = std::numeric_limits<int>::min();
    const bool ignoreMinimumQmllsVersion
        = QmllsSettingsManager::instance()->ignoreMinimumQmllsVersion();
    for (QtVersion *v : versions) {
        // check if we find qmlls
        QVersionNumber vNow = v->qtVersion();
        if (!ignoreMinimumQmllsVersion && vNow < QmllsSettingsManager::mininumQmllsVersion)
            continue;
        FilePath qmllsNow = QmlJS::ModelManagerInterface::qmllsForBinPath(v->hostBinPath(), vNow);
        if (!qmllsNow.isExecutableFile())
            continue;
        if (latestVersion > vNow)
            continue;
        FilePath qmakeNow = v->qmakeFilePath();
        int uniqueIdNow = v->uniqueId();
        if (latestVersion == vNow) {
            if (latestQmakeFilePath > qmakeNow)
                continue;
            if (latestQmakeFilePath == qmakeNow && latestUniqueId >= v->uniqueId())
                continue;
        }
        latestVersion = vNow;
        latestQmlls = qmllsNow;
        latestQmakeFilePath = qmakeNow;
        latestUniqueId = uniqueIdNow;
    }
    return latestQmlls;
}

QmllsSettingsManager *QmllsSettingsManager::instance()
{
    static QmllsSettingsManager *manager = new QmllsSettingsManager;
    return manager;
}

FilePath QmllsSettingsManager::latestQmlls()
{
    QMutexLocker l(&m_mutex);
    return m_latestQmlls;
}

void QmllsSettingsManager::setupAutoupdate()
{
    QObject::connect(QtVersionManager::instance(),
                     &QtVersionManager::qtVersionsChanged,
                     this,
                     &QmllsSettingsManager::checkForChanges);
    if (QtVersionManager::isLoaded())
        checkForChanges();
    else
        QObject::connect(QtVersionManager::instance(),
                         &QtVersionManager::qtVersionsLoaded,
                         this,
                         &QmllsSettingsManager::checkForChanges);
}

void QmllsSettingsManager::checkForChanges()
{
    const QmlJsEditingSettings &newSettings = settings();
    FilePath newLatest = newSettings.useLatestQmlls() && newSettings.useQmlls()
            ? evaluateLatestQmlls() : m_latestQmlls;
    if (m_useQmlls == newSettings.useQmlls()
        && m_useLatestQmlls == newSettings.useLatestQmlls()
        && m_disableBuiltinCodemodel == newSettings.disableBuiltinCodemodel()
        && m_generateQmllsIniFiles == newSettings.generateQmllsIniFiles()
        && newLatest == m_latestQmlls)
        return;
    qCDebug(qmllsLog) << "qmlls settings changed:" << newSettings.useQmlls()
                      << newSettings.useLatestQmlls() << newLatest;
    {
        QMutexLocker l(&m_mutex);
        m_latestQmlls = newLatest;
        m_useQmlls = newSettings.useQmlls();
        m_useLatestQmlls = newSettings.useLatestQmlls();
        m_disableBuiltinCodemodel = newSettings.disableBuiltinCodemodel();
        m_generateQmllsIniFiles = newSettings.generateQmllsIniFiles();
    }
    emit settingsChanged();
}

bool QmllsSettingsManager::useLatestQmlls() const
{
    return m_useLatestQmlls;
}

bool QmllsSettingsManager::ignoreMinimumQmllsVersion() const
{
    return m_ignoreMinimumQmllsVersion;
}

bool QmllsSettingsManager::useQmlls() const
{
    return m_useQmlls;
}

} // QmlJSEditor::Internal
