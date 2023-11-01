// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllssettings.h"
#include "qmljseditingsettingspage.h"

#include <utils/hostosinfo.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QLoggingCategory>
#include <qtsupport/qtversionmanager.h>

#include <nanotrace/nanotrace.h>

#include <limits>

using namespace QtSupport;
using namespace Utils;

namespace QmlJSEditor {

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
    for (QtVersion *v : versions) {
        // check if we find qmlls
        QVersionNumber vNow = v->qtVersion();
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

QmllsSettings QmllsSettingsManager::lastSettings()
{
    QMutexLocker l(&m_mutex);
    return m_lastSettings;
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
    QmllsSettings newSettings = QmlJsEditingSettings::get().qmllsSettings();
    FilePath newLatest = newSettings.useLatestQmlls && newSettings.useQmlls ? evaluateLatestQmlls()
                                                                            : m_latestQmlls;
    if (m_lastSettings == newSettings && newLatest == m_latestQmlls)
        return;
    qCDebug(qmllsLog) << "qmlls settings changed:" << newSettings.useQmlls
                      << newSettings.useLatestQmlls << newLatest;
    {
        QMutexLocker l(&m_mutex);
        m_latestQmlls = newLatest;
        m_lastSettings = newSettings;
    }
    emit settingsChanged();
}

} // namespace QmlJSEditor
