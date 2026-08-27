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

#include <projectexplorer/appoutputpane.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/commandline.h>
#include <utils/outputformat.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>
#include <QtTaskTree/QSingleTaskTreeRunner>
#include <QtTaskTree/QTaskTree>

#include <QChar>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

using namespace Utils;
using namespace Core;
using namespace QtTaskTree;
using namespace ProjectExplorer;
using namespace std::chrono_literals;

namespace Android::Internal {

static QString banner(const QString &label, const QString &state)
{
    return QString("**** %1 - %2 ****\n").arg(label, state);
}

struct LogcatEntry
{
    QString line;
    QString tag;
    QString packageName;
    qsizetype headerLength = 0;
    qsizetype colorLength = 0;
    qsizetype timestampLength = 0;
    qint32 pid = -1;
    qint32 tid = -1;
    Utils::OutputFormat format = Utils::StdOutFormat;
    QChar levelLetter;
    bool bypassFilter = false;
    bool parsed = false;

    static LogcatEntry fromLine(const QString &raw);
    QString displayText() const;
};

// Matches adb's '-v threadtime -v year' line layout.
static const QRegularExpression regExpLogcat(
    "\\A(?:\\x1b\\[[0-9;]*m)?" // optional ANSI color
    "(?<timestamp>(?:\\d{4}-)?\\d\\d-\\d\\d \\d\\d:\\d\\d:\\d\\d\\.\\d+) +"
    "(?<pid>\\d+) +"
    "(?<tid>\\d+) +"
    "(?<level>[VDIWEF]) "
    "(?<tag>.*?) *: ");

LogcatEntry LogcatEntry::fromLine(const QString &raw)
{
    LogcatEntry entry{.line = raw};
    const QRegularExpressionMatch match = regExpLogcat.match(raw);
    entry.parsed = match.hasMatch();
    if (entry.parsed) {
        entry.pid = match.capturedView("pid").toInt();
        entry.tid = match.capturedView("tid").toInt();
        entry.levelLetter = match.capturedView("level").at(0);
        entry.tag = match.captured("tag");
        entry.headerLength = match.capturedEnd();
        entry.colorLength = match.capturedStart("timestamp");
        entry.timestampLength = match.capturedLength("timestamp");
    }
    return entry;
}

QString LogcatEntry::displayText() const
{
    if (bypassFilter || !parsed)
        return line;
    const auto &settings = logcatSettings();
    QString result = line.left(colorLength);
    if (settings.compactView()) {
        result += levelLetter;
        result += QLatin1Char('/') + tag.leftJustified(8) + QLatin1Char('(')
                  + QString::number(pid).rightJustified(5) + QLatin1String("): ");
        result += line.mid(headerLength);
        return result;
    }
    if (settings.showTimestamp())
        result += line.mid(colorLength, timestampLength) + QLatin1Char(' ');
    if (settings.showPid()) {
        result += QString::number(pid) + QLatin1Char('-') + QString::number(tid)
                  + QLatin1Char(' ');
    }
    if (settings.showTag())
        result += tag + QLatin1Char(' ');
    if (settings.showPackage() && !packageName.isEmpty())
        result += packageName + QLatin1Char(' ');
    result += levelLetter + QLatin1String("  ");
    result += line.mid(headerLength);
    return result;
}

class LogcatFilter
{
public:
    void setFromText(const QString &text);
    bool accepts(const LogcatEntry &entry) const;

    QString filterText() const { return m_filterText; }

    using FilterPredicate = std::function<bool(const LogcatEntry &)>;

private:
    QList<FilterPredicate> m_predicates;
    QString m_filterText;
};

void LogcatFilter::setFromText(const QString &text)
{
    m_filterText = text;
    m_predicates.clear();
    if (text.isEmpty())
        return;
    m_predicates.append([text](const LogcatEntry &entry) {
        return entry.line.contains(text, Qt::CaseInsensitive);
    });
}

bool LogcatFilter::accepts(const LogcatEntry &entry) const
{
    if (entry.bypassFilter || !entry.parsed)
        return true;
    for (const FilterPredicate &filterPredicate : m_predicates) {
        if (!filterPredicate(entry))
            return false;
    }
    return true;
}

class LogcatStream : public QObject
{
public:
    LogcatStream(AndroidDevice::ConstPtr device);
    ~LogcatStream() override;

    RunControl *tab() const { return m_tabContext.tab; }
    void attachTab(RunControl *tab);

private:
    void start();
    void stop();
    void setStreaming(bool streaming);

    struct TabContext
    {
        QPointer<RunControl> tab;
        bool streaming = false;
        QList<LogcatEntry> buffer;
        qsizetype bufferedChars = 0;
        QHash<qint32, QString> processNames;
        QSet<qint32> askedPids;
        LogcatFilter filter;

        void appendEntry(const LogcatEntry &entry);
        void enforceBudget();
        void backfillPackageNames();
        void renderFromBuffer();
    };

    void onTabDestroyed();

    void postMessage(const QString &msg, Utils::OutputFormat format = Utils::StdOutFormat);

    void onDeviceUpdated(Id id);
    void onDeviceRemoved(Id id);
    void onDisconnected();
    void onConnected();

    void populateProcesses();

    void onOutputFilterTextChanged(const QString &text);

    AndroidDevice::ConstPtr m_device; // may be re-registered under its id
    bool m_disconnected = false;
    QString m_serial;
    std::unique_ptr<QTaskTree> m_task;
    QSingleTaskTreeRunner m_psRunner;
    TabContext m_tabContext;
    QTimer m_filterDebounce;
    bool m_adbFailedBannered = false;
    bool m_pausedWhileHidden = false;

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

LogcatStream::LogcatStream(AndroidDevice::ConstPtr device)
    : m_device(std::move(device))
{
    const auto &settings = logcatSettings();
    const Utils::BaseAspect *displayAspects[] = {&settings.viewMode, &settings.showTimestamp,
                                                 &settings.showPid, &settings.showTag,
                                                 &settings.showPackage};
    for (const Utils::BaseAspect *column : displayAspects) {
        QObject::connect(column, &Utils::BaseAspect::changed,
                         this, [this] { m_filterDebounce.start(); });
    }

    m_filterDebounce.setSingleShot(true);
    m_filterDebounce.setInterval(150ms);
    QObject::connect(&m_filterDebounce, &QTimer::timeout,
                     this, [this] { m_tabContext.renderFromBuffer(); });

    DeviceManager *dm = DeviceManager::instance();
    QObject::connect(dm, &DeviceManager::deviceRemoved, this, &LogcatStream::onDeviceRemoved);
    QObject::connect(dm, &DeviceManager::deviceUpdated, this, &LogcatStream::onDeviceUpdated);
    QObject::connect(dm, &DeviceManager::deviceAdded, this, &LogcatStream::onDeviceUpdated);
}

LogcatStream::~LogcatStream()
{
    auto &reg = streamRegistry();
    if (reg.value(m_device->id()) == this)
        reg.remove(m_device->id());
}

void LogcatStream::attachTab(RunControl *tab)
{
    QTC_ASSERT(tab, return);
    m_tabContext = {};
    m_tabContext.tab = tab;
    tab->setDisplayName(m_device->displayName());
    QObject::connect(tab, &RunControl::outputVisibilityChanged,
                     this, &LogcatStream::setStreaming);
    QObject::connect(tab, &RunControl::outputFilterChanged,
                     this, &LogcatStream::onOutputFilterTextChanged);
    QObject::connect(tab, &RunControl::outputCleared, this, [this] {
        m_tabContext.buffer.clear();
        m_tabContext.bufferedChars = 0;
    });
    QObject::connect(tab, &QObject::destroyed, this, [this] { onTabDestroyed(); });
    setStreaming(tab->isOutputVisible());
}

void LogcatStream::onTabDestroyed()
{
    m_tabContext = {};
    streamRegistry().remove(m_device->id());
    deleteLater();
}

void LogcatStream::setStreaming(bool streaming)
{
    if (!m_tabContext.tab)
        return;
    if (streaming == m_tabContext.streaming)
        return;
    m_tabContext.streaming = streaming;
    if (streaming)
        start();
    else
        stop();
}

void LogcatStream::populateProcesses()
{
    if (!m_tabContext.tab)
        return;
    if (m_psRunner.isRunning() || m_device->deviceState() != IDevice::DeviceReadyToUse)
        return;
    const auto onSetup = [this](Process &process) {
        process.setCommand(adbCommand({"shell", "ps", "-A", "-o", "PID,NAME"}));
    };
    const auto onDone = [this](const Process &process) {
        if (process.result() != ProcessResult::FinishedWithSuccess)
            return;
        m_tabContext.processNames.clear(); // pids get recycled
        const QStringList psLines = process.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
        for (const QString &psLine : psLines) {
            const QStringList fields = psLine.simplified().split(QChar::Space);
            bool ok = false;
            const int pid = fields.size() == 2 ? fields.first().toInt(&ok) : 0;
            if (ok)
                m_tabContext.processNames.insert(pid, fields.last());
        }
        m_tabContext.backfillPackageNames();
    };
    // The timer paces ps to one per 5s; withTimeout cancels a ps hanging past it.
    m_psRunner.start({parallel,
                      finishAllAndSuccess,
                      ProcessTask(onSetup, onDone).withTimeout(5s),
                      timeoutTask(5s, DoneResult::Success)});
}

void LogcatStream::start()
{
    if (m_task)
        return;
    // deviceRemoved clears the state map only after handlers ran; the
    // latch blocks resurrection via the banner's own pane popup.
    if (m_disconnected)
        return;
    if (m_device->deviceState() != IDevice::DeviceReadyToUse)
        return;
    m_serial = m_device->serialNumber();
    if (m_serial.isEmpty())
        return;
    const auto onSetup = [this](Process &process) {
        process.setStdOutLineCallback([this](const QString &line) {
            const LogcatEntry entry = LogcatEntry::fromLine(line);
            if (entry.pid > 0 && !m_tabContext.processNames.contains(entry.pid)
                && !m_tabContext.askedPids.contains(entry.pid)) {
                m_tabContext.askedPids.insert(entry.pid);
                populateProcesses();
            }
            m_tabContext.appendEntry(entry);
        });
        process.setStdErrLineCallback([this](const QString &line) {
            // adb noise while it waits to re-attach the serial; the
            // disconnect banner already tells the story.
            if (line.contains(QLatin1String("- waiting for device -")))
                return;
            postMessage(line, Utils::StdErrFormat);
        });
        process.setCommand(
            adbCommand({"logcat", "-T", "1", "-v", "color", "-v", "threadtime", "-v", "year"}));
    };
    m_adbFailedBannered = false;
    // Pace the respawn so a persistently failing adb cannot busy-restart.
    m_task = std::make_unique<QTaskTree>(Group{Forever{
        (ProcessTask(onSetup, [this](const Process &process) {
             if (process.error() == ProcessError::FailedToStart && !m_adbFailedBannered) {
                 m_adbFailedBannered = true;
                 postMessage(banner(m_device->displayNameWithSerial(),
                                    QLatin1String("adb failed to start")),
                             Utils::NormalMessageFormat);
             }
         }, CallDoneFlag::OnError) || successItem),
        timeoutTask(1s, DoneResult::Success)}});
    m_task->start();
    if (m_pausedWhileHidden) {
        m_pausedWhileHidden = false;
        if (!m_tabContext.buffer.isEmpty()) {
            postMessage(banner(m_device->displayNameWithSerial(),
                               QLatin1String("output skipped while the tab was hidden")),
                        Utils::NormalMessageFormat);
        }
    }
}

void LogcatStream::stop()
{
    // Deleting the tree kills the tail. Defer that past this event loop
    // pass: a synchronous kill can race the tail's in-flight output.
    // The tab's visibility flickers while the pane rearranges: only tear
    // down if streaming stayed off.
    QTimer::singleShot(0, this, [this] {
        if (!m_tabContext.streaming && m_task) {
            m_task.reset();
            m_pausedWhileHidden = true;
        }
    });
}

static AndroidDevice::ConstPtr findDevice(Id id)
{
    return std::dynamic_pointer_cast<const AndroidDevice>(DeviceManager::find(id));
}

void LogcatStream::onDeviceUpdated(Id id)
{
    if (id != m_device->id())
        return;
    if (const auto current = findDevice(id))
        m_device = current;
    if (m_device->deviceState() == IDevice::DeviceReadyToUse)
        onConnected();
    else
        onDisconnected();
}

void LogcatStream::onDeviceRemoved(Id id)
{
    if (id == m_device->id())
        onDisconnected();
}

void LogcatStream::onDisconnected()
{
    if (m_task) {
        // Cancel first: destruction alone would skip the done handlers.
        m_task->cancel();
        m_task.reset();
    }
    if (m_disconnected)
        return;
    m_disconnected = true;
    postMessage(banner(m_device->displayNameWithSerial(), QLatin1String("disconnected")),
                Utils::NormalMessageFormat);
}

void LogcatStream::onConnected()
{
    if (m_disconnected)
        postMessage(banner(m_device->displayNameWithSerial(), QLatin1String("connected")),
                    Utils::NormalMessageFormat);
    m_disconnected = false;
    if (m_tabContext.tab && m_tabContext.streaming)
        start();
}

void LogcatStream::postMessage(const QString &msg, Utils::OutputFormat format)
{
    m_tabContext.appendEntry({.line = msg, .format = format, .bypassFilter = true});
}

void LogcatStream::TabContext::appendEntry(const LogcatEntry &entry)
{
    if (!tab)
        return;
    LogcatEntry stamped = entry;
    stamped.packageName = processNames.value(stamped.pid);
    buffer.append(stamped);
    bufferedChars += stamped.line.size();
    enforceBudget();
    if (filter.accepts(stamped))
        tab->postMessage(stamped.displayText(), stamped.format, false);
}

void LogcatStream::TabContext::enforceBudget()
{
    const qint64 budget = logcatSettings().maxCharCount();
    while (bufferedChars > budget && buffer.size() > 1) {
        bufferedChars -= buffer.first().line.size();
        buffer.removeFirst();
    }
}

void LogcatStream::TabContext::backfillPackageNames()
{
    for (LogcatEntry &entry : buffer) {
        if (!entry.packageName.isEmpty())
            continue;
        entry.packageName = processNames.value(entry.pid);
    }
}

void LogcatStream::TabContext::renderFromBuffer()
{
    if (!tab)
        return;
    tab->clearOutput();
    for (const LogcatEntry &entry : buffer) {
        if (filter.accepts(entry))
            tab->postMessage(entry.displayText(), entry.format, false);
    }
}

void LogcatStream::onOutputFilterTextChanged(const QString &text)
{
    m_tabContext.filter.setFromText(text);
    m_filterDebounce.start();
}

static LogcatStream *ensureStream(const AndroidDevice::ConstPtr &device)
{
    if (!device)
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
    runControl->setOutputPaneActionsEnabled(false);
    runControl->setFiltersOutputAtSource(true);
    logcatStream->attachTab(runControl);

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
    if (!tab || tab->isOutputVisible())
        return;
    if (!OutputPanePlaceHolder::getCurrent())
        ModeManager::activateMode(Core::Constants::MODE_EDIT);
    tab->showOutputPane();
}

} // namespace Android::Internal
