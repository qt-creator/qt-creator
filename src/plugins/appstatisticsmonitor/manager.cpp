// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "manager.h"

#include "chart.h"
#include "idataprovider.h"

#include <coreplugin/session.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runcontrol.h>

#include <QFormLayout>
#include <QGraphicsItem>
#include <QHash>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppStatisticsMonitor::Internal {

class AppStatisticsMonitorView : public QWidget
{
public:
    explicit AppStatisticsMonitorView(
        AppStatisticsMonitorManager *appStatisticManager, QWidget *parent = nullptr);

    ~AppStatisticsMonitorView() override;

private:
    QComboBox *m_comboBox;

    Chart *m_memChart;
    Chart *m_cpuChart;

    AppStatisticsMonitorManager *m_manager;
};

AppStatisticsMonitorManager::AppStatisticsMonitorManager()
{
    connect(
        ProjectExplorer::ProjectExplorerPlugin::instance(),
        &ProjectExplorer::ProjectExplorerPlugin::runControlStarted,
        this,
        [this](RunControl *runControl) {
            qint64 pid = runControl->applicationProcessHandle().pid();

            m_pidNameMap[pid] = runControl->displayName();
            m_rcPidMap[runControl] = pid;

            m_currentDataProvider = createDataProvider(pid);
            m_pidDataProviders.insert(pid, m_currentDataProvider);

            emit appStarted(runControl->displayName(), pid);
        });

    connect(
        ProjectExplorer::ProjectExplorerPlugin::instance(),
        &ProjectExplorer::ProjectExplorerPlugin::runControlStoped,
        this,
        [this](RunControl *runControl) {
            const auto pidIt = m_rcPidMap.constFind(runControl);
            if (pidIt == m_rcPidMap.constEnd())
                return;

            const qint64 pid = pidIt.value();
            m_rcPidMap.erase(pidIt);

            m_pidNameMap.remove(pid);
            delete m_pidDataProviders[pid];
            m_pidDataProviders.remove(pid);
            if (m_pidDataProviders.isEmpty())
                setCurrentDataProvider(-1);
            else
                setCurrentDataProvider(m_pidDataProviders.keys().last());

            emit appStoped(pid);
        });
}

QString AppStatisticsMonitorManager::nameForPid(qint64 pid) const
{
    const auto pidIt = m_pidNameMap.constFind(pid);
    if (pidIt == m_pidNameMap.constEnd())
        return {};

    return pidIt.value();
}

IDataProvider *AppStatisticsMonitorManager::currentDataProvider() const
{
    return m_currentDataProvider;
}

void AppStatisticsMonitorManager::setCurrentDataProvider(qint64 pid)
{
    m_currentDataProvider = nullptr;
    const auto pidIt = m_pidDataProviders.constFind(pid);
    if (pidIt == m_pidDataProviders.constEnd())
        return;

    m_currentDataProvider = pidIt.value();
    connect(
        m_currentDataProvider,
        &IDataProvider::newDataAvailable,
        this,
        &AppStatisticsMonitorManager::newDataAvailable);
}

QHash<qint64, QString> AppStatisticsMonitorManager::pidNameMap() const
{
    return m_pidNameMap;
}

// AppStatisticsMonitorView
AppStatisticsMonitorView::AppStatisticsMonitorView(AppStatisticsMonitorManager *dapManager, QWidget *)
    : m_manager(dapManager)
{
    auto layout = new QVBoxLayout;
    auto form = new QFormLayout;
    setLayout(layout);

    m_comboBox = new QComboBox();
    form->addRow(m_comboBox);

    m_memChart = new Chart("Memory consumption ");
    m_memChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_memChart->clear();
    form->addRow(m_memChart);

    m_cpuChart = new Chart("CPU consumption ");
    m_cpuChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_cpuChart->clear();
    form->addRow(m_cpuChart);

    layout->addLayout(form);

    for (auto pidName : m_manager->pidNameMap().asKeyValueRange()) {
        qint64 pid = pidName.first;
        m_comboBox->addItem(pidName.second + " : " + QString::number(pid), pid);
    }
    m_comboBox->setCurrentIndex(m_comboBox->count() - 1);

    m_memChart->clear();
    m_cpuChart->clear();

    auto updateCharts = [this](int index) {
        m_manager->setCurrentDataProvider(m_comboBox->itemData(index).toLongLong());
        if (m_manager->currentDataProvider() != nullptr) {
            m_memChart->loadNewProcessData(
                m_manager->currentDataProvider()->memoryConsumptionHistory());
            m_cpuChart->loadNewProcessData(
                m_manager->currentDataProvider()->cpuConsumptionHistory());
        }
    };

    if (m_comboBox->count() != 0)
        updateCharts(m_comboBox->currentIndex());

    connect(m_comboBox, &QComboBox::currentIndexChanged, this, [updateCharts](int index) {
        updateCharts(index);
    });

    connect(
        m_manager,
        &AppStatisticsMonitorManager::appStarted,
        this,
        [this](const QString &name, qint64 pid) {
            if (pid != m_comboBox->currentData()) {
                m_comboBox->addItem(name + " : " + QString::number(pid), pid);

                m_memChart->clear();
                m_cpuChart->clear();

                m_comboBox->setCurrentIndex(m_comboBox->count() - 1);
            }
        });

    connect(m_manager, &AppStatisticsMonitorManager::appStoped, this, [this](qint64 pid) {
        m_memChart->addNewPoint({m_memChart->lastPointX() + 1, 0});
        m_cpuChart->addNewPoint({m_cpuChart->lastPointX() + 1, 0});
        const int indx = m_comboBox->findData(pid);
        if (indx != -1)
            m_comboBox->removeItem(indx);
    });

    connect(m_manager, &AppStatisticsMonitorManager::newDataAvailable, this, [this] {
        const IDataProvider *currentDataProvider = m_manager->currentDataProvider();
        if (currentDataProvider != nullptr) {
            m_memChart->addNewPoint(
                {(double) currentDataProvider->memoryConsumptionHistory().size(),
                 currentDataProvider->memoryConsumptionLast()});
            m_cpuChart->addNewPoint(
                {(double) currentDataProvider->cpuConsumptionHistory().size(),
                 currentDataProvider->cpuConsumptionLast()});
        }
    });
}

AppStatisticsMonitorView::~AppStatisticsMonitorView() = default;

// AppStatisticsMonitorViewFactory
AppStatisticsMonitorViewFactory::AppStatisticsMonitorViewFactory(
    AppStatisticsMonitorManager *appStatisticManager)
    : m_manager(appStatisticManager)
{
    setDisplayName(("AppStatisticsMonitor"));
    setPriority(300);
    setId("AppStatisticsMonitor");
    setActivationSequence(QKeySequence("Alt+S"));
}

Core::NavigationView AppStatisticsMonitorViewFactory::createWidget()
{
    return {new AppStatisticsMonitorView(m_manager), {}};
}

} // namespace AppStatisticsMonitor::Internal
