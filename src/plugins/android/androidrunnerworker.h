// Copyright (C) 2018 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <solutions/tasking/tasktreerunner.h>

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

    // business logic init getters
    ProjectExplorer::RunControl *runControl() const { return m_runControl; }
    QString deviceSerialNumber() const { return m_deviceSerialNumber; }
    int apiLevel() const { return m_apiLevel; }

    // GUI -> business logic
    void cancel() { emit canceled(); }

    // business logic -> GUI
    void setStarted(const Utils::Port &debugServerPort, const QUrl &qmlServer, qint64 pid);
    void setFinished(const QString &errorMessage) { emit finished(errorMessage); }
    void addStdOut(const QString &data) { emit stdOut(data); }
    void addStdErr(const QString &data) { emit stdErr(data); }

signals:
    // GUI -> business logic
    void canceled();

    // business logic -> GUI
    void started(const Utils::Port &debugServerPort, const QUrl &qmlServer, qint64 pid);
    void finished(const QString &errorMessage);
    void stdOut(const QString &data);
    void stdErr(const QString &data);

private:
    ProjectExplorer::RunControl *m_runControl = nullptr;
    QString m_deviceSerialNumber;
    int m_apiLevel = -1;
};

Tasking::ExecutableItem runnerRecipe(const Tasking::Storage<RunnerInterface> &storage);

} // namespace Android::Internal
