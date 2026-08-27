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
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>

#include <utils/algorithm.h>
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

#include <utility>

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

enum class LogcatLevel { Unknown, Verbose, Debug, Info, Warning, Error, Fatal };

static LogcatLevel logcatLevel(QStringView text)
{
    static const std::pair<QLatin1String, LogcatLevel> levels[] = {
        {QLatin1String("verbose"), LogcatLevel::Verbose},
        {QLatin1String("debug"), LogcatLevel::Debug},
        {QLatin1String("info"), LogcatLevel::Info},
        {QLatin1String("warning"), LogcatLevel::Warning},
        {QLatin1String("error"), LogcatLevel::Error},
        {QLatin1String("fatal"), LogcatLevel::Fatal},
        {QLatin1String("assert"), LogcatLevel::Fatal},
    };
    for (const auto &[name, level] : levels) {
        if (name.startsWith(text, Qt::CaseInsensitive))
            return level;
    }
    return LogcatLevel::Unknown;
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
    LogcatLevel level = LogcatLevel::Unknown;
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
        const QStringView levelText = match.capturedView("level");
        entry.pid = match.capturedView("pid").toInt();
        entry.tid = match.capturedView("tid").toInt();
        entry.level = logcatLevel(levelText);
        entry.levelLetter = levelText.at(0);
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

static bool matchesFreeText(const LogcatEntry &entry, const QString &term)
{
    return entry.line.contains(term, Qt::CaseInsensitive)
           || entry.packageName.contains(term, Qt::CaseInsensitive);
}

static QString activeProjectPackage()
{
    const BuildConfiguration *bc = activeBuildConfigForActiveProject();
    return bc ? packageName(bc) : QString();
}

static constexpr QLatin1StringView packageKey("package");
static constexpr QLatin1StringView levelKey("level");
static constexpr QLatin1StringView tagKey("tag");
static constexpr QLatin1StringView mineValue("mine");

class LogcatFilter
{
public:
    void setFromText(const QString &text);
    void bindToPackage(const QString &packageName);
    bool accepts(const LogcatEntry &entry) const;

    QString filterText() const { return m_filterText; }

    using FilterPredicate = std::function<bool(const LogcatEntry &)>;

private:
    QList<FilterPredicate> m_predicates;
    QString m_boundPackage;
    QString m_filterText;
};

static LogcatFilter::FilterPredicate minePredicate(const QString &packageName)
{
    const QString processPrefix = packageName + u':';
    return [packageName, processPrefix](const LogcatEntry &e) {
        return e.packageName.compare(packageName, Qt::CaseInsensitive) == 0
               || e.packageName.startsWith(processPrefix, Qt::CaseInsensitive);
    };
}

static LogcatFilter::FilterPredicate levelPredicate(LogcatLevel min)
{
    return [min](const LogcatEntry &e) { return e.level >= min; };
}

static LogcatFilter::FilterPredicate tagPredicate(const QString &tag)
{
    return [tag](const LogcatEntry &e) { return e.tag.contains(tag, Qt::CaseInsensitive); };
}

void LogcatFilter::setFromText(const QString &text)
{
    m_filterText = text;
    m_predicates.clear();
    const QStringList tokens = text.simplified().split(QChar::Space, Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        const int colon = token.indexOf(u':');
        const QString key = colon > 0 ? token.left(colon).toLower() : QString();
        const QString value = colon > 0 ? token.mid(colon + 1) : QString();
        const bool queryKey = key == packageKey || key == levelKey || key == tagKey;
        if (queryKey && value.isEmpty())
            continue;
        if (key == packageKey) {
            if (value.compare(mineValue, Qt::CaseInsensitive) == 0) {
                const QString package = m_boundPackage.isEmpty() ? activeProjectPackage()
                                                                 : m_boundPackage;
                if (!package.isEmpty())
                    m_predicates.append(minePredicate(package));
                else
                    m_predicates.append([](const LogcatEntry &) { return false; });
            } else {
                m_predicates.append([value](const LogcatEntry &e) {
                    return e.packageName.contains(value, Qt::CaseInsensitive);
                });
            }
        } else if (key == levelKey) {
            const LogcatLevel level = logcatLevel(value);
            if (level != LogcatLevel::Unknown)
                m_predicates.append(levelPredicate(level));
            else
                m_predicates.append([](const LogcatEntry &) { return false; });
        } else if (key == tagKey) {
            m_predicates.append(tagPredicate(value));
        } else {
            m_predicates.append([token](const LogcatEntry &e) {
                return matchesFreeText(e, token);
            });
        }
    }
}

void LogcatFilter::bindToPackage(const QString &packageName)
{
    m_boundPackage = packageName;
    if (m_filterText.isEmpty())
        m_filterText = QStringLiteral("package:mine");
    setFromText(m_filterText);
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

    Id deviceId() const { return m_device->id(); }
    RunControl *tab() const { return m_tabContext.tab; }
    void attachTab(RunControl *tab);
    void adoptAppRunControl(RunControl *appRunControl);

    void bindToApp(qint64 pid, const QString &packageName);

    void addLineReader(RunControl *owner, LogcatLineHandler callback);

private:
    void setAppControllable(bool controllable);
    void onAppCanceled();
    void onAppStopped();

    void removeLineReader(RunControl *owner);

    void start();
    void stop();
    void setStreaming(bool streaming);

    bool shouldKeepRunning() const;

    struct LineReader
    {
        LogcatLineHandler callback;
        QPointer<RunControl> owner;
    };

    bool hasLineReaders() const;

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
    void disposeIfIdle();

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
    QString m_resumeTimestamp;
    QString m_projectPackage;
    QList<LineReader> m_lineReaders;
    QPointer<RunControl> m_boundRunner;
    bool m_appStopRequested = false;
    bool m_appControllable = false;

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

    const auto resolveProjectPackage = [this] {
        const QString package = activeProjectPackage();
        if (package == m_projectPackage)
            return;
        m_projectPackage = package;
        m_tabContext.filter.setFromText(m_tabContext.filter.filterText());
        m_filterDebounce.start();
    };
    QObject::connect(ProjectManager::instance(), &ProjectManager::activeBuildConfigurationChanged,
                     this, resolveProjectPackage);
    QObject::connect(ProjectManager::instance(), &ProjectManager::parsingFinishedActive,
                     this, resolveProjectPackage);

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

static void stopAndDelete(RunControl *runner)
{
    if (runner->isStopped()) {
        runner->deleteLater();
        return;
    }
    QObject::disconnect(runner, &RunControl::stopped, runner, &RunControl::initiateStart);
    QObject::connect(runner, &RunControl::stopped, runner, &QObject::deleteLater);
    runner->initiateStop();
}

void LogcatStream::adoptAppRunControl(RunControl *appRunControl)
{
    RunControl *tab = m_tabContext.tab;
    QTC_ASSERT(tab && appRunControl, return);
    if (m_boundRunner == appRunControl)
        return;
    m_appStopRequested = false;
    RunControl *previous = m_boundRunner.get();
    m_boundRunner = appRunControl;
    if (previous) {
        QObject::disconnect(tab, nullptr, previous, nullptr);
        QObject::disconnect(previous, nullptr, this, nullptr);
        if (!previous->isStopped()) {
            postMessage(banner(previous->displayName(),
                               QLatin1String("stopped for a new run")),
                        Utils::NormalMessageFormat);
        }
        stopAndDelete(previous);
    }
    if (tab->isStopped())
        tab->initiateStart();
    QObject::connect(appRunControl, &RunControl::appendMessage, this,
                     [this](const QString &msg, Utils::OutputFormat format) {
                         postMessage(msg, format);
                     });
    QObject::connect(tab, &RunControl::canceled, this,
                     &LogcatStream::onAppCanceled, Qt::UniqueConnection);
    QObject::connect(tab, &RunControl::canceled, appRunControl, &RunControl::initiateStop);
    QObject::connect(tab, &RunControl::aboutToStart, appRunControl, [appRunControl] {
        if (appRunControl->isStopped()) {
            appRunControl->initiateStart();
            return;
        }
        QObject::connect(appRunControl, &RunControl::stopped, appRunControl,
                         &RunControl::initiateStart,
                         static_cast<Qt::ConnectionType>(Qt::SingleShotConnection
                                                         | Qt::UniqueConnection));
    });
    QObject::connect(appRunControl, &RunControl::stopped, this, &LogcatStream::onAppStopped);
    setAppControllable(true);
}

void LogcatStream::onTabDestroyed()
{
    if (RunControl *runner = m_boundRunner.get()) {
        stopAndDelete(runner);
        m_boundRunner = nullptr;
    }
    m_tabContext = {};
    disposeIfIdle();
}

void LogcatStream::disposeIfIdle()
{
    if (m_tabContext.tab || hasLineReaders())
        return;
    auto &registry = streamRegistry();
    if (registry.value(m_device->id()) == this)
        registry.remove(m_device->id());
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

void LogcatStream::bindToApp(qint64 pid, const QString &packageName)
{
    if (pid <= 0 || !m_tabContext.tab)
        return;
    start();
    m_tabContext.processNames.insert(pid, packageName);
    m_tabContext.backfillPackageNames();
    m_tabContext.filter.bindToPackage(packageName);
    m_tabContext.tab->setOutputFilterText(m_tabContext.filter.filterText());
    m_filterDebounce.stop();
    m_tabContext.renderFromBuffer();
}

bool LogcatStream::hasLineReaders() const
{
    return Utils::anyOf(m_lineReaders, [](const LineReader &reader) {
        return !reader.owner.isNull();
    });
}

void LogcatStream::addLineReader(RunControl *owner, LogcatLineHandler callback)
{
    Utils::erase(m_lineReaders, [owner](const LineReader &reader) {
        return reader.owner == owner || reader.owner.isNull();
    });
    m_lineReaders.append({std::move(callback), owner});
    QObject::connect(owner, &RunControl::stopped, this,
                     [this, owner] { removeLineReader(owner); },
                     Qt::SingleShotConnection);
    start();
}

void LogcatStream::removeLineReader(RunControl *owner)
{
    Utils::erase(m_lineReaders, [owner](const LineReader &reader) {
        return reader.owner == owner || reader.owner.isNull();
    });
    if (!shouldKeepRunning())
        stop();
    disposeIfIdle();
}

bool LogcatStream::shouldKeepRunning() const
{
    return m_tabContext.streaming || hasLineReaders()
           || (m_boundRunner && !m_boundRunner->isStopped());
}

void LogcatStream::onAppCanceled()
{
    m_appStopRequested = true;
}

void LogcatStream::onAppStopped()
{
    if (!m_appStopRequested)
        setAppControllable(false);
    m_appStopRequested = false;
}

void LogcatStream::setAppControllable(bool controllable)
{
    m_appControllable = controllable;
    if (m_tabContext.tab)
        m_tabContext.tab->setOutputPaneActionsEnabled(controllable);
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
            if (entry.parsed)
                m_resumeTimestamp = line.mid(entry.colorLength, entry.timestampLength);
            if (entry.pid > 0 && !m_tabContext.processNames.contains(entry.pid)
                && !m_tabContext.askedPids.contains(entry.pid)) {
                m_tabContext.askedPids.insert(entry.pid);
                populateProcesses();
            }
            m_tabContext.appendEntry(entry);
            if (entry.parsed) {
                for (const LineReader &reader : std::as_const(m_lineReaders)) {
                    if (reader.owner)
                        reader.callback(entry.pid, line);
                }
            }
        });
        process.setStdErrLineCallback([this](const QString &line) {
            // adb noise while it waits to re-attach the serial; the
            // disconnect banner already tells the story.
            if (line.contains(QLatin1String("- waiting for device -")))
                return;
            postMessage(line, Utils::StdErrFormat);
        });
        const QString since = m_resumeTimestamp.isEmpty() ? QString("1") : m_resumeTimestamp;
        process.setCommand(
            adbCommand({"logcat", "-T", since, "-v", "color", "-v", "threadtime", "-v", "year"}));
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
        if (!shouldKeepRunning() && m_task) {
            m_task.reset();
            m_pausedWhileHidden = true;
            m_resumeTimestamp.clear();
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
    if (m_tabContext.tab)
        m_tabContext.tab->setOutputPaneActionsEnabled(false);
}

void LogcatStream::onConnected()
{
    if (m_disconnected)
        postMessage(banner(m_device->displayNameWithSerial(), QLatin1String("connected")),
                    Utils::NormalMessageFormat);
    m_disconnected = false;
    if (m_tabContext.tab)
        m_tabContext.tab->setOutputPaneActionsEnabled(m_appControllable);
    if (!shouldKeepRunning())
        return;
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

static AndroidDevice::ConstPtr deviceForRun(const RunControl *runControl)
{
    const IDeviceConstPtr snapshot = runControl->device();
    return snapshot ? std::dynamic_pointer_cast<const AndroidDevice>(
                          DeviceManager::find(snapshot->id())) : nullptr;
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
    // Keeps the pane from reusing this tab for another run.
    runControl->setCommandLine(
        {FilePath::fromString("android-logcat"), {logcatStream->deviceId().toString()}});
    runControl->setPromptToStop([](bool *) { return true; });
    runControl->setOutputPaneActionsEnabled(false);
    runControl->setFiltersOutputAtSource(true);
    logcatStream->attachTab(runControl);

    const auto reportStarted = QSyncTask([runControl] { runControl->reportStarted(); });
    const auto waitForStop = QBarrierTask([](QBarrier &) {}).withCancel([runControl] {
        return makeObjectSignal(runControl, &RunControl::canceled);
    });
    runControl->setRunRecipe(Group{reportStarted, waitForStop});
    runControl->start();
    return runControl;
}

static LogcatStream *adoptRunControlAsTab(RunControl *runControl)
{
    if (!runControl)
        return nullptr;
    LogcatStream *stream = ensureStream(deviceForRun(runControl));
    if (!stream)
        return nullptr;
    // A closing tab is unlisted before it dies; adopting onto it loses the run.
    if (stream->tab() && !stream->tab()->hasOutputPaneTab())
        return nullptr;
    if (!stream->tab())
        openLogcatTabForStream(stream);
    RunControl *tab = stream->tab();
    if (!tab || tab == runControl)
        return stream;
    runControl->detachOutputPaneTab();
    stream->adoptAppRunControl(runControl);
    return stream;
}

void adoptRunControlForLogcat(RunControl *runControl)
{
    if (!runControl || runControl->suppressApplicationOutput())
        return;
    adoptRunControlAsTab(runControl);
}

void bindRunningAppToLogcat(RunControl *runControl, qint64 pid, const QString &packageName)
{
    if (!runControl || pid <= 0 || runControl->suppressApplicationOutput())
        return;
    LogcatStream *stream = adoptRunControlAsTab(runControl);
    if (!stream)
        return;
    stream->bindToApp(pid, packageName);
}

void monitorLogcat(RunControl *runControl, const LogcatLineHandler &onLine)
{
    QTC_ASSERT(runControl, return);
    if (LogcatStream *stream = ensureStream(deviceForRun(runControl)))
        stream->addLineReader(runControl, onLine);
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
