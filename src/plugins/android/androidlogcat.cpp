// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "androidlogcat.h"

#include "androidconfigurations.h"
#include "androiddevice.h"
#include "androidtr.h"
#include "androidutils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/outputpane.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/commandline.h>
#include <utils/outputformat.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QTaskTree>

#include <QHash>
#include <QObject>
#include <QPointer>

using namespace Utils;
using namespace Core;
using namespace QtTaskTree;
using namespace ProjectExplorer;
using namespace std::chrono_literals;

namespace Android::Internal {

class LogcatStream : public QObject
{
public:
    LogcatStream(AndroidDevice::ConstPtr device)
        : m_device(std::move(device))
    {}
    ~LogcatStream() override;

    void start();
    void stop();

    RunControl *tab() const { return m_tabContext.tab; }
    void attachTab(RunControl *tab);

private:
    struct TabContext
    {
        QPointer<RunControl> tab;
    };

    void onTabDestroyed();

    const AndroidDevice::ConstPtr m_device;
    QString m_serial;
    std::unique_ptr<QTaskTree> m_task;
    TabContext m_tabContext;

    CommandLine adbCommand(const QStringList &args) const
    {
        return {AndroidConfig::adbToolPath(), adbSelector(m_serial) + args};
    }
};

static QHash<Id, LogcatStream *> &streamRegistry()
{
    static QHash<Id, LogcatStream *> map;
    return map;
}

LogcatStream::~LogcatStream()
{
    stop();
    auto &reg = streamRegistry();
    if (reg.value(m_device->id()) == this)
        reg.remove(m_device->id());
}

void LogcatStream::attachTab(RunControl *tab)
{
    QTC_ASSERT(tab, return);
    m_tabContext = {};
    m_tabContext.tab = tab;
    tab->setDisplayName(m_device->displayNameWithSerial());
    QObject::connect(tab, &QObject::destroyed, this, [this] { onTabDestroyed(); });
}

void LogcatStream::onTabDestroyed()
{
    m_tabContext = {};
    stop();
    streamRegistry().remove(m_device->id());
    deleteLater();
}

void LogcatStream::start()
{
    m_serial = m_device->serialNumber();
    if (m_serial.isEmpty())
        return;
    const auto onSetup = [this](Process &process) {
        const auto post = [this](const QString &line, Utils::OutputFormat fmt) {
            if (m_tabContext.tab)
                m_tabContext.tab->postMessage(line, fmt, false);
        };
        process.setStdOutLineCallback(
            [post](const QString &line) { post(line, Utils::StdOutFormat); });
        process.setStdErrLineCallback(
            [post](const QString &line) { post(line, Utils::StdErrFormat); });
        // -T 1 starts the tail at the current head, skipping the device's existing ring buffer (live tail only).
        process.setCommand(adbCommand({"logcat", "-T", "1", "-v", "color", "-v", "brief"}));
    };
    // Pace the respawn so a persistently failing adb cannot busy-restart.
    m_task = std::make_unique<QTaskTree>(Group{Forever{
        (ProcessTask(onSetup) || successItem),
        timeoutTask(1s, DoneResult::Success)}});
    m_task->start();
}

void LogcatStream::stop()
{
    m_task.reset();
}

static LogcatStream *ensureStream(const AndroidDevice::ConstPtr &device)
{
    if (!device || device->serialNumber().isEmpty())
        return nullptr;
    const auto id = device->id();
    auto &reg = streamRegistry();
    if (auto *stream = reg.value(id))
        return stream;
    auto *stream = new LogcatStream(device);
    reg.insert(id, stream);
    return stream;
}

static RunControl *openLogcatTabForStream(LogcatStream *logcatStream)
{
    if (!logcatStream)
        return nullptr;
    if (RunControl *existing = logcatStream->tab())
        return existing;
    auto *runControl = new RunControl(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    runControl->setPromptToStop([](bool *) { return true; });
    logcatStream->attachTab(runControl);
    logcatStream->start();

    runControl->setRunRecipe(QBarrierTask([](QBarrier &) {}).withCancel([runControl] {
        return makeObjectSignal(runControl, &RunControl::canceled);
    }));
    runControl->start();
    return runControl;
}

void showLogcatTab(const AndroidDevice::ConstPtr &device)
{
    // The menu snapshot can go stale between aboutToShow and the click.
    const AndroidDevice::ConstPtr ready
        = device ? AndroidDevice::asReady(DeviceManager::find(device->id())) : nullptr;
    if (!ready) {
        Core::MessageManager::writeFlashing(
            Tr::tr("Logcat: device \"%1\" is no longer available.")
                .arg(device ? device->displayName() : QString()));
        return;
    }
    auto *stream = ensureStream(ready);
    if (!stream)
        return;
    RunControl *tab = openLogcatTabForStream(stream);
    if (tab) {
        if (!OutputPanePlaceHolder::getCurrent())
            ModeManager::activateMode(Core::Constants::MODE_EDIT);
        tab->showOutputPane();
    }
}

} // namespace Android::Internal
