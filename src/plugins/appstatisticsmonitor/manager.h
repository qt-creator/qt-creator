// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "chart.h"

#include "idataprovider.h"

#include <coreplugin/inavigationwidgetfactory.h>

#include <QComboBox>
#include <QList>
#include <QMap>

namespace Core { class IContext; }
namespace ProjectExplorer { class RunControl; }

namespace AppStatisticsMonitor::Internal {

class AppStatisticsMonitorManager : public QObject
{
    Q_OBJECT
public:
    AppStatisticsMonitorManager();

    QString nameForPid(qint64 pid) const;
    QHash<qint64, QString> pidNameMap() const;

    double memoryConsumption(qint64 pid) const;
    double cpuConsumption(qint64 pid) const;

    IDataProvider *currentDataProvider() const;
    void setCurrentDataProvider(qint64 pid);

signals:
    void newDataAvailable();
    void appStarted(const QString &name, qint64 pid);
    void appStoped(qint64 pid);

private:
    QHash<qint64, QString> m_pidNameMap;
    QHash<ProjectExplorer::RunControl *, int> m_rcPidMap;

    QMap<qint64, IDataProvider *> m_pidDataProviders;
    IDataProvider *m_currentDataProvider;
};

class AppStatisticsMonitorViewFactory : public Core::INavigationWidgetFactory
{
public:
    AppStatisticsMonitorViewFactory(AppStatisticsMonitorManager *appStatisticManager);

private:
    Core::NavigationView createWidget() override;

    AppStatisticsMonitorManager *m_manager;
};

} // namespace AppStatisticsMonitor::Internal
