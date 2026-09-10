// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "buildsystem.h"
#include "buildtargetinfo.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "devicesupport/idevicefactory.h"
#include "devicesupport/sshparameters.h"
#include "editorconfiguration.h"
#include "issuesmanager.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorerconstants.h"
#include "projectmanager.h"
#include "projectnodes.h"
#include "runconfiguration.h"
#include "runconfigurationaspects.h"
#include "runcontrol.h"
#include "target.h"
#include "task.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/vcsmanager.h>

#include <mcp/server/mcpserver.h>
#include <mcp/server/toolregistry.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/filepath.h>
#include <utils/filesearch.h>
#include <utils/globaltasktree.h>
#include <utils/id.h>
#include <utils/mimeconstants.h>
#include <utils/processinterface.h>
#include <utils/result.h>
#include <utils/shutdownguard.h>
#include <utils/stringutils.h>

#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMap>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>

using namespace Utils;

static Q_LOGGING_CATEGORY(mcpDevices, "qtc.projectexplorer.mcp", QtWarningMsg)

namespace ProjectExplorer::Internal {

static QList<Project *> projectsForName(const QString &name)
{
    return Utils::filtered(ProjectManager::projects(), Utils::equal(&Project::displayName, name));
}

// Returns the version control branch/topic for the given project directory, or an empty
// string when the project is not under a recognised version control system.
static QString currentBranchForProject(const Project *p)
{
    if (!p)
        return {};
    const FilePath dir = p->projectDirectory();
    if (dir.isEmpty())
        return {};
    if (Core::IVersionControl *vcs = Core::VcsManager::findVersionControlForDirectory(dir, nullptr))
        return vcs->vcsTopic(dir);
    return {};
}

static QString projectTypeFromMimeType(const QString &mimeType)
{
    if (mimeType == Utils::Constants::CMAKE_PROJECT_MIMETYPE)
        return "cmake";
    if (mimeType == Utils::Constants::PROFILE_MIMETYPE)
        return "qmake";
    if (mimeType == Utils::Constants::QBS_MIMETYPE)
        return "qbs";
    if (mimeType == Utils::Constants::QMLPROJECT_MIMETYPE)
        return "qmlproject";
    return mimeType; // fall back to raw mime type for unknown project kinds
}

// Returns a {name, path, branch, type} JSON object for a single project.  Used by
// project_list (which also injects is_active) and by resolveProjects (which
// populates the candidates array for ambiguous-name errors).
static QJsonObject projectInfoObject(const Project *p)
{
    if (!p)
        return {};
    return {
        {"name", p->displayName()},
        {"path", p->projectFilePath().toUserOutput()},
        {"branch", currentBranchForProject(p)},
        {"type", projectTypeFromMimeType(p->mimeType())},
    };
}

struct ResolvedProjects
{
    QList<Project *> projects;
    QJsonArray candidates; // [{name, path, branch}] for every project that name-matched
};

// Dual-key (projectName, projectPath) lookup.  When projectPath is supplied it
// is authoritative — paths are always unique so projects.size() ≤ 1.  When
// only projectName is supplied, multiple loaded projects may share it (e.g.
// the same project open in two Git worktrees); callers detect this via
// projects.size() > 1 and should return an "ambiguous_name" error with the
// candidates list so the AI can pass projectPath to disambiguate.
static ResolvedProjects resolveProjects(const QString &projectName,
                                        const QString &projectPath)
{
    ResolvedProjects result;
    for (Project *p : ProjectManager::projects()) {
        if (!p)
            continue;
        const bool nameMatches = !projectName.isEmpty() && p->displayName() == projectName;
        const bool pathMatches = !projectPath.isEmpty()
                                 && p->projectFilePath().toUserOutput() == projectPath;
        if (!projectPath.isEmpty()) {
            // Path is authoritative when supplied — never name-match alongside it.
            if (pathMatches)
                result.projects.append(p);
        } else if (nameMatches) {
            result.projects.append(p);
        }
        if (nameMatches)
            result.candidates.append(projectInfoObject(p));
    }
    return result;
}

struct ProjectResolution
{
    Project *project = nullptr; // non-null on success
    QJsonObject error;          // ready-to-return failure object when project is null
};

// Resolves the single project a tool should act on from a (projectName,
// projectPath) pair, applying the same dual-key rules and error conventions as
// project_set_active. When both are empty: defaults to the startup project if
// defaultToStartup is true (error reason "no_startup_project" when there is
// none), otherwise fails with reason "no_target". A name matching multiple
// loaded projects fails with reason "ambiguous_name" plus a candidates array.
static ProjectResolution resolveTargetProject(
    const QString &projectName, const QString &projectPath, bool defaultToStartup)
{
    ProjectResolution r;

    if (projectName.isEmpty() && projectPath.isEmpty()) {
        if (defaultToStartup) {
            r.project = ProjectManager::startupProject();
            if (!r.project)
                r.error = {{"success", false}, {"reason", "no_startup_project"},
                           {"message", "No startup project. Pass project_name or project_path."}};
        } else {
            r.error = {{"success", false}, {"reason", "no_target"},
                       {"message", "Pass project_name or project_path to identify the target "
                                   "project."}};
        }
        return r;
    }

    const ResolvedProjects resolved = resolveProjects(projectName, projectPath);
    if (resolved.projects.isEmpty()) {
        r.error = {
            {"success", false},
            {"reason", projectPath.isEmpty() ? "project_not_loaded" : "path_not_loaded"},
            {"message",
             projectPath.isEmpty()
                 ? QString("No project named '%1' is currently loaded.").arg(projectName)
                 : QString("No project at path '%1' is currently loaded.").arg(projectPath)},
            {"candidates", resolved.candidates}};
        return r;
    }
    if (resolved.projects.size() > 1) {
        r.error = {
            {"success", false},
            {"reason", "ambiguous_name"},
            {"message",
             QString("Multiple projects named '%1' are loaded. Pass project_path "
                     "(one of the listed candidates) to disambiguate.")
                 .arg(projectName)},
            {"candidates", resolved.candidates}};
        return r;
    }

    r.project = resolved.projects.first();
    return r;
}

// Describes a project as a "module": its runnable application targets (from the
// active build system, so empty until the project is configured and parsed - and
// executables only, as that is all applicationTargets() carries) and the other
// loaded projects it depends on (the session Dependencies settings).
static QJsonObject projectModulesObject(Project *project)
{
    QJsonObject obj = projectInfoObject(project);

    QJsonArray targets;
    if (BuildSystem *bs = project->activeBuildSystem()) {
        for (const BuildTargetInfo &t : bs->applicationTargets()) {
            targets.append(QJsonObject{
                {"name", t.displayName},
                {"build_key", t.buildKey},
                {"executable", t.targetFilePath.toUserOutput()},
                {"project_file", t.projectFilePath.toUserOutput()}});
        }
    }
    obj["targets"] = targets;

    QJsonArray dependsOn;
    for (const Project *dep : ProjectManager::dependencies(project))
        dependsOn.append(projectInfoObject(dep));
    obj["depends_on"] = dependsOn;

    return obj;
}

// Counts what it drops, so a reply can say how much is missing.
class BoundedOutput
{
public:
    static constexpr int maxCaptureSize = 1024 * 200;

    void append(const QString &text)
    {
        m_text += text;
        if (m_text.size() > maxCaptureSize) {
            m_dropped += m_text.size() - maxCaptureSize;
            m_text = m_text.right(maxCaptureSize);
        }
    }

    void clear() { *this = {}; }

    bool isEmpty() const { return m_text.isEmpty(); }
    qint64 total() const { return m_dropped + m_text.size(); }
    bool truncatedAt(int maxChars) const { return keptFor(maxChars) < m_text.size(); }

    // For output scoped to one build: the first error is the cause and the
    // ones after it cascade, so the head carries more than the tail. The end
    // is kept too, because a link or deploy failure has nothing before it.
    QString digest(int maxChars) const
    {
        const int keep = keptFor(maxChars);
        if (keep == m_text.size() && m_dropped == 0)
            return m_text;
        if (m_dropped > 0) // the head is gone already; do not imply otherwise
            return tail(maxChars);
        const int split = keep - keep / 4;
        const auto joined = [this](int head, int tailFrom) {
            return m_text.left(head) + marker(tailFrom - head) + m_text.mid(tailFrom);
        };
        // Cut on line boundaries: splicing mid-token reads as corruption
        // rather than as an omission. Aligning shortens both halves but can
        // lengthen the marker by a digit, so it may not breach the cap.
        const int head = m_text.lastIndexOf('\n', split - 1) + 1;
        const int tailFrom = m_text.indexOf('\n', m_text.size() - (keep - head)) + 1;
        if (head > 0 && tailFrom > head) {
            const QString aligned = joined(head, tailFrom);
            if (aligned.size() <= maxChars)
                return aligned;
        }
        return joined(split, m_text.size() - (keep - split));
    }

    QString tail(int maxChars) const
    {
        const int keep = keptFor(maxChars);
        if (keep == m_text.size() && m_dropped == 0)
            return m_text;
        return marker(total() - keep) + m_text.right(keep);
    }

private:
    static QString marker(qint64 omitted)
    {
        return QString("[%1 earlier character(s) omitted]\n").arg(omitted);
    }

    // The marker counts as part of what the caller asked for, so its own
    // length has to come out of the budget before the text does.
    int keptFor(int maxChars) const
    {
        if (total() <= maxChars)
            return m_text.size();
        int keep = maxChars;
        for (int i = 0; i < 8; ++i) {
            const int next = qMax(0, maxChars - int(marker(total() - keep).size()));
            if (next == keep)
                break;
            keep = next;
        }
        return qMin(keep, int(m_text.size()));
    }

    QString m_text;
    qint64 m_dropped = 0;
};

// Above this, typical clients spill the reply to a file.
static constexpr int maxReplyOutputSize = 1024 * 16;
static constexpr int minReplyOutputSize = 1000;
// Past the capture size, so asking for the maximum returns everything still
// held instead of reporting truncated for good: the marker is charged
// against the same budget.
static constexpr int maxRequestableOutputSize = BoundedOutput::maxCaptureSize + 128;
static constexpr int defaultSearchResults = 200;
static constexpr int maxSearchResults = 5000;

// A wrongly-typed count would fall through to a default and quietly do
// something else than asked.
static Utils::Result<int> readCount(const QJsonObject &args, const QString &key, int fallback)
{
    const QJsonValue value = args.value(key);
    if (value.isUndefined() || value.isNull())
        return fallback;
    if (!value.isDouble())
        return ResultError(QString("%1 must be a number").arg(key));
    return int(qBound<double>(INT_MIN, value.toDouble(), INT_MAX));
}

static QString withoutAnsiCodes(const QString &text)
{
    static const QRegularExpression csi("\x1B\\[[0-9;?]*[ -/]*[@-~]");
    return QString(text).remove(csi);
}

// Accumulates the Compile Output pane text (build AND deploy step output) so it
// can be inspected via build_get_compile_output - deploy errors in particular are not
// otherwise surfaced through MCP.
static BoundedOutput &compileOutput()
{
    static BoundedOutput out;
    static const QMetaObject::Connection conn = QObject::connect(
        BuildManager::instance(),
        &BuildManager::outputText,
        BuildManager::instance(),
        [](const QString &text) { out.append(withoutAnsiCodes(text)); });
    Q_UNUSED(conn)
    return out;
}

static QString &generalMessagesBuffer()
{
    static QString buffer;
    static const bool started = [] {
        Core::MessageManager::addObserver(Utils::shutdownGuard(), [](const QString &message) {
            buffer += message;
            buffer += '\n';
            static constexpr int maxOutputSize = 1024 * 200; // cap to bound context size
            if (buffer.size() > maxOutputSize)
                buffer = buffer.right(maxOutputSize);
        });
        return true;
    }();
    Q_UNUSED(started)
    return buffer;
}

// Persistent issues manager for all PE mcp tools. Guarded rather than a plain
// static: at exit() the TaskHub it is connected to is gone already.
static ProjectExplorer::IssuesManager &issuesManager()
{
    static Utils::GuardedObject<ProjectExplorer::IssuesManager> manager;
    return manager;
}

struct BuildRecord
{
    enum class State { Running, Succeeded, Failed, Canceled };

    quint64 id = 0;
    QString projectName;
    FilePath projectDir;
    State state = State::Running;
    // queue() runs a modal loop before the build starts, and isBuilding() is
    // false throughout it, so a record that has not seen it running yet must
    // not be taken for stale.
    bool everRunning = false;
    QElapsedTimer elapsed;
    qint64 durationMs = 0;
    int baselineErrorCount = 0;
    int baselineWarningCount = 0;
    int errorCount = 0;
    int warningCount = 0;
    QJsonArray issues;
    BoundedOutput output;

    // Task counts are cumulative unless "Clear issues list on new build" is
    // set. Fewer than at build start means the list was cleared while the
    // build ran, which leaves the baseline meaningless.
    static int addedSinceStart(int current, int baseline)
    {
        return current >= baseline ? current - baseline : current;
    }
};

static constexpr int keptBuilds = 8;
// Capped under the ~60 s clients typically use: a longer wait means the
// transport drops the session, and the build dies with it.
static constexpr qint64 defaultBuildWaitMs = 45000;
static constexpr qint64 maxBuildWaitMs = 55000;
static constexpr int defaultIssueCount = 50;
static constexpr int maxIssueCount = 1000;

// Seeded from the process start: a counter would hand out 1 again after every
// restart, so an id held across one could name a different build.
static quint64 lastBuildId = quint64(QDateTime::currentMSecsSinceEpoch());
static quint64 runningBuildId = 0;
static bool buildWasCanceled = false;

static QList<BuildRecord> &buildRecords()
{
    static QList<BuildRecord> records;
    return records;
}

static BuildRecord *buildRecord(quint64 id)
{
    for (BuildRecord &record : buildRecords()) {
        if (record.id == id)
            return &record;
    }
    return nullptr;
}

static BuildRecord *runningBuild()
{
    return runningBuildId == 0 ? nullptr : buildRecord(runningBuildId);
}

static BuildRecord &beginBuildRecord(Project *project)
{
    QList<BuildRecord> &records = buildRecords();
    while (records.size() >= keptBuilds)
        records.removeFirst();
    BuildRecord record;
    record.id = ++lastBuildId;
    if (project) {
        record.projectName = project->displayName();
        record.projectDir = project->projectDirectory();
    }
    record.baselineErrorCount = BuildManager::getErrorTaskCount();
    record.baselineWarningCount = BuildManager::getWarningTaskCount();
    record.elapsed.start();
    records.append(record);
    runningBuildId = record.id;
    return records.last();
}

static void finishBuildRecord(BuildRecord &record, BuildRecord::State state)
{
    record.state = state;
    record.durationMs = record.elapsed.elapsed();
    const QJsonObject data = issuesManager().getBuildIssues();
    const QJsonObject summary = data.value("summary").toObject();
    record.errorCount = summary.value("errorCount").toInt();
    record.warningCount = summary.value("warningCount").toInt();
    record.issues = data.value("issues").toArray();
    if (runningBuildId == record.id)
        runningBuildId = 0;
}

static void trackBuilds()
{
    BuildManager *manager = BuildManager::instance();
    static const bool connected = [manager] {
        // BuildManager has no buildStarted signal.
        QObject::connect(manager, &BuildManager::buildStateChanged, manager, [] {
            if (!BuildManager::isBuilding())
                return;
            BuildRecord *record = runningBuild();
            if (!record)
                record = &beginBuildRecord(nullptr);
            record->everRunning = true;
        });
        QObject::connect(manager, &BuildManager::outputText, manager, [](const QString &text) {
            if (BuildRecord *record = runningBuild())
                record->output.append(withoutAnsiCodes(text));
        });
        // Arrives before buildQueueFinished(false), so the verdict below can
        // tell a cancel from a build error.
        QObject::connect(manager, &BuildManager::buildQueueCanceled, manager, [] {
            buildWasCanceled = true;
        });
        QObject::connect(manager, &BuildManager::buildQueueFinished, manager, [](bool success) {
            // Consumed before the guard below, so an unpaired cancel cannot
            // reach the next build.
            const bool canceled = buildWasCanceled;
            buildWasCanceled = false;
            BuildRecord *record = runningBuild();
            if (!record)
                return; // an empty queue finishes without anything having been built
            finishBuildRecord(
                *record,
                canceled  ? BuildRecord::State::Canceled
                : success ? BuildRecord::State::Succeeded
                          : BuildRecord::State::Failed);
            // A queued build starts right here, with no buildStateChanged of
            // its own to record it.
            if (BuildManager::isBuilding())
                beginBuildRecord(nullptr).everRunning = true;
        });
        return true;
    }();
    Q_UNUSED(connected)
}

static void reclaimStaleBuild()
{
    BuildRecord *record = runningBuild();
    if (record && record->everRunning && !BuildManager::isBuilding())
        finishBuildRecord(*record, BuildRecord::State::Canceled);
}

static QString buildStateName(BuildRecord::State state)
{
    switch (state) {
    case BuildRecord::State::Running:   return "running";
    case BuildRecord::State::Succeeded: return "succeeded";
    case BuildRecord::State::Failed:    return "failed";
    case BuildRecord::State::Canceled:  return "canceled";
    }
    return "unknown";
}

static QJsonObject buildStatusObject(const BuildRecord &record)
{
    const bool running = record.state == BuildRecord::State::Running;
    QJsonObject status{
        {"build_id", qint64(record.id)},
        {"state", buildStateName(record.state)},
        {"error_count",
         running ? BuildRecord::addedSinceStart(
             BuildManager::getErrorTaskCount(), record.baselineErrorCount)
                 : record.errorCount},
        {"warning_count",
         running ? BuildRecord::addedSinceStart(
             BuildManager::getWarningTaskCount(), record.baselineWarningCount)
                 : record.warningCount}};
    if (!record.projectName.isEmpty())
        status.insert("project", record.projectName);
    if (running) {
        status.insert("elapsed_ms", record.elapsed.elapsed());
        if (const std::optional<QPair<int, QString>> progress
            = BuildManager::currentProgressPercent()) {
            status.insert("progress_percent", progress->first);
            status.insert("current_step", progress->second);
        }
    } else {
        status.insert("duration_ms", record.durationMs);
    }
    return status;
}

static Utils::Result<qint64> readNumber(
    const QJsonObject &args, const QString &key, qint64 fallback)
{
    const QJsonValue value = args.value(key);
    if (value.isUndefined() || value.isNull())
        return fallback;
    if (!value.isDouble())
        return ResultError(QString("%1 must be a number").arg(key));
    // Converting out of range is UB, and qint64's max has no exact double, so
    // the bound is 2^63 -- the first double above it.
    const double d = value.toDouble();
    if (!(d >= -9223372036854775808.0 && d < 9223372036854775808.0))
        return ResultError(QString("%1 is out of range").arg(key));
    return qint64(d);
}

struct BuildLookup
{
    BuildRecord *record = nullptr;
    QString reason;  // the state to report back when record is null
    QString message;
};

static BuildLookup lookupBuild(const QJsonObject &args)
{
    const Utils::Result<qint64> id = readNumber(args, "build_id", 0);
    if (!id)
        return {nullptr, "invalid_arguments", id.error()};
    if (*id < 0)
        return {nullptr, "invalid_arguments", "build_id must not be negative"};
    if (*id == 0) {
        if (buildRecords().isEmpty()) {
            return {nullptr, "never_built",
                    "No build has run since Qt Creator started. Start one with "
                    "build_project."};
        }
        return {&buildRecords().last(), {}, {}};
    }
    BuildRecord *record = buildRecord(quint64(*id));
    if (!record) {
        return {nullptr, "unknown_build_id",
                QString("No build with id %1 is known: only the last %2 are kept, so it has "
                        "been replaced by newer builds, or it predates a restart.")
                    .arg(*id)
                    .arg(keptBuilds)};
    }
    return {record, {}, {}};
}

static QJsonObject startBuild(const QJsonObject &args)
{
    const ProjectResolution resolution = resolveTargetProject(
        args.value("project_name").toString(), args.value("project_path").toString(), true);
    if (!resolution.project) {
        QJsonObject body = resolution.error;
        body.remove("success"); // this tool reports through "started"
        body["started"] = false;
        return body;
    }

    reclaimStaleBuild();
    if (const BuildRecord *running = runningBuild()) {
        return {{"started", false},
                {"build_id", qint64(running->id)},
                {"reason", "build_in_progress"},
                {"message",
                 QString("A build%1 is already running. Wait for build_id %2 with "
                         "build_get_status, then call build_project again.")
                     .arg(running->projectName.isEmpty()
                              ? QString()
                              : QString(" of '%1'").arg(running->projectName))
                     .arg(running->id)}};
    }

    // Before buildProjects(): a queue with nothing to do emits
    // buildQueueFinished from inside it.
    const quint64 id = beginBuildRecord(resolution.project).id;
    if (BuildManager::buildProjects({resolution.project}, ConfigSelection::Active) <= 0) {
        if (const BuildRecord *ours = runningBuild(); ours && ours->id == id) {
            runningBuildId = 0;
            buildRecords().removeIf([id](const BuildRecord &record) { return record.id == id; });
        }
        return {{"started", false},
                {"reason", "build_failed_to_start"},
                {"message",
                 "Nothing was queued to build. Check that the project is configured for "
                 "the active kit."}};
    }
    return {{"started", true},
            {"build_id", qint64(id)},
            {"reason", "ok"},
            {"message", QString("Building '%1'. Wait for it with build_get_status.")
                            .arg(resolution.project->displayName())}};
}

static QJsonObject cancelBuild(const QJsonObject &args)
{
    const BuildLookup lookup = lookupBuild(args);
    if (!lookup.record)
        return {{"canceled", false}, {"reason", lookup.reason}, {"message", lookup.message}};
    if (lookup.record->state != BuildRecord::State::Running) {
        return {{"canceled", false},
                {"build_id", qint64(lookup.record->id)},
                {"reason", "not_running"},
                {"message", QString("Build %1 already ended as %2.")
                                .arg(lookup.record->id)
                                .arg(buildStateName(lookup.record->state))}};
    }
    const quint64 id = lookup.record->id;
    BuildManager::cancel();
    return {{"canceled", true},
            {"build_id", qint64(id)},
            {"reason", "ok"},
            {"message", QString("Cancelled build %1.").arg(id)}};
}

static QString severityWord(const QString &type)
{
    if (type == "ERROR")
        return "error";
    if (type == "WARNING")
        return "warning";
    return "info";
}

static QString issueLine(
    const QJsonObject &issue, const FilePath &baseDir, bool withDetails, bool *relative)
{
    QString location;
    const FilePath file = FilePath::fromUserInput(issue.value("file").toString());
    if (!file.isEmpty()) {
        if (!baseDir.isEmpty() && file.isChildOf(baseDir)) {
            location = file.relativeChildPath(baseDir).path();
            *relative = true;
        } else {
            location = file.toUserOutput();
        }
        const int line = issue.value("line").toInt();
        if (line > 0)
            location += ':' + QString::number(line);
        location += ": ";
    }
    // Task::description() is the summary followed by the details.
    const QString description = issue.value("description").toString();
    const int firstBreak = description.indexOf('\n');
    QString text = firstBreak < 0 ? description : description.left(firstBreak);
    if (withDetails && firstBreak >= 0) {
        QStringList details = description.mid(firstBreak + 1).split('\n');
        // A compiler opens its detail block by restating the message the
        // summary already is, at a path this line already names.
        if (!text.isEmpty() && details.first().endsWith(text))
            details.removeFirst();
        if (!details.isEmpty())
            text += '\n' + details.join('\n');
    }
    return location + severityWord(issue.value("type").toString()) + ": " + text;
}

static QJsonObject buildIssuesReply(const QJsonObject &args)
{
    const auto emptyReply = [](const QString &reason, const QString &message) {
        return QJsonObject{{"reason", reason},
                           {"message", message},
                           {"issues", QJsonArray{}},
                           {"error_count", 0},
                           {"warning_count", 0},
                           {"total", 0},
                           {"truncated", false}};
    };

    const Utils::Result<int> requested = readCount(args, "max", defaultIssueCount);
    if (!requested)
        return emptyReply("invalid_arguments", requested.error());
    const Utils::Result<int> requestedOffset = readCount(args, "offset", 0);
    if (!requestedOffset)
        return emptyReply("invalid_arguments", requestedOffset.error());
    const int maxIssues = qBound(1, *requested, maxIssueCount);
    const int skip = qMax(0, *requestedOffset);

    QJsonArray issues;
    FilePath baseDir;
    quint64 buildId = 0;
    if (args.value("scope").toString("build") == "current") {
        issues = issuesManager().getCurrentIssues().value("issues").toArray();
        if (const Project *project = ProjectManager::startupProject())
            baseDir = project->projectDirectory();
    } else {
        const BuildLookup lookup = lookupBuild(args);
        if (!lookup.record)
            return emptyReply(lookup.reason, lookup.message);
        if (lookup.record->state == BuildRecord::State::Running) {
            QJsonObject body = emptyReply(
                "still_building",
                QString("Build %1 has not finished; its issues are collected when it does. "
                        "Wait for it with build_get_status.")
                    .arg(lookup.record->id));
            body["build_id"] = qint64(lookup.record->id);
            return body;
        }
        issues = lookup.record->issues;
        baseDir = lookup.record->projectDir;
        buildId = lookup.record->id;
    }

    const bool withDetails = args.value("details").toBool();
    const FilePath file = FilePath::fromUserInput(args.value("file").toString());
    const auto matchesFile = [&file](const QJsonObject &issue) {
        return file.isEmpty()
               || FilePath::fromUserInput(issue.value("file").toString()) == file;
    };

    int errorCount = 0;
    int warningCount = 0;
    for (const QJsonValue &value : std::as_const(issues)) {
        const QJsonObject issue = value.toObject();
        if (!matchesFile(issue))
            continue;
        const QString type = issue.value("type").toString();
        if (type == "ERROR")
            ++errorCount;
        else if (type == "WARNING")
            ++warningCount;
    }

    QString severity = args.value("severity").toString("auto");
    if (severity == "auto")
        severity = errorCount > 0 ? QStringLiteral("error") : QStringLiteral("warning");

    QJsonArray lines;
    int matched = 0;
    bool relative = false;
    for (const QJsonValue &value : std::as_const(issues)) {
        const QJsonObject issue = value.toObject();
        if (!matchesFile(issue))
            continue;
        const QString type = issue.value("type").toString();
        const bool wanted = severity == "all" || (severity == "error" && type == "ERROR")
                            || (severity == "warning" && type == "WARNING");
        if (!wanted)
            continue;
        ++matched;
        if (matched <= skip || lines.size() >= maxIssues)
            continue;
        lines.append(issueLine(issue, baseDir, withDetails, &relative));
    }

    QJsonObject reply{{"issues", lines},
                      {"severity", severity},
                      {"error_count", errorCount},
                      {"warning_count", warningCount},
                      {"total", matched},
                      {"truncated", matched > skip + lines.size()}};
    if (buildId != 0)
        reply.insert("build_id", qint64(buildId));
    if (relative)
        reply.insert("base_dir", baseDir.toUserOutput());
    return reply;
}

static QJsonObject compileOutputReply(const QJsonObject &args)
{
    const Utils::Result<int> requested = readCount(args, "max_chars", maxReplyOutputSize);
    if (!requested) {
        return {{"output", QString()},
                {"truncated", false},
                {"total_chars", 0},
                {"reason", "invalid_arguments"},
                {"message", requested.error()}};
    }
    const int maxChars = qBound(minReplyOutputSize, *requested, maxRequestableOutputSize);

    if (args.value("scope").toString("build") == "session") {
        const BoundedOutput &captured = compileOutput();
        return {{"output", captured.tail(maxChars)},
                {"truncated", captured.truncatedAt(maxChars)},
                {"total_chars", captured.total()}};
    }

    const BuildLookup lookup = lookupBuild(args);
    if (!lookup.record) {
        return {{"output", QString()},
                {"truncated", false},
                {"total_chars", 0},
                {"reason", lookup.reason},
                {"message", lookup.message}};
    }
    const BoundedOutput &output = lookup.record->output;
    return {{"build_id", qint64(lookup.record->id)},
            {"output", output.digest(maxChars)},
            {"truncated", output.truncatedAt(maxChars)},
            {"total_chars", output.total()}};
}

static QStringList findFiles(const QList<Project *> &projects, const QRegularExpression &re)
{
    QStringList result;
    for (auto project : projects) {
        const FilePaths matches = project->files([&re](const Node *n) {
            return !n->filePath().isEmpty() && re.match(n->filePath().fileName()).hasMatch();
        });
        result.append(Utils::transform(matches, &FilePath::toUserOutput));
    }
    return result;
}

static QJsonArray listProjects()
{
    QJsonArray result;
    const Project *startup = ProjectManager::startupProject();
    for (Project *project : ProjectManager::projects()) {
        QJsonObject obj = projectInfoObject(project);
        obj["is_active"] = (project == startup);
        result.append(obj);
    }
    return result;
}

static QStringList listBuildConfigs()
{
    QStringList configs;
    Project *project = ProjectManager::startupProject();
    if (!project)
        return configs;
    Target *target = project->activeTarget();
    if (!target)
        return configs;
    for (BuildConfiguration *config : target->buildConfigurations())
        configs.append(config->displayName());
    return configs;
}

static bool switchToBuildConfig(const QString &name)
{
    if (name.isEmpty())
        return false;
    Project *project = ProjectManager::startupProject();
    if (!project)
        return false;
    Target *target = project->activeTarget();
    if (!target)
        return false;
    for (BuildConfiguration *config : target->buildConfigurations()) {
        if (config->displayName() == name) {
            target->setActiveBuildConfiguration(config, SetActive::Cascade);
            return true;
        }
    }
    return false;
}

static QJsonObject addBuildConfig(const QString &buildType, bool setActive)
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return {{"success", false}, {"error", "No active project."}};
    Target *target = project->activeTarget();
    if (!target)
        return {{"success", false}, {"error", "No active target."}};
    BuildConfigurationFactory *factory = BuildConfigurationFactory::find(target);
    if (!factory)
        return {{"success", false}, {"error", "No build configuration factory for the active kit."}};

    const QList<BuildInfo> builds = factory->allAvailableBuilds(target);
    QStringList available;
    for (const BuildInfo &b : builds)
        available.append(b.displayName.isEmpty() ? b.typeName : b.displayName);

    const auto match = Utils::findOrDefault(builds, [&buildType](const BuildInfo &b) {
        return b.displayName.compare(buildType, Qt::CaseInsensitive) == 0
               || b.typeName.compare(buildType, Qt::CaseInsensitive) == 0;
    });
    if (match.factory == nullptr) {
        return {{"success", false},
                {"error", QString("No build type \"%1\" for this kit. Available: %2")
                              .arg(buildType, available.join(", "))}};
    }

    // allAvailableBuilds() may leave displayName empty (the GUI prompts for one, defaulting to
    // the type name); give the configuration a stable, unique name here.
    BuildInfo chosen = match;
    if (chosen.displayName.isEmpty())
        chosen.displayName = chosen.typeName.isEmpty() ? buildType : chosen.typeName;
    QStringList existingNames;
    for (BuildConfiguration *bc : target->buildConfigurations())
        existingNames.append(bc->displayName());
    chosen.displayName = Utils::makeUniquelyNumbered(chosen.displayName, existingNames);

    const BuildInfo info = BuildConfiguration::fixupBuildInfo(
        chosen, target->kit(), project->projectFilePath());
    BuildConfiguration *bc = info.factory->create(target, info);
    if (!bc)
        return {{"success", false}, {"error", "Failed to create the build configuration."}};

    target->addBuildConfiguration(bc);
    if (setActive)
        target->setActiveBuildConfiguration(bc, SetActive::Cascade);
    return {{"success", true}, {"name", bc->displayName()}, {"active", setActive}};
}

static QJsonObject getCurrentProject()
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return {};
    return {
        {"project_name", project->displayName()},
        {"project_file", project->projectFilePath().toUserOutput()},
        {"project_directory", project->projectDirectory().toUserOutput()},
    };
}

static QJsonObject kitInfoObject(const Kit *k)
{
    if (!k)
        return {};
    const DetectionSource ds = k->detectionSource();
    QJsonArray issues;
    for (const Task &t : k->validate()) {
        if (!t.summary().isEmpty())
            issues.append(t.summary());
    }
    const auto deviceName = [](Utils::Id deviceId) -> QString {
        const IDevice::ConstPtr device = DeviceManager::find(deviceId);
        return device ? device->displayName() : QString();
    };
    return {
        {"name", k->displayName()},
        {"id", k->id().toString()},
        {"valid", k->isValid()},
        {"has_warning", k->hasWarning()},
        {"is_default", k == KitManager::defaultKit()},
        {"auto_detected", ds.isAutoDetected()},
        {"sdk_provided", ds.isSdkProvided()},
        {"file_system_friendly_name", k->fileSystemFriendlyName()},
        {"run_device_id", RunDeviceKitAspect::deviceId(k).toString()},
        {"run_device", deviceName(RunDeviceKitAspect::deviceId(k))},
        {"build_device_id", BuildDeviceKitAspect::deviceId(k).toString()},
        {"build_device", deviceName(BuildDeviceKitAspect::deviceId(k))},
        {"issues", issues},
    };
}

static QJsonArray listKits()
{
    QJsonArray result;
    for (Kit *k : KitManager::sortedKits())
        result.append(kitInfoObject(k));
    return result;
}

// Returns the kits a project is configured for (one per Target), with is_active
// marking the kit of the project's active target. Defaults to the startup
// project when neither projectName nor projectPath is supplied.
static QJsonObject projectKits(const QString &projectName, const QString &projectPath)
{
    const ProjectResolution resolution = resolveTargetProject(projectName, projectPath, true);
    if (!resolution.project)
        return resolution.error;
    Project *project = resolution.project;

    const Target *activeTarget = project->activeTarget();
    QJsonArray kits;
    for (const Target *target : project->targets()) {
        QJsonObject obj = kitInfoObject(target->kit());
        obj["is_active"] = (target == activeTarget);
        kits.append(obj);
    }

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", QString("Project '%1' is configured for %2 kit(s).")
                        .arg(project->displayName())
                        .arg(kits.size())},
        {"project", projectInfoObject(project)},
        {"kits", kits}};
}

// Resolves a kit by id or, failing that, by display name. A name several kits share is
// refused rather than resolved to the first match, since nothing tells them apart. The
// error is the tool reply for the failure, so each tool reports the same way.
struct KitResolution
{
    Kit *kit = nullptr;
    QJsonObject error;
};

static KitResolution resolveKit(const QString &kitIdentifier)
{
    if (kitIdentifier.isEmpty()) {
        return {nullptr, {{"success", false}, {"reason", "no_kit"},
                          {"message", "No kit specified."}}};
    }

    if (Kit * const kit = KitManager::kit(Utils::Id::fromString(kitIdentifier)))
        return {kit, {}};

    const QList<Kit *> byName = Utils::filtered(
        KitManager::kits(),
        [&kitIdentifier](const Kit *k) { return k->displayName() == kitIdentifier; });
    if (byName.size() == 1)
        return {byName.first(), {}};

    if (byName.size() > 1) {
        QJsonArray candidates;
        for (const Kit *k : byName)
            candidates.append(kitInfoObject(k));
        return {nullptr, {{"success", false},
                          {"reason", "ambiguous_name"},
                          {"message", QString("%1 kits are called \"%2\"; pass a kit id instead.")
                                          .arg(byName.size())
                                          .arg(kitIdentifier)},
                          {"candidates", candidates}}};
    }

    return {nullptr, {{"success", false},
                      {"reason", "not_found"},
                      {"message", QString("No kit matching \"%1\".").arg(kitIdentifier)}}};
}

// The same failure as one entry of a tool that works through a list of kits.
static QJsonObject kitResolutionResult(const QString &kitIdentifier, const QJsonObject &error)
{
    QJsonObject result{{"kit", kitIdentifier},
                       {"status", error.value("reason")},
                       {"message", error.value("message")}};
    if (error.contains("candidates"))
        result.insert("candidates", error.value("candidates"));
    return result;
}

// Adds a build target for each requested kit to a project. `kitIdentifiers` may
// hold kit ids or display names. Defaults to the startup project when neither
// projectName nor projectPath is supplied. Reports per-kit results
// (added/already_present/not_found/failed) without aborting on the first error.
static QJsonObject addKitsToProject(
    const QString &projectName, const QString &projectPath, const QStringList &kitIdentifiers)
{
    const ProjectResolution resolution = resolveTargetProject(projectName, projectPath, true);
    if (!resolution.project)
        return resolution.error;
    Project *project = resolution.project;

    if (kitIdentifiers.isEmpty())
        return {{"success", false}, {"reason", "no_kits"}, {"message", "No kits specified."}};

    QJsonArray results;
    int addedCount = 0;
    for (const QString &identifier : kitIdentifiers) {
        const KitResolution kitResolution = resolveKit(identifier);
        if (!kitResolution.kit) {
            results.append(kitResolutionResult(identifier, kitResolution.error));
            continue;
        }
        Kit * const kit = kitResolution.kit;
        if (project->target(kit)) {
            results.append(QJsonObject{
                {"kit", kit->displayName()}, {"id", kit->id().toString()},
                {"status", "already_present"}});
            continue;
        }
        Target *target = project->addTargetForKit(kit);
        if (target) {
            ++addedCount;
            results.append(QJsonObject{
                {"kit", kit->displayName()}, {"id", kit->id().toString()},
                {"status", "added"}});
        } else {
            results.append(QJsonObject{
                {"kit", kit->displayName()}, {"id", kit->id().toString()},
                {"status", "failed"},
                {"message", QString("Failed to add kit '%1'.").arg(kit->displayName())}});
        }
    }

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", QString("Added %1 kit(s) to project '%2'.")
                        .arg(addedCount)
                        .arg(project->displayName())},
        {"project", projectInfoObject(project)},
        {"results", results}};
}

// Renames a kit, as editing its name on the Kits preferences page does. A display name
// shared by several kits has to be told apart by id, which is the case renaming is for.
static QJsonObject renameKit(const QString &kitIdentifier, const QString &name)
{
    if (name.isEmpty())
        return {{"success", false}, {"reason", "no_name"}, {"message", "No name specified."}};

    const KitResolution resolution = resolveKit(kitIdentifier);
    if (!resolution.kit)
        return resolution.error;
    Kit * const kit = resolution.kit;

    const QString previous = kit->displayName();
    kit->setUnexpandedDisplayName(name);

    // The name is not made unique here, as it is for a clone on the Kits page: a caller
    // may want one deliberately, in two steps. But sharing one is what this tool exists
    // to undo, so it is reported rather than left to be discovered.
    const QList<Kit *> sharing = Utils::filtered(KitManager::kits(), [kit](const Kit *k) {
        return k != kit && k->displayName() == kit->displayName();
    });
    QJsonArray shared;
    for (const Kit *k : sharing)
        shared.append(kitInfoObject(k));

    QString message = QString("Kit \"%1\" is now called \"%2\".").arg(previous, kit->displayName());
    if (sharing.size() == 1) {
        message += QString(" One other kit carries that name as well; kits sharing a name "
                           "share a build directory.");
    } else if (sharing.size() > 1) {
        message += QString(" %1 other kits carry that name as well; kits sharing a name "
                           "share a build directory.").arg(sharing.size());
    }

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", message},
        {"previous_name", previous},
        {"kit", kitInfoObject(kit)},
        {"shared_name", !sharing.isEmpty()},
        {"shares_name_with", shared}};
}

// Makes a project's target for a kit the active one, as choosing it in the kit selector
// does. A display name shared by several kits is refused rather than guessed at, since
// nothing distinguishes them; the id from kit_list always identifies one.
static QJsonObject setActiveKit(
    const QString &projectName, const QString &projectPath, const QString &kitIdentifier)
{
    const ProjectResolution resolution = resolveTargetProject(projectName, projectPath, true);
    if (!resolution.project)
        return resolution.error;
    Project *project = resolution.project;

    const KitResolution kitResolution = resolveKit(kitIdentifier);
    if (!kitResolution.kit)
        return kitResolution.error;
    Kit * const kit = kitResolution.kit;

    Target * const target = project->target(kit);
    if (!target) {
        return {
            {"success", false},
            {"reason", "kit_not_configured"},
            {"message", QString("Project \"%1\" is not configured for kit \"%2\"; add it with "
                                "kit_add_to_project first.")
                            .arg(project->displayName(), kit->displayName())}};
    }

    const bool wasActive = project->activeTarget() == target;
    project->setActiveTarget(target, SetActive::Cascade);

    const QString message
        = (wasActive ? QString("Kit \"%1\" was already active for project \"%2\".")
                     : QString("Kit \"%1\" is now active for project \"%2\"."))
              .arg(kit->displayName(), project->displayName());

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", message},
        {"project", projectInfoObject(project)},
        {"kit", kitInfoObject(kit)},
        {"already_active", wasActive}};
}

// Removes kits identified by kit id or display name. SDK-provided kits are refused (as in
// the Kits preferences page), and so is a display name shared by several kits, since
// removal is not undoable. Reports per-kit results without aborting on the first error.
static QJsonObject removeKits(const QStringList &kitIdentifiers)
{
    if (kitIdentifiers.isEmpty())
        return {{"success", false}, {"reason", "no_kits"}, {"message", "No kits specified."}};

    QList<Kit *> toRemove;
    QJsonArray results;
    for (const QString &identifier : kitIdentifiers) {
        const KitResolution resolution = resolveKit(identifier);
        if (!resolution.kit) {
            results.append(kitResolutionResult(identifier, resolution.error));
            continue;
        }
        Kit * const kit = resolution.kit;
        if (kit->detectionSource().isSdkProvided()) {
            results.append(QJsonObject{
                {"kit", kit->displayName()},
                {"id", kit->id().toString()},
                {"status", "sdk_provided"},
                {"message", QString("Kit '%1' is provided by an SDK and cannot be removed.")
                                .arg(kit->displayName())}});
            continue;
        }
        if (toRemove.contains(kit))
            continue;
        toRemove.append(kit);
        results.append(QJsonObject{
            {"kit", kit->displayName()}, {"id", kit->id().toString()}, {"status", "removed"}});
    }

    // Deregistering deletes the kits, so this comes after all of them were reported on.
    KitManager::deregisterKits(toRemove);

    return {
        {"success", true},
        {"reason", "ok"},
        {"message", QString("Removed %1 kit(s).").arg(toRemove.size())},
        {"results", results}};
}

static QJsonObject openProjectFile(const QString &path)
{
    const FilePath fp = FilePath::fromUserInput(path);
    if (fp.isEmpty())
        return {{"success", false}, {"message", "Empty project path."}};
    if (!fp.exists()) {
        return {
            {"success", false},
            {"message", QString("Path does not exist: %1").arg(fp.toUserOutput())}};
    }

    const OpenProjectResult result = ProjectExplorerPlugin::openProject(fp);

    if (!result.alreadyOpen().isEmpty()) {
        Project *p = result.alreadyOpen().first();
        return {
            {"success", true},
            {"already_open", true},
            {"message",
             QString("Project '%1' is already open.").arg(p ? p->displayName() : path)},
            {"project", p ? QJsonValue(projectInfoObject(p)) : QJsonValue()}};
    }

    if (!result) {
        return {
            {"success", false},
            {"message",
             result.errorMessage().isEmpty()
                 ? QString("Failed to open project: %1").arg(fp.toUserOutput())
                 : result.errorMessage()}};
    }

    Project *p = result.project();
    return {
        {"success", true},
        {"already_open", false},
        {"message", QString("Opened project '%1'.").arg(p ? p->displayName() : path)},
        {"project", p ? QJsonValue(projectInfoObject(p)) : QJsonValue()}};
}

static QString getCurrentBuildConfig()
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return {};
    Target *target = project->activeTarget();
    if (!target)
        return {};
    BuildConfiguration *buildConfig = target->activeBuildConfiguration();
    return buildConfig ? buildConfig->displayName() : QString{};
}

static Result<QStringList> projectDependencies(const QString &projectName)
{
    for (Project *candidate : ProjectManager::projects()) {
        if (candidate->displayName() == projectName) {
            QStringList projects;
            for (Project *project : ProjectManager::dependencies(candidate))
                projects.append(project->displayName());
            return projects;
        }
    }
    return ResultError("No project found with name: " + projectName);
}

static QMap<QString, QSet<QString>> knownRepositoriesInProject(const QString &projectName)
{
    const FilePaths projectDirectories
        = Utils::transform(projectsForName(projectName), &Project::projectDirectory);
    if (projectDirectories.isEmpty())
        return {};
    QMap<QString, QSet<QString>> repos;
    const QList<Core::IVersionControl *> versionControls = Core::VcsManager::versionControls();
    for (const Core::IVersionControl *vcs : versionControls) {
        const FilePaths repositories = Utils::filteredUnique(Core::VcsManager::repositories(vcs));
        for (const FilePath &repo : repositories) {
            if (Utils::anyOf(projectDirectories, [repo](const FilePath &projectDir) {
                    return repo == projectDir || repo.isChildOf(projectDir);
                })) {
                repos[vcs->displayName()].insert(repo.toUserOutput());
            }
        }
    }
    return repos;
}

static QJsonArray getRunConfigurations()
{
    QJsonArray result;
    Project *project = ProjectManager::startupProject();
    if (!project)
        return result;
    Target *target = project->activeTarget();
    if (!target)
        return result;
    BuildConfiguration *bc = target->activeBuildConfiguration();
    if (!bc)
        return result;
    RunConfiguration *activeRc = target->activeRunConfiguration();
    for (RunConfiguration *rc : bc->runConfigurations()) {
        QJsonObject obj;
        obj["name"] = rc->expandedDisplayName();
        obj["id"] = rc->id().toString();
        obj["active"] = (rc == activeRc);

        const ProcessRunData runnable = rc->runnable();
        obj["executable"] = runnable.command.executable().toUserOutput();
        const QString arguments = runnable.command.arguments();
        if (!arguments.isEmpty())
            obj["arguments"] = arguments;
        if (!runnable.workingDirectory.isEmpty())
            obj["workingDirectory"] = runnable.workingDirectory.toUserOutput();

        QJsonObject env;
        runnable.environment.forEachEntry(
            [&env](const QString &key, const QString &value, bool enabled) {
                if (enabled)
                    env.insert(key, value);
            });
        obj["environment"] = env;

        result.append(obj);
    }
    return result;
}

static QJsonObject configureRunConfig(const QString &idOrName,
                                      const QString &executable,
                                      const std::optional<QString> &arguments,
                                      bool setActive)
{
    Project *project = ProjectManager::startupProject();
    if (!project)
        return {{"success", false}, {"reason", "no_project"}, {"message", "No startup project."}};
    Target *target = project->activeTarget();
    if (!target)
        return {{"success", false}, {"reason", "no_target"}, {"message", "No active target."}};
    BuildConfiguration *bc = target->activeBuildConfiguration();
    if (!bc) {
        return {{"success", false}, {"reason", "no_build_config"},
                {"message", "No active build configuration."}};
    }
    // A type id is shared by every run configuration its factory created, so it
    // can match several; the display name is the selector that identifies one.
    QList<RunConfiguration *> byName;
    QList<RunConfiguration *> byId;
    for (RunConfiguration *rc : bc->runConfigurations()) {
        if (rc->expandedDisplayName() == idOrName)
            byName.append(rc);
        else if (rc->id().toString() == idOrName)
            byId.append(rc);
    }
    const QList<RunConfiguration *> &matches = byName.isEmpty() ? byId : byName;
    if (matches.isEmpty()) {
        return {{"success", false}, {"reason", "not_found"},
                {"message", QString("No run configuration matching \"%1\".").arg(idOrName)}};
    }
    if (matches.size() > 1) {
        QJsonArray candidates;
        for (RunConfiguration *rc : matches)
            candidates.append(rc->expandedDisplayName());
        return {{"success", false}, {"reason", "ambiguous"},
                {"candidates", candidates},
                {"message", QString("\"%1\" matches %2 run configurations. Pass one of "
                                    "\"candidates\" as \"id\" instead.")
                                .arg(idOrName).arg(matches.size())}};
    }
    RunConfiguration *match = matches.first();
    if (!executable.isEmpty()) {
        auto aspect = match->aspect<ExecutableAspect>();
        if (!aspect) {
            return {{"success", false}, {"reason", "no_executable_aspect"},
                    {"message", "Run configuration has no executable aspect."}};
        }
        aspect->setExecutable(FilePath::fromUserInput(executable));
    }
    if (arguments) {
        auto aspect = match->aspect<ArgumentsAspect>();
        if (!aspect) {
            return {{"success", false}, {"reason", "no_arguments_aspect"},
                    {"message", "Run configuration has no arguments aspect."}};
        }
        aspect->setArguments(*arguments);
    }
    if (setActive)
        bc->setActiveRunConfiguration(match);
    return {{"success", true},
            {"reason", "ok"},
            {"id", match->id().toString()},
            {"name", match->expandedDisplayName()},
            {"active", match == target->activeRunConfiguration()},
            {"executable", match->runnable().command.executable().toUserOutput()},
            {"arguments", match->runnable().command.arguments()}};
}

// Helper: compute FindFlags from regex/caseSensitive booleans
static FindFlags mcpFindFlags(bool regex, bool caseSensitive)
{
    FindFlags flags;
    if (regex)
        flags |= FindRegularExpression;
    if (caseSensitive)
        flags |= FindCaseSensitively;
    return flags;
}

// Search helper used by search_projects
using McpResponseCallback = std::function<void(const QJsonObject &)>;

static void mcpFindInFiles(
    FileContainer fileContainer,
    bool regex,
    bool caseSensitive,
    const QString &pattern,
    int maxResults,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern,
        fileContainer,
        mcpFindFlags(regex, caseSensitive),
        TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback, maxResults](
                                         const QFuture<SearchResultItems> &future) {
        QJsonArray resultsArray;
        qint64 total = 0;
        for (const Utils::SearchResultItems &results : future.results()) {
            for (const SearchResultItem &item : results) {
                ++total;
                if (resultsArray.size() >= maxResults)
                    continue;
                QJsonObject resultObj;
                const Text::Range range = item.mainRange();
                const QString lineText = item.lineText();
                const int startCol = range.begin.column;
                const int endCol = range.end.column;
                const QString matchedText = lineText.mid(startCol, endCol - startCol);

                resultObj["file"] = item.path().value(0, QString());
                resultObj["line"] = range.begin.line;
                resultObj["column"] = startCol + 1;
                resultObj["text"] = matchedText;
                resultsArray.append(resultObj);
            }
        }
        QJsonObject response;
        response["results"] = resultsArray;
        response["total_matches"] = total;
        response["truncated"] = total > resultsArray.size();
        callback(response);
    });
}

// Replace helper used by fs_replace_in_projects
static void mcpReplace(
    FileContainer fileContainer,
    bool regex,
    bool caseSensitive,
    const QString &pattern,
    const QString &replacement,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QFuture<SearchResultItems> future = Utils::findInFiles(
        pattern,
        fileContainer,
        mcpFindFlags(regex, caseSensitive),
        TextEditor::TextDocument::openedTextDocumentContents());
    Utils::onFinished(future, guard, [callback, replacement](const QFuture<SearchResultItems> &future) {
        QJsonObject response;
        bool success = true;

        TextEditor::PlainRefactoringFileFactory changes;
        QHash<Utils::FilePath, TextEditor::RefactoringFilePtr> refactoringFiles;

        for (const SearchResultItems &results : future.results()) {
            for (const SearchResultItem &item : results) {
                Text::Range range = item.mainRange();
                if (range.begin >= range.end)
                    continue;
                const FilePath filePath = FilePath::fromUserInput(item.path().value(0));
                if (filePath.isEmpty())
                    continue;
                TextEditor::RefactoringFilePtr refactoringFile = refactoringFiles.value(filePath);
                if (!refactoringFile)
                    refactoringFile
                        = refactoringFiles.insert(filePath, changes.file(filePath)).value();
                const int start = refactoringFile->position(range.begin);
                const int end = refactoringFile->position(range.end);
                ChangeSet changeSet = refactoringFile->changeSet();
                changeSet.replace(ChangeSet::Range(start, end), replacement);
                refactoringFile->setChangeSet(changeSet);
            }
        }

        for (auto refactoringFile : refactoringFiles) {
            if (!refactoringFile->apply())
                success = false;
        }

        response["ok"] = success;
        callback(response);
    });
}

static void searchInFiles(
    const QString &filePattern,
    const std::optional<QString> &projectName,
    const QString &pattern,
    bool regex,
    bool caseSensitive,
    int maxResults,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QList<Project *> projects = projectName ? projectsForName(*projectName)
                                                  : ProjectManager::projects();

    const FilterFilesFunction filterFiles
        = Utils::filterFilesFunction({filePattern.isEmpty() ? "*" : filePattern}, {});
    const QMap<FilePath, TextEncoding> openEditorEncodings
        = TextEditor::TextDocument::openedTextDocumentEncodings();
    QMap<FilePath, TextEncoding> encodings;
    for (const Project *project : projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        TextEncoding projectEncoding = config->useGlobalSettings()
                                           ? Core::EditorManager::defaultTextEncoding()
                                           : config->textEncoding();
        const FilePaths filteredFiles = filterFiles(project->files(
            Core::Find::hasFindFlag(DontFindGeneratedFiles) ? Project::SourceFiles
                                                            : Project::AllFiles));
        for (const FilePath &fileName : filteredFiles) {
            TextEncoding encoding = openEditorEncodings.value(fileName);
            if (!encoding.isValid())
                encoding = projectEncoding;
            encodings.insert(fileName, encoding);
        }
    }
    FileListContainer fileContainer(encodings.keys(), encodings.values());
    mcpFindInFiles(fileContainer, regex, caseSensitive, pattern, maxResults, guard, callback);
}

static void replaceInFiles(
    const QString &filePattern,
    const std::optional<QString> &projectName,
    const QString &pattern,
    const QString &replacement,
    bool regex,
    bool caseSensitive,
    QObject *guard,
    const McpResponseCallback &callback)
{
    const QList<Project *> projects = projectName ? projectsForName(*projectName)
                                                  : ProjectManager::projects();

    const FilterFilesFunction filterFiles
        = Utils::filterFilesFunction({filePattern.isEmpty() ? "*" : filePattern}, {});
    const QMap<FilePath, TextEncoding> openEditorEncodings
        = TextEditor::TextDocument::openedTextDocumentEncodings();
    QMap<FilePath, TextEncoding> encodings;
    for (const Project *project : projects) {
        const EditorConfiguration *config = project->editorConfiguration();
        TextEncoding projectEncoding = config->useGlobalSettings()
                                           ? Core::EditorManager::defaultTextEncoding()
                                           : config->textEncoding();
        const FilePaths filteredFiles = filterFiles(project->files(
            Core::Find::hasFindFlag(DontFindGeneratedFiles) ? Project::SourceFiles
                                                            : Project::AllFiles));
        for (const FilePath &fileName : filteredFiles) {
            TextEncoding encoding = openEditorEncodings.value(fileName);
            if (!encoding.isValid())
                encoding = projectEncoding;
            encodings.insert(fileName, encoding);
        }
    }
    FileListContainer fileContainer(encodings.keys(), encodings.values());
    mcpReplace(fileContainer, regex, caseSensitive, pattern, replacement, guard, callback);
}

// --- Device helpers --------------------------------------------------------

static QString deviceStateString(IDevice::DeviceState state)
{
    switch (state) {
    case IDevice::DeviceReadyToUse:
        return "ready";
    case IDevice::DeviceConnected:
        return "connected";
    case IDevice::DeviceDisconnected:
        return "disconnected";
    case IDevice::DeviceStateUnknown:
        break;
    }
    return "unknown";
}

static QString hostKeyCheckingModeString(SshHostKeyCheckingMode mode)
{
    switch (mode) {
    case SshHostKeyCheckingNone:
        return "none";
    case SshHostKeyCheckingStrict:
        return "strict";
    case SshHostKeyCheckingAllowNoMatch:
        break;
    }
    return "allowNoMatch";
}

static SshHostKeyCheckingMode parseHostKeyCheckingMode(
    const QString &s, SshHostKeyCheckingMode fallback)
{
    if (s == "none")
        return SshHostKeyCheckingNone;
    if (s == "strict")
        return SshHostKeyCheckingStrict;
    if (s == "allowNoMatch")
        return SshHostKeyCheckingAllowNoMatch;
    return fallback;
}

static QJsonObject deviceToJson(const IDevice::ConstPtr &device)
{
    const SshParameters ssh = device->sshParameters();
    return QJsonObject{
        {"id", device->id().toString()},
        {"type", device->type().toString()},
        {"displayName", device->displayName()},
        {"displayType", device->displayType()},
        {"state", deviceStateString(device->deviceState())},
        {"rootPath", device->rootPath().toUrlishString()},
        {"host", ssh.host()},
        {"port", ssh.port()},
        {"userName", ssh.userName()},
        {"useKeyFile", ssh.authenticationType() == SshParameters::AuthenticationTypeSpecificKey},
        {"privateKeyFile", ssh.privateKeyFile().toUserOutput()},
        {"timeout", ssh.timeout()},
        {"hostKeyCheckingMode", hostKeyCheckingModeString(ssh.hostKeyCheckingMode())},
    };
}

// Overlays the SSH-related fields present in the JSON object onto a base parameter set,
// so callers can update individual fields without resetting the others.
static SshParameters mergeSshParameters(const SshParameters &base, const QJsonObject &params)
{
    SshParameters ssh = base;
    if (params.contains("host"))
        ssh.setHost(params.value("host").toString());
    if (params.contains("port"))
        ssh.setPort(params.value("port").toInt(ssh.port()));
    if (params.contains("userName"))
        ssh.setUserName(params.value("userName").toString());
    if (params.contains("privateKeyFile"))
        ssh.setPrivateKeyFile(FilePath::fromUserInput(params.value("privateKeyFile").toString()));
    if (params.contains("useKeyFile")) {
        ssh.setAuthenticationType(
            params.value("useKeyFile").toBool() ? SshParameters::AuthenticationTypeSpecificKey
                                                 : SshParameters::AuthenticationTypeAll);
    }
    if (params.contains("timeout"))
        ssh.setTimeout(params.value("timeout").toInt(ssh.timeout()));
    if (params.contains("hostKeyCheckingMode")) {
        ssh.setHostKeyCheckingMode(parseHostKeyCheckingMode(
            params.value("hostKeyCheckingMode").toString(), ssh.hostKeyCheckingMode()));
    }
    return ssh;
}

static void applySshParameters(const IDevice::Ptr &device, const QJsonObject &params)
{
    const SshParameters ssh = mergeSshParameters(device->sshParameters(), params);
    device->sshParametersAspectContainer().setSshParameters(ssh);
    device->sshParametersAspectContainer().apply();
}

void registerMcpTools()
{
    using namespace Mcp::Schema;
    namespace Schema = Mcp::Schema;
    using Mcp::ToolInterface;
    using Mcp::ToolRegistry;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    // Helpers for the generic kit-configuration tools below.
    static const auto variantToJson = [](const QVariant &v) -> QJsonValue {
        switch (v.typeId()) {
        case QMetaType::Bool: return v.toBool();
        case QMetaType::Int:
        case QMetaType::LongLong: return double(v.toLongLong());
        case QMetaType::Double: return v.toDouble();
        // An aspect holding one value per language, such as the compiler.
        case QMetaType::QVariantMap: return QJsonObject::fromVariantMap(v.toMap());
        case QMetaType::QStringList:
        case QMetaType::QVariantList: {
            QJsonArray arr;
            for (const QVariant &item : v.toList())
                arr.append(item.toString());
            return arr;
        }
        default:
            if (v.typeId() == qMetaTypeId<Utils::Store>())
                return QJsonObject::fromVariantMap(mapFromStore(Utils::storeFromVariant(v)));
            return v.toString();
        }
    };
    static const auto jsonToVariant = [](const QJsonValue &j) -> QVariant {
        if (j.isBool()) return j.toBool();
        if (j.isDouble()) return j.toDouble();
        if (j.isObject()) return j.toObject().toVariantMap();
        if (j.isArray()) {
            // Through the variant, since QJsonValue::toString() is empty for a number.
            QStringList list;
            for (const QJsonValue &item : j.toArray())
                list << item.toVariant().toString();
            return list;
        }
        return j.toString();
    };
    static const auto kitAspectFactoryById = [](Utils::Id id) -> KitAspectFactory * {
        for (KitAspectFactory *f : KitManager::kitAspectFactories()) {
            if (f->id() == id)
                return f;
        }
        return nullptr;
    };

    using Callback = std::function<void(const QJsonObject &)>;
    using SimplifiedAsyncCallback = std::function<void(const QJsonObject &, const Callback &)>;
    static const auto wrapAsync =
        [](SimplifiedAsyncCallback asyncFunc) -> Mcp::Server::ToolInterfaceCallback {
        return [asyncFunc](
                   const Schema::CallToolRequestParams &params,
                   const ToolInterface &toolInterface) -> Utils::Result<> {
            asyncFunc(params.argumentsAsObject(), [toolInterface](QJsonObject result) {
                toolInterface.finish(CallToolResult{}.isError(false).structuredContent(result));
            });
            return ResultOk;
        };
    };

    issuesManager(); // start tracking issues from now on
    trackBuilds();   // record every build from now on
    compileOutput(); // start capturing build/deploy output from now on

    ToolRegistry::registerTool(
        Tool{}
            .name("build_project")
            .title("Start a build")
            .description(
                "Starts a build of the named project - the startup project when no name is "
                "given - and returns at once with a build_id. The build runs in the "
                "background; this call never waits for it."
                "\n\n"
                "Wait for the verdict with build_get_status, then read the diagnostics with "
                "build_get_issues and the raw text with build_get_compile_output. None of "
                "them are carried here, so a build that succeeds costs one small reply."
                "\n\n"
                "One build runs at a time. When one is already going this starts nothing and "
                "answers reason:\"build_in_progress\" with that build's build_id: wait on "
                "that id, then call again.")
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Project to build. Pass project_path as well when several "
                             "loaded projects share the name."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file (CMakeLists.txt, .pro, ...). "
                             "Identifies the project when display names collide."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "started",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether this call queued a build."}})
                    .addProperty(
                        "build_id",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "The build the other build_ tools answer for. Present both for "
                             "a build this call started and for one it found running."}})
                    .addProperty(
                        "reason",
                        QJsonObject{
                            {"type", "string"},
                            {"enum",
                             QJsonArray{"ok", "build_in_progress", "build_failed_to_start",
                                        "no_startup_project", "project_not_loaded",
                                        "path_not_loaded", "ambiguous_name"}}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "candidates",
                        QJsonObject{
                            {"type", "array"},
                            {"description",
                             "The projects an ambiguous name matched; pass one of their "
                             "paths as project_path."}})
                    .addRequired("started")
                    .addRequired("reason")
                    .addRequired("message"))
            .annotations(
                ToolAnnotations{}
                    .destructiveHint(false)
                    .idempotentHint(false)
                    .openWorldHint(false)
                    .readOnlyHint(false)),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject body = startBuild(params.argumentsAsObject());
            const QString reason = body.value("reason").toString();
            return CallToolResult{}
                .structuredContent(body)
                .isError(reason != "ok" && reason != "build_in_progress");
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("build_get_status")
            .title("Wait for a build and report its verdict")
            .description(
                "Reports a build's state, waiting for it to finish first: a running build "
                "blocks this call for up to wait_ms, a finished one answers at once. "
                "Defaults to the most recent build, whether build_project or the user "
                "started it."
                "\n\n"
                "state:\"running\" means the wait budget ran out, not that anything went "
                "wrong - call again to keep waiting, and repeat until state is something "
                "else. Never sleep between calls; the waiting happens here."
                "\n\n"
                "Counts only. Read the diagnostics with build_get_issues and the raw text "
                "with build_get_compile_output.")
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "build_id",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Build to report on. Defaults to the most recent one."}})
                    .addProperty(
                        "wait_ms",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "How long to wait for a running build before answering "
                             "state:\"running\" (default 45000). Clamped to 0-55000: a "
                             "longer wait outlives the request timeout of typical clients, "
                             "which drops the session and cancels the build. Pass 0 for an "
                             "immediate snapshot."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "state",
                        QJsonObject{
                            {"type", "string"},
                            {"enum",
                             QJsonArray{"running", "succeeded", "failed", "canceled",
                                        "never_built", "unknown_build_id",
                                        "invalid_arguments"}},
                            {"description",
                             "The build's verdict, and the only field that decides "
                             "success. \"running\" means the wait budget ran out and the "
                             "build continues."}})
                    .addProperty("build_id", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "error_count",
                        QJsonObject{
                            {"type", "integer"},
                            {"minimum", 0},
                            {"description",
                             "Errors this build produced. Read them with build_get_issues."}})
                    .addProperty(
                        "warning_count",
                        QJsonObject{
                            {"type", "integer"},
                            {"minimum", 0},
                            {"description",
                             "Warnings this build produced. build_get_issues returns them "
                             "when the build produced no errors, and on request otherwise."}})
                    .addProperty("duration_ms", QJsonObject{{"type", "integer"}, {"minimum", 0}})
                    .addProperty(
                        "elapsed_ms",
                        QJsonObject{
                            {"type", "integer"},
                            {"minimum", 0},
                            {"description", "How long it has been running. Set only while "
                                            "state is \"running\"."}})
                    .addProperty("progress_percent", QJsonObject{{"type", "integer"}})
                    .addProperty("current_step", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "message",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Present only when state names a problem with the request."}})
                    .addRequired("state")
                    .addRequired("error_count")
                    .addRequired("warning_count"))
            .annotations(ToolAnnotations{}.readOnlyHint(true)),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const Utils::Result<qint64> waitMsArg
                = readNumber(args, "wait_ms", defaultBuildWaitMs);
            const BuildLookup lookup = lookupBuild(args);
            if (!waitMsArg || !lookup.record) {
                toolInterface.finish(CallToolResult{}.isError(true).structuredContent(QJsonObject{
                    {"state", waitMsArg ? lookup.reason : QString("invalid_arguments")},
                    {"error_count", 0},
                    {"warning_count", 0},
                    {"message", waitMsArg ? lookup.message : waitMsArg.error()}}));
                return ResultOk;
            }
            const qint64 waitMs = qBound<qint64>(0, *waitMsArg, maxBuildWaitMs);
            if (waitMs == 0 || lookup.record->state != BuildRecord::State::Running) {
                toolInterface.finish(CallToolResult{}.isError(false).structuredContent(
                    buildStatusObject(*lookup.record)));
                return ResultOk;
            }

            struct State
            {
                quint64 id = 0;
                qint64 waitMs = 0;
                QElapsedTimer waited;
                QJsonObject reply;
                std::optional<Schema::TaskStatus> finalStatus;

                QJsonObject snapshot() const
                {
                    if (const BuildRecord *record = buildRecord(id))
                        return buildStatusObject(*record);
                    return {{"state", "unknown_build_id"},
                            {"build_id", qint64(id)},
                            {"error_count", 0},
                            {"warning_count", 0},
                            {"message", "Newer builds replaced this one before its verdict "
                                        "could be read."}};
                }
            };
            auto state = std::make_shared<State>();
            state->id = lookup.record->id;
            state->waitMs = waitMs;
            state->waited.start();

            using namespace std::chrono_literals;
            const auto statusTask = toolInterface.startTask(
                1s,
                [state](Schema::Task task) -> Schema::Task {
                    if (state->finalStatus)
                        return task.status(*state->finalStatus);

                    const BuildRecord *record = buildRecord(state->id);
                    const bool running = record
                                         && record->state == BuildRecord::State::Running;
                    if (running && state->waited.elapsed() < state->waitMs) {
                        if (const std::optional<QPair<int, QString>> progress
                            = BuildManager::currentProgressPercent()) {
                            task.statusMessage(
                                QString("%1 (%2%)").arg(progress->second).arg(progress->first));
                        }
                        return task.status(Schema::TaskStatus::working);
                    }

                    state->reply = state->snapshot();
                    state->finalStatus = Schema::TaskStatus::completed;
                    task.status(*state->finalStatus);
                    task.statusMessage(state->reply.value("state").toString());
                    Mcp::letTaskDieIn(task, 1min);
                    return task;
                },
                [state]() -> Utils::Result<Schema::CallToolResult> {
                    // The result can be collected before the task goes terminal.
                    if (state->reply.isEmpty())
                        state->reply = state->snapshot();
                    return CallToolResult{}.structuredContent(state->reply).isError(false);
                },
                // Abandoning the wait leaves the build running.
                [state]() { state->reply = state->snapshot(); },
                Mcp::progressToken(params));

            if (!statusTask) {
                toolInterface.finish(CallToolResult{}.isError(true).addContent(
                    Schema::TextContent{}.text(statusTask.error())));
            }
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("build_get_issues")
            .title("List build errors and warnings")
            .description(
                "Errors and warnings of a build, one compiler-style line each "
                "(\"src/foo.cpp:42: error: ...\"), relative to base_dir where they lie "
                "under it. Reports on the most recent build unless build_id says "
                "otherwise, and by default returns the errors, or the warnings when "
                "the build produced no errors - so one call covers a build either way. "
                "Pass details:true for the compiler's own echo of each issue, and "
                "scope:\"current\" for everything in the Issues pane rather than one "
                "build's own - a build system's parse errors, for instance, which no "
                "build produced.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "build_id",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Build to report on. Defaults to the most recent one."}})
                    .addProperty(
                        "scope",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"build", "current"}},
                            {"default", "build"},
                            {"description",
                             "\"build\" (default): what the build named by build_id "
                             "produced. \"current\": everything the Issues pane holds now, "
                             "which ignores build_id."}})
                    .addProperty(
                        "severity",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"auto", "error", "warning", "all"}},
                            {"default", "auto"},
                            {"description",
                             "Which issues to return. \"auto\" (default) is the errors, "
                             "or the warnings when there are no errors. error_count and "
                             "warning_count are reported whichever you pick, and the "
                             "reply says which severity it settled on."}})
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path of a single file to report on."}})
                    .addProperty(
                        "details",
                        QJsonObject{
                            {"type", "boolean"},
                            {"default", false},
                            {"description",
                             "Append each issue's detail lines - the offending source "
                             "and the notes under it, which build_get_compile_output "
                             "carries as well. Off by default; turn it on for a "
                             "diagnostic whose summary alone does not say enough."}})
                    .addProperty(
                        "max",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "How many lines to return (default 50, clamped to 1-1000). "
                             "The first error is usually the cause and the rest cascade."}})
                    .addProperty(
                        "offset",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "How many matching issues to skip, for reading past a "
                             "truncated reply."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "issues",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description",
                             "\"<file>:<line>: <severity>: <summary>\", with the file left "
                             "out when the issue has none."}})
                    .addProperty(
                        "base_dir",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "What the relative paths in issues are relative to. Present "
                             "only when at least one path is relative."}})
                    .addProperty("build_id", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "severity",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"error", "warning", "all"}},
                            {"description", "Which severity the issues array holds, with "
                                            "\"auto\" resolved."}})
                    .addProperty(
                        "error_count",
                        QJsonObject{{"type", "integer"}, {"minimum", 0}})
                    .addProperty(
                        "warning_count",
                        QJsonObject{{"type", "integer"}, {"minimum", 0}})
                    .addProperty(
                        "total",
                        QJsonObject{
                            {"type", "integer"},
                            {"minimum", 0},
                            {"description", "How many issues matched, before max cut the "
                                            "list down."}})
                    .addProperty(
                        "truncated",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Whether matches were left out; raise max or move offset on "
                             "to see them."}})
                    .addProperty(
                        "reason",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Present only when no issues could be reported: "
                             "\"still_building\", \"never_built\", \"unknown_build_id\" or "
                             "\"invalid_arguments\"."}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("issues")
                    .addRequired("error_count")
                    .addRequired("warning_count")
                    .addRequired("total")
                    .addRequired("truncated")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject body = buildIssuesReply(params.argumentsAsObject());
            return CallToolResult{}
                .structuredContent(body)
                .isError(body.value("reason").toString() == "invalid_arguments");
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("build_get_compile_output")
            .title("Get compile and deploy output")
            .description(
                "The raw Compile Output text of a build - the head and the tail of it, "
                "since the first error is the cause and a link or deploy failure has "
                "nothing before it. Prefer build_get_issues for the structured "
                "diagnostics; reach for this when a build or deployment failed without "
                "producing any."
                "\n\n"
                "Defaults to the most recent build. scope:\"session\" returns the pane's "
                "whole text instead, across builds and deployments alike.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "build_id",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Build to report on. Defaults to the most recent one."}})
                    .addProperty(
                        "scope",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"build", "session"}},
                            {"default", "build"},
                            {"description",
                             "\"build\" (default): one build's output. \"session\": "
                             "everything the pane has shown since Qt Creator started, "
                             "which ignores build_id."}})
                    .addProperty(
                        "max_chars",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "How much text to return (default 16384, clamped to "
                             "1000-204928). A marker naming the dropped character count "
                             "stands in for what was left out."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("output", QJsonObject{{"type", "string"}})
                    .addProperty("build_id", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "truncated",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Whether output left out any of the kept text; raising "
                             "max_chars returns the rest."}})
                    .addProperty(
                        "total_chars",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Characters captured. Only the last 204800 are kept, so a "
                             "larger number here means the rest is gone for good."}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("output")
                    .addRequired("truncated")
                    .addRequired("total_chars")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject body = compileOutputReply(params.argumentsAsObject());
            return CallToolResult{}
                .structuredContent(body)
                .isError(body.value("reason").toString() == "invalid_arguments");
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("build_cancel")
            .title("Cancel a running build")
            .description(
                "Stops the running build, as the Cancel Build button does. Defaults to "
                "the most recent build, which fails with reason:\"not_running\" if it has "
                "already ended.")
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "build_id",
                    QJsonObject{
                        {"type", "integer"},
                        {"description", "Build to stop. Defaults to the most recent one."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("canceled", QJsonObject{{"type", "boolean"}})
                    .addProperty("build_id", QJsonObject{{"type", "integer"}})
                    .addProperty(
                        "reason",
                        QJsonObject{
                            {"type", "string"},
                            {"enum",
                             QJsonArray{"ok", "not_running", "never_built", "unknown_build_id",
                                        "invalid_arguments"}}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addRequired("canceled")
                    .addRequired("reason")
                    .addRequired("message"))
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(false)),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            const QJsonObject body = cancelBuild(params.argumentsAsObject());
            return CallToolResult{}
                .structuredContent(body)
                .isError(!body.value("canceled").toBool());
        });

    // Shared output schema for run/debug tools.
    const auto runToolOutputSchema = [&]() {
        const Tool::OutputSchema issSchema = ProjectExplorer::IssuesManager::issuesSchema();
        QJsonObject issuesField{
            {"type", "object"},
            {"description",
             "Build issues — present when the build failed; same shape as the Issues "
             "pane's tasks"}};
        if (issSchema._properties) {
            QJsonObject props;
            for (auto it = issSchema._properties->cbegin(); it != issSchema._properties->cend();
                 ++it)
                props[it.key()] = it.value();
            issuesField["properties"] = props;
        }
        if (issSchema._required)
            issuesField["required"] = QJsonArray::fromStringList(*issSchema._required);

        return Tool::OutputSchema{}
            .addProperty(
                "output",
                QJsonObject{
                    {"type", "string"},
                    {"description",
                     "Tail of the output the run produced (present on success), truncated "
                     "to keep the reply small. The Application Output pane keeps the rest; "
                     "read_pane returns it."}})
            .addProperty(
                "exitCode",
                QJsonObject{
                    {"type", "integer"},
                    {"description",
                     "Process exit code, when the run produced one. Absent if it crashed or "
                     "terminated abnormally (e.g. a failing terminal launch)."}})
            .addProperty(
                "succeeded",
                QJsonObject{
                    {"type", "boolean"},
                    {"description", "True if the run finished with exit code 0."}})
            .addProperty("issues", issuesField);
    }();

    // Shared callback factory for run/debug tools.
    const auto makeRunCallback =
        [](Utils::Id defaultRunMode, const QString &finishedMessage) -> Mcp::Server::ToolInterfaceCallback {
        return [defaultRunMode, finishedMessage](
                   const Schema::CallToolRequestParams &params,
                   const ToolInterface &toolInterface) -> Utils::Result<> {
            // The run mode defaults to the tool's own (e.g. normal run), but can
            // be overridden per call, so one tool reaches any registered run mode
            // (analyzers, profilers, ...). Interactive modes have dedicated tools.
            const QString runModeArg = params.argumentsAsObject().value("run_mode").toString();
            const Utils::Id runMode = runModeArg.isEmpty() ? defaultRunMode
                                                           : Utils::Id::fromString(runModeArg);
            const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(runMode);
            if (!canRun) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(canRun.error())));
                return ResultOk;
            }

            struct State
            {
                BoundedOutput output;
                QString lastLine;
                bool finished = false;
                QJsonObject failureIssues;
                QPointer<RunControl> rc;
                std::optional<int> exitCode;
            };
            auto state = std::make_shared<State>();

            using namespace std::chrono_literals;

            const Utils::Result<ToolInterface::TaskProgressNotify> task = toolInterface.startTask(
                500ms,
                [state](Schema::Task t) {
                    if (state->finished)
                        Mcp::letTaskDieIn(t, 1min);
                    const bool failed = !state->failureIssues.isEmpty();
                    return t
                        .status(
                            !state->finished ? Schema::TaskStatus::working
                            : failed         ? Schema::TaskStatus::failed
                                             : Schema::TaskStatus::completed)
                        .statusMessage(
                            state->lastLine.isEmpty() ? std::nullopt
                                                      : std::optional{state->lastLine});
                },
                [state]() -> Utils::Result<CallToolResult> {
                    if (!state->failureIssues.isEmpty())
                        return CallToolResult{}.isError(true).structuredContent(
                            QJsonObject{{"issues", state->failureIssues}});
                    QJsonObject out{{"output", state->output.tail(maxReplyOutputSize)}};
                    if (state->exitCode)
                        out["exitCode"] = *state->exitCode;
                    out["succeeded"] = state->exitCode.has_value() && *state->exitCode == 0;
                    return CallToolResult{}.isError(false).structuredContent(out);
                },
                [state]() {
                    if (state->rc)
                        state->rc->initiateStop();
                },
                Mcp::progressToken(params));

            if (!task) {
                toolInterface.finish(
                    CallToolResult{}.isError(true).addContent(
                        Schema::TextContent{}.text(task.error())));
                return ResultOk;
            }

            const ToolInterface::TaskProgressNotify notify = *task;

            auto rcStartedConn = std::make_shared<QMetaObject::Connection>();

            *rcStartedConn = QObject::connect(
                ProjectExplorerPlugin::instance(),
                &ProjectExplorerPlugin::runControlStarted,
                ProjectExplorerPlugin::instance(),
                [state, notify, rcStartedConn, runMode](RunControl *rc) {
                    if (rc->runMode() != runMode)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    state->rc = rc;
                    QObject::connect(
                        rc,
                        &RunControl::appendMessage,
                        rc,
                        [state, notify](const QString &msg, Utils::OutputFormat) {
                            const QString trimmed = msg.trimmed();
                            if (trimmed.isEmpty())
                                return;
                            state->output.append(trimmed + '\n');
                            state->lastLine = trimmed;
                            if (notify)
                                notify(Schema::TaskStatus::working, trimmed, std::nullopt);
                        });
                });

            // Complete on stop even if the run never reached started(): a run
            // that fails during startup (e.g. a broken terminal whose stub never
            // connects) goes straight to stopped without emitting started(), so
            // runControlStarted never fires. runControlStoped fires on stop
            // regardless, so it is the reliable completion signal.
            auto rcStoppedConn = std::make_shared<QMetaObject::Connection>();
            *rcStoppedConn = QObject::connect(
                ProjectExplorerPlugin::instance(),
                &ProjectExplorerPlugin::runControlStoped,
                ProjectExplorerPlugin::instance(),
                [state, notify, rcStartedConn, rcStoppedConn, runMode, finishedMessage](
                    RunControl *rc) {
                    if (state->finished || rc->runMode() != runMode)
                        return;
                    if (state->rc && state->rc != rc)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    QObject::disconnect(*rcStoppedConn);
                    state->rc = rc;
                    state->finished = true;
                    state->exitCode = rc->lastExitCode();
                    if (notify)
                        notify(Schema::TaskStatus::completed, finishedMessage, std::nullopt);
                });

            QObject::connect(
                BuildManager::instance(),
                &BuildManager::buildQueueFinished,
                BuildManager::instance(),
                [state, notify, rcStartedConn](bool success) {
                    if (success || state->rc)
                        return;
                    QObject::disconnect(*rcStartedConn);
                    state->finished = true;
                    state->failureIssues = issuesManager().getCurrentIssues();
                    const int errorCount = state->failureIssues.value("summary")
                                               .toObject()
                                               .value("errorCount")
                                               .toInt();
                    const QString statusMsg
                        = errorCount > 0 ? QString("Build failed with %1 error(s)").arg(errorCount)
                                         : QString("Build failed");
                    if (notify)
                        notify(Schema::TaskStatus::failed, statusMsg, std::nullopt);
                },
                Qt::SingleShotConnection);

            ProjectExplorerPlugin::runStartupProject(runMode, false);
            return ResultOk;
        };
    };

    ToolRegistry::registerTool(
        Tool{}
            .name("run_project")
            .title("Run project")
            .description(
                "Runs the current startup project and waits for it to finish. "
                "Progress messages from the application are streamed during execution. "
                "On success, returns the full output plus the run outcome: exitCode (absent "
                "if the process crashed or the terminal launch failed) and succeeded (exit "
                "code 0). "
                "On build failure, returns isError=true with structured content in the same "
                "shape as the Issues pane's tasks (issues array + summary). "
                "By default this is a normal run; pass run_mode to run the project under a "
                "different, non-interactive run mode such as an analyzer (the run must finish "
                "on its own). Interactive modes have dedicated tools: use debugger_start for "
                "debugging and profiler_qml_start for the QML profiler. "
                "Returns an error if there is no startup project, no active build configuration, "
                "or the project cannot currently be run in the requested mode.")
            .inputSchema(
                Tool::InputSchema{}.addProperty(
                    "run_mode",
                    QJsonObject{
                        {"type", "string"},
                        {"description",
                         "Run-mode id to run the startup project under. Defaults to the normal "
                         "run mode (\"RunConfiguration.NormalRunMode\"). Examples: "
                         "\"PerfProfiler.RunMode\", \"RunConfiguration.QmlProfilerRunMode\". The "
                         "mode must have a run worker registered for the project's device and "
                         "run to completion; interactive modes belong to debugger_start / "
                         "profiler_qml_start."}}))
            .execution(ToolExecution().taskSupport(ToolExecution::TaskSupport::optional))
            .outputSchema(runToolOutputSchema),
        makeRunCallback(Utils::Id(Constants::NORMAL_RUN_MODE), "Run finished"));

    ToolRegistry::registerTool(
        Tool{}
            .name("run_list_modes")
            .title("Get run modes")
            .description(
                "Lists every run mode that has a registered run worker, and whether the current "
                "startup project can be run in each one right now (with the reason if not). Use a "
                "runnable id as run_project's run_mode; interactive modes have dedicated tools "
                "(debugger_start, profiler_qml_start).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("run_modes", QJsonObject{{"type", "array"}})
                    .addRequired("run_modes")),
        wrap([](const QJsonObject &) {
            QJsonArray modes;
            for (const Utils::Id &mode : RunWorkerFactory::allRunModes()) {
                const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(mode);
                QJsonObject entry{{"id", mode.toString()}, {"runnable", bool(canRun)}};
                if (!canRun)
                    entry.insert("reason", canRun.error());
                modes.append(entry);
            }
            return QJsonObject{{"run_modes", modes}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("project_show_panel")
            .title("Select a project settings panel")
            .description(
                "Switches to Projects mode and shows one of the active project's settings "
                "panels. Pass panel = \"build\", \"deploy\" or \"run\" for the target's "
                "Build/Deploy/Run Settings tabs, or a project-panel id (e.g. \"Editor\") for "
                "the left-hand project settings. Use this to reach settings only shown in "
                "these panels, such as the run configuration's \"Executable on device\" "
                "field. Returns an error when no project is open.")
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "panel",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "\"build\", \"deploy\" or \"run\" for the target settings tabs, "
                             "or a project-panel id for the left-hand project settings."}})
                    .addRequired("panel"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("panel", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &args) -> QJsonObject {
            const QString panel = args.value("panel").toString();
            if (!ProjectManager::startupProject())
                return {{"success", false}, {"reason", "no_project"}, {"message", "No project is open."}};
            const QString key = panel.toLower();
            if (key == "build")
                ProjectExplorerPlugin::activateBuildSettings();
            else if (key == "deploy")
                ProjectExplorerPlugin::activateDeploySettings();
            else if (key == "run")
                ProjectExplorerPlugin::activateRunSettings();
            else
                ProjectExplorerPlugin::activateProjectPanel(Utils::Id::fromString(panel));
            return {{"success", true}, {"panel", panel}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("search_projects")
            .title("Search for pattern in project files")
            .description(
                "Search for a text pattern in files matching a file pattern within a "
                "project (or all projects) and return all matches")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file_pattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "File pattern to filter which files to search (e.g., '*.cpp', "
                             "'*.h')"}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Optional: name of the project to search in (searches all projects if "
                             "not specified)"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "case_sensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addProperty(
                        "max_results",
                        QJsonObject{
                            {"type", "number"},
                            {"description",
                             "How many matches to return (default 200). Clamped to 1-5000. "
                             "total_matches reports how many there were."}})
                    .addRequired("file_pattern")
                    .addRequired("pattern"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "results",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"file", QJsonObject{{"type", "string"}}},
                                      {"line", QJsonObject{{"type", "number"}}},
                                      {"column", QJsonObject{{"type", "number"}}},
                                      {"text", QJsonObject{{"type", "string"}}}}}}}})
                    .addProperty(
                        "total_matches",
                        QJsonObject{
                            {"type", "number"},
                            {"description", "Matches found, which max_results may have cut "
                                            "the returned list down from."}})
                    .addProperty(
                        "truncated",
                        QJsonObject{{"type", "boolean"},
                                    {"description", "Whether results is shorter than "
                                                    "total_matches."}})
                    .addRequired("results")
                    .addRequired("total_matches")
                    .addRequired("truncated")),
        [](const Schema::CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject p = params.argumentsAsObject();
            const QString pattern = p.value("pattern").toString();
            if (pattern.isEmpty()) {
                // Declared required, but nothing enforces that: an empty pattern
                // matches every position of every file in every open project.
                return ResultError(QString("pattern must not be empty"));
            }
            const Utils::Result<int> requested
                = readCount(p, "max_results", defaultSearchResults);
            if (!requested)
                return ResultError(requested.error());
            const std::optional<QString> projectName
                = p.contains("project_name")
                      ? std::optional<QString>(p.value("project_name").toString())
                      : std::nullopt;
            searchInFiles(
                p.value("file_pattern").toString(),
                projectName,
                pattern,
                p.value("regex").toBool(false),
                p.value("case_sensitive").toBool(false),
                qBound(1, *requested, maxSearchResults),
                BuildManager::instance(),
                [toolInterface](const QJsonObject &result) {
                    toolInterface.finish(
                        CallToolResult{}.isError(false).structuredContent(result));
                });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("fs_replace_in_projects")
            .title("Replace pattern in project files")
            .description(
                "Replace all matches of a text pattern in files matching a file pattern "
                "within a project (or all projects) with replacement text")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file_pattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "File pattern to filter which files to modify (e.g., '*.cpp', "
                             "'*.h')"}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Optional: name of the project to search in (searches all projects if "
                             "not specified)"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{{"type", "string"}, {"description", "Text pattern to search for"}})
                    .addProperty(
                        "replacement",
                        QJsonObject{{"type", "string"}, {"description", "Replacement text"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regular expression"}})
                    .addProperty(
                        "case_sensitive",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the search should be case sensitive"}})
                    .addRequired("file_pattern")
                    .addRequired("pattern")
                    .addRequired("replacement"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("ok", QJsonObject{{"type", "boolean"}})
                    .addRequired("ok")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("file_pattern").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("case_sensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("project_name")
                                                           ? std::optional<QString>(
                                                                 p.value("project_name").toString())
                                                           : std::nullopt;
            replaceInFiles(
                filePattern,
                projectName,
                pattern,
                replacement,
                isRegex,
                caseSensitive,
                BuildManager::instance(),
                callback);
        }));

    // ===== Original PE tools =====


    generalMessagesBuffer(); // start capturing General Messages from now on
    ToolRegistry::registerTool(
        Tool{}
            .name("ui_read_general_messages")
            .title("Get General Messages output")
            .description("Returns the recent General Messages pane text - the warnings, errors "
                         "and status that plugins surface to the user outside the Compile Output "
                         "and Application Output panes. Use it to see diagnostics that are "
                         "otherwise only shown in the GUI.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("output", QJsonObject{{"type", "string"}})
                    .addRequired("output")),
        wrap([](const QJsonObject &) { return QJsonObject{{"output", generalMessagesBuffer()}}; }));

    ToolRegistry::registerTool(
        Tool{}
            .name("project_find_files")
            .title("Find files in project")
            .description("Find all files matching the pattern in a given project")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Name of the project to limit the search to (optional)"}})
                    .addProperty(
                        "pattern",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Pattern for finding the file, either a glob pattern or a regex"}})
                    .addProperty(
                        "regex",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "Whether the pattern is a regex (default is false)"}})
                    .addRequired("pattern"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "files",
                        QJsonObject{
                            {"type", "array"},
                            {"description", "List of file paths matching the pattern"},
                            {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("files")),
        [](const Schema::CallToolRequestParams &params) -> Utils::Result<Schema::CallToolResult> {
            const QJsonObject &p = params.argumentsAsObject();
            const QString projectName = p.value("project_name").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool();

            QRegularExpression re(
                isRegex ? pattern : QRegularExpression::wildcardToRegularExpression(pattern));
            if (!re.isValid()) {
                return CallToolResult{}.isError(true).structuredContent(
                    QJsonObject{{"error", "Invalid regex pattern"}});
            }

            const QList<Project *> projects = projectName.isEmpty() ? ProjectManager::projects()
                                                                    : projectsForName(projectName);
            const QStringList files = findFiles(projects, re);
            return CallToolResult{}
                .structuredContent(QJsonObject{{"files", QJsonArray::fromStringList(files)}})
                .isError(false);
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("project_list")
            .title("List all available projects")
            .description(
                "List all loaded projects. Each entry includes the project name, its file "
                "path, the active version control branch, and whether it is the current startup "
                "project (is_active). Use path or branch to disambiguate when multiple "
                "projects share the same display name (common in multi-worktree setups).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "projects",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"path", QJsonObject{{"type", "string"}}},
                                      {"branch", QJsonObject{{"type", "string"}}},
                                      {"type", QJsonObject{{"type", "string"}}},
                                      {"is_active", QJsonObject{{"type", "boolean"}}},
                                  }}}}})
                    .addRequired("projects")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"projects", listProjects()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("project_get_modules")
            .title("Get a project's targets and dependencies")
            .description(
                "Describes a project's structure: its runnable application targets (name, "
                "build key, executable, and defining project file) and the other loaded "
                "projects it depends on (from the session Dependencies settings). Selects the "
                "project by project_name or project_path, defaulting to the startup project. "
                "\"targets\" comes from the build system's application targets, so it lists "
                "executables and runnable utility targets only - libraries and other "
                "non-runnable targets are not reported - and it is empty until the project is "
                "configured with a kit and parsed.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the project. Defaults to the startup "
                                            "project."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Project file path, to disambiguate a shared name."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("name", QJsonObject{{"type", "string"}})
                    .addProperty("path", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "targets",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"build_key", QJsonObject{{"type", "string"}}},
                                      {"executable", QJsonObject{{"type", "string"}}},
                                      {"project_file", QJsonObject{{"type", "string"}}},
                                  }}}}})
                    .addProperty(
                        "depends_on",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "object"}}}})),
        wrap([](const QJsonObject &p) {
            const ProjectResolution r = resolveTargetProject(
                p.value("project_name").toString(), p.value("project_path").toString(), true);
            if (!r.project)
                return r.error;
            return projectModulesObject(r.project);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_list")
            .title("List all available kits")
            .description(
                "List all kits configured in Qt Creator. Each entry includes the kit name, "
                "its id, whether it is valid, whether it has warnings, whether it is the "
                "default kit, whether it was auto-detected (and SDK-provided), a "
                "filesystem-friendly name, the kit's run and build device, and an issues array "
                "with validation messages for invalid or warning kits.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "kits",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"id", QJsonObject{{"type", "string"}}},
                                      {"valid", QJsonObject{{"type", "boolean"}}},
                                      {"has_warning", QJsonObject{{"type", "boolean"}}},
                                      {"is_default", QJsonObject{{"type", "boolean"}}},
                                      {"auto_detected", QJsonObject{{"type", "boolean"}}},
                                      {"sdk_provided", QJsonObject{{"type", "boolean"}}},
                                      {"file_system_friendly_name",
                                       QJsonObject{{"type", "string"}}},
                                      {"run_device", QJsonObject{{"type", "string"}}},
                                      {"run_device_id", QJsonObject{{"type", "string"}}},
                                      {"build_device", QJsonObject{{"type", "string"}}},
                                      {"build_device_id", QJsonObject{{"type", "string"}}},
                                      {"issues",
                                       QJsonObject{
                                           {"type", "array"},
                                           {"items", QJsonObject{{"type", "string"}}}}},
                                  }}}}})
                    .addRequired("kits")),
        wrap([](const QJsonObject &) { return QJsonObject{{"kits", listKits()}}; }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_get_aspects")
            .title("List the configurable aspects of a kit")
            .description(
                "List the configurable aspects of a kit (debugger, toolchains, Qt version, "
                "device, ...). Each entry has the aspect id, its display name, a human-readable "
                "current value, and the raw stored value. Use the aspect id with "
                "kit_get_aspect_options and kit_set_value.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("kit_id",
                                 QJsonObject{{"type", "string"},
                                             {"description", "Kit id (as reported by kit_list)"}})
                    .addRequired("kit_id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("aspects", QJsonObject{{"type", "array"}})
                    .addRequired("aspects")),
        wrap([](const QJsonObject &p) {
            Kit *k = KitManager::kit(Utils::Id::fromString(p.value("kit_id").toString()));
            if (!k)
                return QJsonObject{{"error", "no such kit"}};
            QJsonArray arr;
            for (KitAspectFactory *f : KitManager::kitAspectFactories()) {
                QStringList disp;
                for (const KitAspectFactory::Item &item : f->toUserOutput(k))
                    disp << (item.second.isEmpty() ? item.first : item.second);
                arr.append(QJsonObject{{"id", f->id().toString()},
                                       {"name", f->displayName()},
                                       {"value", disp.join(", ")},
                                       {"raw", variantToJson(k->value(f->id()))}});
            }
            return QJsonObject{{"aspects", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_get_aspect_options")
            .title("List the valid values for a kit aspect")
            .description(
                "List the values a kit aspect can be set to (for item-backed aspects such as the "
                "debugger, toolchain, Qt version or device). Each option has a value (to pass to "
                "kit_set_value) and a display name. An empty list means the aspect is free-form.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("kit_id", QJsonObject{{"type", "string"}})
                    .addProperty("aspect_id",
                                 QJsonObject{{"type", "string"},
                                             {"description", "Aspect id (from kit_get_aspects)"}})
                    .addRequired("kit_id")
                    .addRequired("aspect_id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("options", QJsonObject{{"type", "array"}})
                    .addRequired("options")),
        wrap([](const QJsonObject &p) {
            Kit *k = KitManager::kit(Utils::Id::fromString(p.value("kit_id").toString()));
            if (!k)
                return QJsonObject{{"error", "no such kit"}};
            KitAspectFactory *f = kitAspectFactoryById(
                Utils::Id::fromString(p.value("aspect_id").toString()));
            if (!f)
                return QJsonObject{{"error", "no such aspect"}};
            QJsonArray arr;
            for (const KitAspectFactory::Candidate &c : f->candidateValues(k))
                arr.append(QJsonObject{{"value", variantToJson(c.value)}, {"display", c.displayName}});
            return QJsonObject{{"options", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_set_value")
            .title("Set the value of a kit aspect")
            .description(
                "Set a kit aspect to a value. Pass the value reported by kit_get_aspect_options "
                "for item-backed aspects (the exact stored type is preserved); free-form aspects "
                "take the value as-is. Aspects holding a list, such as the CMake configuration, "
                "take an array of strings.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("kit_id", QJsonObject{{"type", "string"}})
                    .addProperty("aspect_id", QJsonObject{{"type", "string"}})
                    .addProperty("value", QJsonObject{{"description", "Value to set"}})
                    .addRequired("kit_id")
                    .addRequired("aspect_id")
                    .addRequired("value"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            Kit *k = KitManager::kit(Utils::Id::fromString(p.value("kit_id").toString()));
            if (!k)
                return QJsonObject{{"success", false}, {"error", "no such kit"}};
            const Utils::Id aspectId = Utils::Id::fromString(p.value("aspect_id").toString());
            KitAspectFactory *f = kitAspectFactoryById(aspectId);
            if (!f)
                return QJsonObject{{"success", false}, {"error", "no such aspect"}};
            const QJsonValue wanted = p.value("value");
            // Prefer an enumerated candidate so the exact stored QVariant type
            // (e.g. a debugger item id) is preserved; otherwise store as-is.
            QVariant toStore = jsonToVariant(wanted);
            for (const KitAspectFactory::Candidate &c : f->candidateValues(k)) {
                if (variantToJson(c.value) == wanted) {
                    toStore = c.value;
                    break;
                }
            }
            k->setValue(aspectId, toStore);
            return QJsonObject{{"success", true}, {"raw", variantToJson(k->value(aspectId))}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_list_for_project")
            .title("List kits a project is configured for")
            .description(
                "List the kits a project is configured for (one per build target). "
                "Defaults to the active startup project when neither project_name nor "
                "project_path is given. Each kit entry has the same fields as kit_list "
                "plus is_active, which marks the kit of the project's active target. When "
                "multiple loaded projects share the same display name, pass project_path to "
                "disambiguate (returns reason:\"ambiguous_name\" with candidates otherwise).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project. Optional; defaults to the active "
                             "startup project."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addProperty("kits", QJsonObject{{"type", "array"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            return projectKits(
                p.value("project_name").toString(), p.value("project_path").toString());
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_add_to_project")
            .title("Add kits to a project")
            .description(
                "Adds a build target for each of the given kits to a project. Kits may be "
                "identified by kit id or display name (see kit_list). Defaults to the "
                "active startup project when neither project_name nor project_path is given. "
                "Returns a per-kit results array with status added/already_present/"
                "not_found/failed; the call does not abort on the first error. When "
                "multiple loaded projects share the same display name, pass project_path to "
                "disambiguate.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "kits",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description",
                             "Kit ids or display names to add to the project."}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project. Optional; defaults to the active "
                             "startup project."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name."}})
                    .addRequired("kits"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addProperty("results", QJsonObject{{"type", "array"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            QStringList kits;
            for (const QJsonValue &v : p.value("kits").toArray())
                kits.append(v.toString());
            return addKitsToProject(
                p.value("project_name").toString(), p.value("project_path").toString(), kits);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_rename")
            .title("Rename a kit")
            .description(
                "Gives a kit another display name, as editing it on the Kits preferences page "
                "does. Worth knowing why it matters: the default build directory of a project "
                "is derived from the kit name, so two kits that share one name also share one "
                "build directory and overwrite each other's configuration. Kits generated per "
                "Qt version collide that way when the versions carry the same version number "
                "and ABI. The kit may be given by id or display name (see kit_list); a name "
                "several kits share has to be told apart by id, which is the case this is for.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "kit",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Kit id or current display name."}})
                    .addProperty(
                        "name",
                        QJsonObject{{"type", "string"}, {"description", "The new display name."}})
                    .addRequired("kit")
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("previous_name", QJsonObject{{"type", "string"}})
                    .addProperty("kit", QJsonObject{{"type", "object"}})
                    .addProperty("shared_name", QJsonObject{{"type", "boolean"}})
                    .addProperty("shares_name_with", QJsonObject{{"type", "array"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            return renameKit(p.value("kit").toString(), p.value("name").toString());
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_set_active")
            .title("Set the active kit of a project")
            .description(
                "Makes the project build and run with one of the kits it is configured for, "
                "as choosing it in the kit selector does. The kit may be given by id or "
                "display name (see kit_list, and kit_list_for_project for which are configured "
                "and which is active); a display name that several kits share is refused, so "
                "use the id to tell them apart. Defaults to the active startup project when "
                "neither project_name nor project_path is given.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "kit",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Kit id or display name to make active."}})
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project. Optional; defaults to the active "
                             "startup project."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name."}})
                    .addRequired("kit"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addProperty("kit", QJsonObject{{"type", "object"}})
                    .addProperty("already_active", QJsonObject{{"type", "boolean"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            return setActiveKit(
                p.value("project_name").toString(),
                p.value("project_path").toString(),
                p.value("kit").toString());
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("kit_remove")
            .title("Remove kits")
            .description(
                "Removes kits from Qt Creator, identified by kit id or display name (see "
                "kit_list). Use it to clean up after device_detect_tools, which creates a kit "
                "per toolchain found on a device; kit_list reports each kit's run and build "
                "device, so the kits belonging to a device can be picked out. Returns a per-kit "
                "results array with status removed/not_found/sdk_provided/ambiguous_name; the "
                "call does not abort on the first error. SDK-provided kits cannot be removed. "
                "Removing a kit drops the corresponding build target from every project using "
                "it, so its build and run settings are lost.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "kits",
                        QJsonObject{
                            {"type", "array"},
                            {"items", QJsonObject{{"type", "string"}}},
                            {"description", "Kit ids or display names to remove."}})
                    .addRequired("kits"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("results", QJsonObject{{"type", "array"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            QStringList kits;
            for (const QJsonValue &v : p.value("kits").toArray())
                kits.append(v.toString());
            return removeKits(kits);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("build_list_configs")
            .title("List available build configurations")
            .description("List available build configurations")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "build_configs",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("build_configs")),
        wrap([](const QJsonObject &) {
            const QStringList configs = listBuildConfigs();
            QJsonArray arr;
            for (const QString &c : configs)
                arr.append(c);
            return QJsonObject{{"build_configs", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("build_switch_config")
            .title("Switch to a specific build configuration")
            .description("Switch to a specific build configuration")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the build configuration to switch to"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString name = p.value("name").toString();
            return QJsonObject{{"success", switchToBuildConfig(name)}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("build_add_config")
            .title("Add a build configuration")
            .description(
                "Creates a new build configuration for the active project's kit and, unless "
                "set_active is false, makes it active. build_type is matched against the build "
                "types the kit offers (e.g. \"Debug\", \"Release\", \"RelWithDebInfo\", "
                "\"MinSizeRel\"); on a mismatch the error lists the available types. Useful to "
                "run or build in a configuration the project does not have yet.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "build_type",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Build type to add, e.g. \"Release\" (matched case-insensitively "
                             "against the kit's available build types)."}})
                    .addProperty(
                        "set_active",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Make the new configuration active. Defaults to true."}})
                    .addRequired("build_type"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("name", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const QString buildType = p.value("build_type").toString();
            const bool setActive = p.value("set_active").toBool(true);
            return addBuildConfig(buildType, setActive);
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("project_get_current")
            .title("Get the currently active project")
            .description("Get the currently active project")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name of the currently active project"},
                            })
                    .addProperty(
                        "project_file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Path to the project's main file (e.g., .pro, .vcxproj, "
                             "CMakeLists.txt)"},
                            })
                    .addProperty(
                        "project_directory",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Path to the project's directory"},
                            })
                    .addRequired("project_directory")),
        wrap([](const QJsonObject &) { return getCurrentProject(); }));

    ToolRegistry::registerTool(
        Tool{}
            .name("project_set_active")
            .title("Set the active startup project")
            .description(
                "Changes the active startup project (the one Qt Creator builds, runs, and "
                "debugs by default). Accepts project_name, project_path, or both. When "
                "multiple loaded projects share the same display name (e.g. the same "
                "project open in two Git worktrees), you must also supply project_path to "
                "disambiguate; the tool returns reason:\"ambiguous_name\" with a "
                "candidates array if project_path is omitted and the name matches more "
                "than one project.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "project_name",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Display name of the project to activate. Required unless "
                             "project_path alone is sufficient to identify it."}})
                    .addProperty(
                        "project_path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to the project file. Unambiguously identifies "
                             "the project and takes precedence over project_name when "
                             "multiple projects share the same display name."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("active_project", QJsonObject{{"type", "object"}})
                    .addProperty("previous_active_project", QJsonObject{{"type", "object"}})
                    .addProperty("candidates", QJsonObject{{"type", "array"}})
                    .addRequired("success")
                    .addRequired("reason")
                    .addRequired("message")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const QString projectName = p.value("project_name").toString();
            const QString projectPath = p.value("project_path").toString();

            const ProjectResolution resolution
                = resolveTargetProject(projectName, projectPath, false);
            if (!resolution.project)
                return resolution.error;

            Project *previous = ProjectManager::startupProject();
            Project *target = resolution.project;

            if (previous == target) {
                return {
                    {"success", true},
                    {"reason", "ok_already_active"},
                    {"message",
                     QString("Project '%1' is already the active startup project.")
                         .arg(target->displayName())},
                    {"active_project", projectInfoObject(target)},
                    {"previous_active_project", projectInfoObject(previous)}};
            }

            ProjectManager::setStartupProject(target);

            return {
                {"success", true},
                {"reason", "ok"},
                {"message",
                 QString("Active startup project set to '%1'.").arg(target->displayName())},
                {"active_project", projectInfoObject(target)},
                {"previous_active_project",
                 previous ? QJsonValue(projectInfoObject(previous)) : QJsonValue()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("project_open")
            .title("Open a project")
            .description(
                "Opens a project in Qt Creator from a project file path (e.g., "
                "CMakeLists.txt, a .pro, .qbs, or .qmlproject file). If the project is "
                "already open, returns success with already_open=true. The opened project "
                "is added to the session; use project_set_active to make it the startup "
                "project.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"format", "uri"},
                            {"description",
                             "Absolute path to the project file to open (e.g., "
                             "CMakeLists.txt, .pro, .qbs, .qmlproject)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("already_open", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("project", QJsonObject{{"type", "object"}})
                    .addRequired("success")
                    .addRequired("message")),
        wrap([](const QJsonObject &p) {
            return openProjectFile(p.value("path").toString());
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("build_get_current_config")
            .title("Get the currently active build configuration")
            .description("Get the currently active build configuration")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("build_config", QJsonObject{{"type", "string"}})
                    .addRequired("build_config")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"build_config", getCurrentBuildConfig()}};
        }));

    ToolRegistry::registerTool(
        Tool()
            .name("project_list_repositories")
            .title("Get known version control repositories in all projects")
            .description(
                "List all known version control repositories (e.g., Git, Subversion) that are "
                "within the directories of all open projects")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema()
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the project to query repositories for"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema()
                    .addProperty(
                        "repositories",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "Map of version control system names to lists of repository paths"}})
                    .addRequired("repositories")),
        wrap([](const QJsonObject &p) {
            const QString projectName = p.value("name").toString();
            const QMap<QString, QSet<QString>> repos = knownRepositoriesInProject(projectName);
            QJsonObject reposJson;
            for (auto it = repos.constBegin(); it != repos.constEnd(); ++it)
                reposJson[it.key()] = QJsonArray::fromStringList(Utils::toList(it.value()));
            return QJsonObject{{"repositories", reposJson}};
        }));

    ToolRegistry::registerTool(
        Tool()
            .name("project_get_dependencies")
            .title("List project dependencies for all projects")
            .description("List project dependencies for all projects")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "name",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Name of the project to query dependencies for"}})
                    .addRequired("name"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "dependencies",
                        QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("dependencies")),
        wrap([](const QJsonObject &p) {
            const Utils::Result<QStringList> projects = projectDependencies(p["name"].toString());
            QJsonArray arr;
            for (const QString &pr : projects.value_or(QStringList{}))
                arr.append(pr);
            return QJsonObject{{"dependencies", arr}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("run_list_configs")
            .title("Get run configurations")
            .description(
                "Returns the project's existing run configurations. Each entry includes the "
                "display name, the run configuration type id, whether it is the active one, and "
                "the resolved runnable: executable, arguments, working directory, and the full "
                "run environment (key-value map, as the application would see it).")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "configurations",
                        QJsonObject{
                            {"type", "array"},
                            {"items",
                             QJsonObject{
                                 {"type", "object"},
                                 {"properties",
                                  QJsonObject{
                                      {"name", QJsonObject{{"type", "string"}}},
                                      {"id", QJsonObject{{"type", "string"}}},
                                      {"active", QJsonObject{{"type", "boolean"}}},
                                      {"executable", QJsonObject{{"type", "string"}}},
                                      {"arguments", QJsonObject{{"type", "string"}}},
                                      {"workingDirectory", QJsonObject{{"type", "string"}}},
                                      {"environment", QJsonObject{{"type", "object"}}}}},
                                 {"required", QJsonArray{"name", "id", "active"}}}},
                            {"description", "List of run configurations"}})
                    .addRequired("configurations")),
        wrap([](const QJsonObject &) {
            return QJsonObject{{"configurations", getRunConfigurations()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("run_configure")
            .title("Configure a run configuration")
            .description(
                "Selects an existing run configuration (by display name or type id, see "
                "run_list_configs) as the active one and/or sets its executable and the "
                "arguments the application is started with. Setting "
                "the executable only works for run configurations that have one, such as the "
                "bare-metal \"Custom Executable\" configuration. Then debugger_start (with no "
                "arguments) debugs it via its run configuration's own launch path. Several run "
                "configurations share one type id, so an id matching more than one fails with "
                "reason \"ambiguous\" and the matching display names in \"candidates\", rather "
                "than picking one of them.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Run configuration display name, or type id when it is unique."}})
                    .addProperty(
                        "executable",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Executable to set on the run configuration (local path)."}})
                    .addProperty(
                        "arguments",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Command-line arguments to start the application with, as one "
                             "string; an empty one clears them."}})
                    .addProperty(
                        "set_active",
                        QJsonObject{
                            {"type", "boolean"},
                            {"default", true},
                            {"description", "Make this the active run configuration."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("reason", QJsonObject{{"type", "string"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("id", QJsonObject{{"type", "string"}})
                    .addProperty("name", QJsonObject{{"type", "string"}})
                    .addProperty("active", QJsonObject{{"type", "boolean"}})
                    .addProperty("executable", QJsonObject{{"type", "string"}})
                    .addProperty("arguments", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "candidates",
                        QJsonObject{
                            {"type", "array"}, {"items", QJsonObject{{"type", "string"}}}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) {
            const bool setActive = p.contains("set_active") ? p.value("set_active").toBool() : true;
            const std::optional<QString> arguments = p.contains("arguments")
                ? std::make_optional(p.value("arguments").toString()) : std::nullopt;
            return configureRunConfig(
                p.value("id").toString(), p.value("executable").toString(), arguments, setActive);
        }));

    // --- Device management tools -------------------------------------------

    ToolRegistry::registerTool(
        Tool{}
            .name("device_list")
            .title("List configured devices")
            .description(
                "Lists all devices known to Qt Creator (ProjectExplorer::DeviceManager), with "
                "their id, type, display name, connection state, and SSH parameters.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(Tool::InputSchema{})
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("devices", QJsonObject{{"type", "array"}})
                    .addRequired("devices")),
        wrap([](const QJsonObject &) {
            QJsonArray devices;
            DeviceManager::forEachDevice([&devices](const IDeviceConstPtr &device) {
                devices.append(deviceToJson(device));
            });
            return QJsonObject{{"devices", devices}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("device_add")
            .title("Add a device")
            .description(
                "Creates a new device of the given device-type id (e.g. 'GenericLinuxOsType') and "
                "adds it to the DeviceManager, without going through the GUI wizard. 'params' sets "
                "the SSH parameters (host, port, userName, privateKeyFile, useKeyFile, timeout, "
                "hostKeyCheckingMode). Returns the new device id.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "type",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Device-type id, e.g. 'GenericLinuxOsType'. See the 'type' field of "
                             "device_list, or use device_list_types."}})
                    .addProperty(
                        "displayName",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Display name for the new device (optional)."}})
                    .addProperty(
                        "params",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "SSH parameters: host, port, userName, privateKeyFile, useKeyFile, "
                             "timeout, hostKeyCheckingMode (none|strict|allowNoMatch)."}})
                    .addRequired("type"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("id", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const Id typeId = Id::fromString(p.value("type").toString());
            IDeviceFactory *factory = IDeviceFactory::find(typeId);
            if (!factory)
                return {{"success", false},
                        {"error", QString("Unknown device type: " + p.value("type").toString())}};
            if (!factory->canCreate())
                return {{"success", false}, {"error", "Device type cannot be created programmatically."}};

            IDevice::Ptr device = factory->construct();
            if (!device)
                return {{"success", false}, {"error", "Failed to construct device."}};

            if (!device->id().isValid())
                device->setupId(IDevice::ManuallyAdded);

            const QString displayName = p.value("displayName").toString();
            if (!displayName.isEmpty())
                device->setDisplayName(displayName);

            applySshParameters(device, p.value("params").toObject());

            DeviceManager::addDevice(device);
            qCInfo(mcpDevices) << "Added device" << device->id().toString()
                               << "of type" << typeId.toString();
            return {{"success", true}, {"id", device->id().toString()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("device_set_parameters")
            .title("Update device parameters")
            .description(
                "Updates the display name and/or SSH parameters of an existing device. Only the "
                "fields present in 'params' are changed; others keep their current values.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id",
                        QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addProperty(
                        "displayName",
                        QJsonObject{{"type", "string"}, {"description", "New display name (optional)."}})
                    .addProperty(
                        "params",
                        QJsonObject{
                            {"type", "object"},
                            {"description",
                             "SSH parameters to change: host, port, userName, privateKeyFile, "
                             "useKeyFile, timeout, hostKeyCheckingMode."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(p.value("id").toString()));
            if (!device)
                return {{"success", false}, {"error", "No such device."}};

            const QString displayName = p.value("displayName").toString();
            if (!displayName.isEmpty())
                device->setDisplayName(displayName);

            if (p.contains("params"))
                applySshParameters(device, p.value("params").toObject());

            return {{"success", true}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("device_remove")
            .title("Remove a device")
            .description(
                "Removes the device with the given id from the DeviceManager. As in the Devices "
                "preferences page, an auto-detected device can only be removed while it is "
                "disconnected; the local desktop device can never be removed. Kits referring to "
                "the device are left behind, so remove those with kit_remove.")
            .annotations(ToolAnnotations{}.readOnlyHint(false).destructiveHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("id", QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &p) -> QJsonObject {
            const Id id = Id::fromString(p.value("id").toString());
            const IDevice::ConstPtr device = DeviceManager::find(id);
            if (!device)
                return {{"success", false}, {"error", "No such device."}};
            // Mirrors the Devices page, which only offers to remove an auto-detected device
            // once it is disconnected. This is what keeps the desktop device (auto-detected
            // and permanently ready) from being removed.
            if (device->isAutoDetected() && device->deviceState() != IDevice::DeviceDisconnected) {
                return {
                    {"success", false},
                    {"error",
                     QString("Device '%1' was auto-detected and is not disconnected, so it "
                             "cannot be removed.")
                         .arg(device->displayName())}};
            }
            DeviceManager::removeDevice(id);
            return {{"success", true}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("device_test")
            .title("Test a device connection")
            .description(
                "Runs the device's connection tester (IDevice::createDeviceTester()), collecting "
                "the streamed progress and error messages, and returns the final result. Blocks "
                "until the test finishes or the timeout elapses.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("id", QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addProperty(
                        "timeoutSeconds",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Abort the test after this many seconds (default 60)."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "result",
                        QJsonObject{
                            {"type", "string"},
                            {"enum", QJsonArray{"success", "failure", "timeout"}}})
                    .addProperty("messages", QJsonObject{{"type", "array"}})
                    .addRequired("result")
                    .addRequired("messages")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(p.value("id").toString()));
            if (!device) {
                callback(
                    {{"result", "failure"},
                     {"messages",
                      QJsonArray{QJsonObject{{"kind", "error"}, {"text", "No such device."}}}}});
                return;
            }
            if (!device->hasDeviceTester()) {
                callback(
                    {{"result", "failure"},
                     {"messages",
                      QJsonArray{QJsonObject{
                          {"kind", "error"},
                          {"text", "Device type does not support testing."}}}}});
                return;
            }

            DeviceTester *tester = device->createDeviceTester();
            if (!tester) {
                callback(
                    {{"result", "failure"},
                     {"messages",
                      QJsonArray{QJsonObject{
                          {"kind", "error"}, {"text", "Could not create device tester."}}}}});
                return;
            }

            tester->setParent(Utils::shutdownGuard());
            auto messages = std::make_shared<QJsonArray>();
            auto finished = std::make_shared<bool>(false);

            const auto finish = [callback, tester, messages, finished](const QString &result) {
                if (*finished)
                    return;
                *finished = true;
                callback({{"result", result}, {"messages", *messages}});
                tester->deleteLater();
            };

            QObject::connect(
                tester, &DeviceTester::progressMessage, tester, [messages](const QString &msg) {
                    messages->append(QJsonObject{{"kind", "progress"}, {"text", msg}});
                });
            QObject::connect(
                tester, &DeviceTester::errorMessage, tester, [messages](const QString &msg) {
                    messages->append(QJsonObject{{"kind", "error"}, {"text", msg}});
                });
            QObject::connect(
                tester, &DeviceTester::finished, tester, [finish](DeviceTester::TestResult result) {
                    finish(result == DeviceTester::TestSuccess ? "success" : "failure");
                });

            const int timeoutSeconds = p.value("timeoutSeconds").toInt(60);
            QTimer::singleShot(timeoutSeconds * 1000, tester, [finish]() { finish("timeout"); });

            tester->testDevice();
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("device_detect_tools")
            .title("Detect tools on a device and create kits")
            .description(
                "Connects to the device and runs the same auto-detection as the device "
                "configuration's \"Run Auto-Detection Now\" button: detects toolchains and "
                "debuggers on the device, detects on-device build tools (rsync, cmake, ...), "
                "and then creates kits for the device. Use this to set up a remote "
                "build/run/debug environment without the GUI. Returns the kits now "
                "associated with the device.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "id", QJsonObject{{"type", "string"}, {"description", "Device id."}})
                    .addRequired("id"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("kits", QJsonObject{{"type", "array"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrapAsync([](const QJsonObject &p, const Callback &callback) {
            IDevice::Ptr device = DeviceManager::find(Id::fromString(p.value("id").toString()));
            if (!device) {
                callback({{"success", false}, {"error", "No such device."}});
                return;
            }

            const auto reportKits = [device, callback] {
                // requestToolDetection() only creates kits when kit creation is enabled for
                // the device; create them explicitly so this tool always produces kits.
                // Skip if the device already has kits (from a prior run or the enabled
                // auto-creation) to avoid duplicates.
                const bool hasKits = Utils::anyOf(KitManager::kits(), [&](Kit *k) {
                    return BuildDeviceKitAspect::deviceId(k) == device->id();
                });
                if (!hasKits) {
                    KitManager::createKitsForBuildDevice(device);
                } else {
                    // Existing kits may predate the detection of some tools: a remote CMake
                    // tool, for example, only becomes detectable once the device is reachable,
                    // which is typically after the kits were first created. Re-complete the
                    // device's kits so newly detected tools get bound into aspects that are
                    // still unset (completeKit() runs setup() for those and fix() otherwise).
                    for (Kit *k : KitManager::kits()) {
                        if (BuildDeviceKitAspect::deviceId(k) == device->id())
                            KitManager::completeKit(k);
                    }
                }

                QJsonArray kits;
                for (Kit *k : KitManager::kits()) {
                    if (BuildDeviceKitAspect::deviceId(k) == device->id()
                        || RunDeviceKitAspect::deviceId(k) == device->id()) {
                        kits.append(QJsonObject{{"id", k->id().toString()},
                                                {"name", k->displayName()},
                                                {"valid", k->isValid()}});
                    }
                }
                callback({{"success", true}, {"kits", kits}});
            };

            const auto onConnected = [device, reportKits, callback](const Utils::Result<> &res) {
                if (!res) {
                    callback({{"success", false}, {"error", res.error()}});
                    return;
                }
                device->runAutoDetect({}, reportKits);
            };

            device->tryToConnect({Utils::shutdownGuard(), onConnected});
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("device_list_types")
            .title("List available device types")
            .description(
                "Lists the device-type ids that can be passed to device_add, with their display "
                "names and whether they can be created programmatically.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(Tool::InputSchema{})
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("types", QJsonObject{{"type", "array"}})
                    .addRequired("types")),
        wrap([](const QJsonObject &) {
            QJsonArray types;
            for (IDeviceFactory *factory : IDeviceFactory::allDeviceFactories()) {
                types.append(QJsonObject{
                    {"type", factory->deviceType().toString()},
                    {"displayName", factory->displayName()},
                    {"canCreate", factory->canCreate()},
                });
            }
            return QJsonObject{{"types", types}};
        }));
}

} // namespace ProjectExplorer::Internal
