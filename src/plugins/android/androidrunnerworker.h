// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtTaskTree/QTaskTree>

#include <projectexplorer/qmldebugcommandlinearguments.h>

namespace ProjectExplorer { class RunControl; }
namespace Utils { class Port; }

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Android::Internal {

class RunnerInterface : public QObject
{
    Q_OBJECT

public:
    // Gui init setters
    void setRunControl(ProjectExplorer::RunControl *runControl) { m_runControl = runControl; }
    void setDeviceSerialNumber(const QString &deviceSerialNumber) { m_deviceSerialNumber = deviceSerialNumber; }
    void setApiLevel(int apiLevel) { m_apiLevel = apiLevel; }
    void setQmlDebugServicesPreset(ProjectExplorer::QmlDebugServicesPreset services) { m_qmlDebugServices = services; }

    // business logic init getters
    ProjectExplorer::RunControl *runControl() const { return m_runControl; }
    QString deviceSerialNumber() const { return m_deviceSerialNumber; }
    int apiLevel() const { return m_apiLevel; }
    bool wasCancelled() const { return m_wasCancelled; };
    ProjectExplorer::QmlDebugServicesPreset qmlDebugServicesPreset() const { return m_qmlDebugServices; }

    // business logic -> GUI
    void setStartData(qint64 pid, const QString &packageDir);

    // GUI -> business logic
    void cancel();

signals:
    // GUI -> business logic
    void canceled();

    // business logic -> GUI
    void started();
    void finished(const QString &errorMessage);

private:
    ProjectExplorer::RunControl *m_runControl = nullptr;
    QString m_deviceSerialNumber;
    bool m_wasCancelled = false;
    int m_apiLevel = -1;
    ProjectExplorer::QmlDebugServicesPreset m_qmlDebugServices = ProjectExplorer::NoQmlDebugServices;
};

QtTaskTree::ExecutableItem runnerRecipe(const QtTaskTree::Storage<RunnerInterface> &storage);

} // namespace Android::Internal
