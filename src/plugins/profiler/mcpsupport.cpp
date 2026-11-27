// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "perfprofilertool.h"
#include "perfprofilertracebackend.h"
#include "perfprofilertracemanager.h"
#include "profilertracedocument.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersettings.h"
#include "qmlprofilerstatemanager.h"
#include "qmlprofilertool.h"
#include "qmlprofilertracebackend.h"
#include "qmlprofilertracesummary.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>

#include <mcp/server/toolregistry.h>

#include <qmldebug/qmlevent.h>
#include <qmldebug/qmleventtype.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/result.h>
#include <utils/url.h>

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

#include <algorithm>
#include <iterator>
#include <limits>
#include <memory>

using namespace ProjectExplorer;
using namespace Utils;

namespace Profiler::Internal {

// The tools are deleted on shutdown while the MCP server may still answer.
static QJsonObject unavailableError(const QString &tool)
{
    return {{"success", false}, {"error", QString("The %1 is not available.").arg(tool)}};
}

static RunControl *runningRunControl(const Utils::Id &runMode)
{
    const QList<RunControl *> runControls = ProjectExplorerPlugin::allRunControls();
    for (RunControl *rc : runControls) {
        if (rc && rc->runMode() == runMode && rc->isRunning())
            return rc;
    }
    return nullptr;
}

// Machine-readable names of QmlDebug::ProfileFeature, in its order.
static const char *featureNames[] = {
    "javascript",
    "memory",
    "pixmap_cache",
    "scene_graph",
    "animations",
    "painting",
    "compiling",
    "creating",
    "binding",
    "handling_signal",
    "input_events",
    "debug_messages",
    "quick3d",
};
static_assert(std::size(featureNames) == QmlDebug::MaximumProfileFeature);

static QJsonArray featureArray(quint64 features)
{
    QJsonArray result;
    for (int feature = 0; feature < QmlDebug::MaximumProfileFeature; ++feature) {
        if (features & (1ULL << feature))
            result.append(QString::fromLatin1(featureNames[feature]));
    }
    return result;
}

static QJsonObject featuresSchema()
{
    QJsonArray names;
    for (const char *name : featureNames)
        names.append(QString::fromLatin1(name));
    return {{"type", "array"},
            {"items", QJsonObject{{"type", "string"}, {"enum", names}}},
            {"description",
             "The event categories to record. Omit to record all categories the trace "
             "supports. Fewer categories mean less data to transfer from the device."}};
}

static QJsonObject flushIntervalSchema()
{
    return {{"type", "integer"},
            {"minimum", 0},
            {"description",
             "How often, in milliseconds, the application sends its recorded data while "
             "recording, so that it does not accumulate in the application's memory. Use this "
             "for long recordings on devices with little memory. 0 turns periodic flushing "
             "off. Omit to use the run configuration's QML profiler setting."}};
}

static Utils::Result<QmlRecordingOptions> recordingOptions(const QJsonObject &args)
{
    QmlRecordingOptions options;
    if (args.contains("features")) {
        quint64 features = 0;
        const QJsonArray names = args.value("features").toArray();
        for (const QJsonValue &name : names) {
            int feature = 0;
            while (feature < QmlDebug::MaximumProfileFeature
                   && name.toString() != QLatin1String(featureNames[feature])) {
                ++feature;
            }
            if (feature == QmlDebug::MaximumProfileFeature)
                return Utils::ResultError(QString("Unknown feature \"%1\".").arg(name.toString()));
            features |= 1ULL << feature;
        }
        if (features == 0)
            return Utils::ResultError("No features to record given.");
        options.features = features;
    }
    if (args.contains("flush_interval_ms")) {
        const qint64 interval = args.value("flush_interval_ms").toInteger(-1);
        if (interval < 0 || interval > std::numeric_limits<int>::max())
            return Utils::ResultError("flush_interval_ms must be a non-negative integer.");
        options.flushInterval = std::chrono::milliseconds(interval);
    }
    return options;
}

static ProjectExplorer::Kit *findKit(const QString &name)
{
    if (name.isEmpty()) {
        if (RunConfiguration *runConfig = activeRunConfigForActiveProject())
            return runConfig->kit();
        return KitManager::defaultKit();
    }
    return KitManager::kit([&name](const Kit *kit) {
        return kit->displayName() == name || kit->id().toString() == name;
    });
}

// The live trace, for tools that read it. Reading it while a run still writes
// to it would see a trace in the middle of being recorded.
static Utils::Result<QmlProfilerTraceBackend *> idleLiveBackend()
{
    QmlProfilerTool *tool = QmlProfilerTool::instance();
    if (!tool)
        return Utils::ResultError("The QML profiler is not available.");
    QmlProfilerTraceBackend *backend = tool->liveBackend();
    if (!backend)
        return Utils::ResultError("There is no QML profiler trace. Record one first.");
    if (backend->stateManager()->currentState() != QmlProfilerStateManager::Idle)
        return Utils::ResultError("The QML profiler is still recording. Stop it first.");
    return backend;
}

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolRegistry;

    using SimplifiedCallback = std::function<QJsonObject(const QJsonObject &)>;
    static const auto wrap = [](const SimplifiedCallback &cb) {
        return [cb](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            return CallToolResult{}.structuredContent(cb(params.argumentsAsObject())).isError(false);
        };
    };

    const QJsonObject stringArray{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}};

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_qml_start")
            .title("Start the QML profiler")
            .description(
                "Starts the QML profiler on the current startup project (QML profiler run mode), "
                "using its active run configuration and kit. Does not build first - use the "
                "build_project tool beforehand if it may be out of date. Recording starts "
                "automatically. Poll profiler_qml_get_status for progress, and use "
                "profiler_qml_stop (or let the application exit) to finalize the trace. Returns "
                "as soon as the run is requested, unlike run_project, which waits for the run "
                "to finish. To profile an application that is started outside Qt Creator, use "
                "profiler_qml_attach instead.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(Tool::InputSchema{}
                             .addProperty("features", featuresSchema())
                             .addProperty("flush_interval_ms", flushIntervalSchema()))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &args) -> QJsonObject {
            QmlProfilerTool *tool = QmlProfilerTool::instance();
            if (!tool)
                return unavailableError("QML profiler");
            const Utils::Result<QmlRecordingOptions> options = recordingOptions(args);
            if (!options)
                return {{"success", false}, {"error", options.error()}};
            if (runningRunControl(Constants::QML_PROFILER_RUN_MODE))
                return {{"success", false}, {"error", "A QML profiler session is already running."}};
            const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(
                Constants::QML_PROFILER_RUN_MODE);
            if (!canRun)
                return {{"success", false}, {"error", canRun.error()}};
            tool->profileStartupProject(*options);
            return {{"success", true},
                    {"message", "QML profiler start requested for the startup project."}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_qml_attach")
            .title("Attach the QML profiler to a waiting application")
            .description(
                "Connects the QML profiler to an application that is already running and waits "
                "for a QML profiler connection, as \"Analyze > QML Profiling (Attach to Waiting "
                "Application)\" does. The application has to be started outside Qt Creator with "
                "QML debugging enabled, for example with "
                "-qmljsdebugger=port:<port>,block,services:CanvasFrameRate,EngineControl,"
                "DebugMessages. Recording starts once connected. Poll profiler_qml_get_status "
                "for progress, and use profiler_qml_stop (or let the application exit) to "
                "finalize the trace.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("port",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 1},
                                             {"maximum", 65535},
                                             {"description",
                                              "The port the application listens on for the "
                                              "QML debug connection."}})
                    .addProperty("host",
                                 QJsonObject{{"type", "string"},
                                             {"description",
                                              "The host the application runs on. Omit to use "
                                              "the host of the kit's run device."}})
                    .addProperty("kit",
                                 QJsonObject{{"type", "string"},
                                             {"description",
                                              "The display name or ID of the kit whose run "
                                              "device the application runs on. Omit to use the "
                                              "kit of the active run configuration, or the "
                                              "default kit."}})
                    .addProperty("features", featuresSchema())
                    .addProperty("flush_interval_ms", flushIntervalSchema())
                    .addRequired("port"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("server_url", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &args) -> QJsonObject {
            QmlProfilerTool *tool = QmlProfilerTool::instance();
            if (!tool)
                return unavailableError("QML profiler");
            const Utils::Result<QmlRecordingOptions> options = recordingOptions(args);
            if (!options)
                return {{"success", false}, {"error", options.error()}};
            const int port = args.value("port").toInt();
            if (port < 1 || port > std::numeric_limits<quint16>::max())
                return {{"success", false}, {"error", "port must be between 1 and 65535."}};
            const QString kitName = args.value("kit").toString();
            Kit *kit = findKit(kitName);
            if (!kit) {
                return {{"success", false},
                        {"error", kitName.isEmpty() ? QString("There is no kit to attach with.")
                                                    : QString("There is no kit \"%1\".")
                                                          .arg(kitName)}};
            }
            QString host = args.value("host").toString();
            if (host.isEmpty()) {
                const IDevice::ConstPtr device = RunDeviceKitAspect::device(kit);
                if (!device) {
                    return {{"success", false},
                            {"error", QString("The kit \"%1\" has no run device. Pass a host.")
                                          .arg(kit->displayName())}};
                }
                host = device->toolControlChannel(IDevice::QmlControlChannel).host();
            }
            if (runningRunControl(Constants::QML_PROFILER_RUN_MODE))
                return {{"success", false}, {"error", "A QML profiler session is already running."}};

            QUrl serverUrl;
            serverUrl.setScheme(Utils::urlTcpScheme());
            serverUrl.setHost(host);
            serverUrl.setPort(port);
            if (!serverUrl.isValid())
                return {{"success", false}, {"error", QString("Invalid host \"%1\".").arg(host)}};
            if (!tool->attachToApplication(kit, serverUrl, *options))
                return {{"success", false}, {"error", "Could not attach the QML profiler."}};
            return {{"success", true},
                    {"message", "QML profiler attach requested."},
                    {"server_url", serverUrl.toString()}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_qml_get_status")
            .title("Get QML profiler status")
            .description(
                "Returns the QML profiler state (Idle/AppRunning/AppStopRequested/AppDying, or "
                "Unavailable if the profiler is gone), whether the server is recording, whether a "
                "profiler run is active, and a summary of the data collected so far: the event "
                "count and the trace duration in nanoseconds. requested_features are the event "
                "categories asked of the application, recorded_features those it has sent "
                "events for. Use profiler_qml_get_statistics for the results.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("state", QJsonObject{{"type", "string"}})
                    .addProperty("recording", QJsonObject{{"type", "boolean"}})
                    .addProperty("running", QJsonObject{{"type", "boolean"}})
                    .addProperty("num_events", QJsonObject{{"type", "integer"}})
                    .addProperty("trace_duration_ns", QJsonObject{{"type", "integer"}})
                    .addProperty("requested_features", stringArray)
                    .addProperty("recorded_features", stringArray)
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("state")),
        wrap([](const QJsonObject &) -> QJsonObject {
            QmlProfilerTool *tool = QmlProfilerTool::instance();
            if (!tool) {
                return {{"state", "Unavailable"},
                        {"running", false},
                        {"error", "The QML profiler is not available."}};
            }
            // The trace of the running session, or of the last one to have run.
            QmlProfilerTraceBackend *backend = tool->liveBackend();
            if (!backend) {
                return {{"state", "Idle"},
                        {"recording", false},
                        {"running", false},
                        {"num_events", 0},
                        {"trace_duration_ns", 0}};
            }
            const QmlProfilerModelManager *modelManager = backend->modelManager();
            QmlProfilerStateManager *stateManager = backend->stateManager();
            return {
                {"state", stateManager->currentStateAsString()},
                {"recording", stateManager->serverRecording()},
                {"running", runningRunControl(Constants::QML_PROFILER_RUN_MODE) != nullptr},
                {"num_events", modelManager->numEvents()},
                {"trace_duration_ns", qint64(modelManager->traceDuration().count())},
                {"requested_features", featureArray(stateManager->requestedFeatures())},
                {"recorded_features", featureArray(stateManager->recordedFeatures())},
            };
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_qml_stop")
            .title("Stop the QML profiler")
            .description(
                "Stops the running QML profiler session by requesting its run control to stop, "
                "which finalizes the trace. Returns an error if no profiler session is running. "
                "Poll profiler_qml_get_status afterwards for the finalized event count.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) -> QJsonObject {
            RunControl *runControl = runningRunControl(Constants::QML_PROFILER_RUN_MODE);
            if (!runControl)
                return {{"success", false}, {"error", "No QML profiler session is running."}};
            runControl->initiateStop();
            return {{"success", true}, {"message", "QML profiler stop requested."}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_qml_save")
            .title("Save the QML profiler trace")
            .description(
                "Saves the trace of the last QML profiler session to a file, which can be loaded "
                "again with \"Analyze > QML Profiling Options > Load QML Trace\" or compared "
                "with later recordings. The file name decides the format: .qtd is the XML "
                "format, .qzt the compressed binary format. Fails while the profiler is still "
                "recording. Returns once the file is written.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(Tool::InputSchema{}
                             .addProperty("path",
                                          QJsonObject{{"type", "string"},
                                                      {"description",
                                                       "Absolute path of the file to write, "
                                                       "ending in .qtd or .qzt."}})
                             .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("path", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        [](const CallToolRequestParams &params,
           const Mcp::ToolInterface &toolInterface) -> Utils::Result<> {
            const auto finish = [toolInterface](const QJsonObject &result) {
                toolInterface.finish(
                    CallToolResult{}.structuredContent(result).isError(false));
            };
            const auto fail = [finish](const QString &message) {
                finish({{"success", false}, {"error", message}});
            };

            const FilePath path = FilePath::fromUserInput(
                params.argumentsAsObject().value("path").toString());
            if (path.isEmpty() || !path.isAbsolutePath()) {
                fail("path must be an absolute file path.");
                return Utils::ResultOk;
            }
            if (path.suffix() != "qtd" && path.suffix() != "qzt") {
                fail("path must end in .qtd or .qzt.");
                return Utils::ResultOk;
            }
            const Utils::Result<QmlProfilerTraceBackend *> backend = idleLiveBackend();
            if (!backend) {
                fail(backend.error());
                return Utils::ResultOk;
            }
            ProfilerTraceDocument *document = QmlProfilerTool::instance()->liveDocument();
            QTC_ASSERT(document, fail("There is no QML profiler trace."); return Utils::ResultOk);

            // Writing runs in the background: the answer comes with saved(), which
            // also follows a failed write. The guard dies with the trace, if the
            // trace is closed before that.
            struct SaveState
            {
                QString error;
                bool finished = false;
            };
            const auto state = std::make_shared<SaveState>();
            auto guard = new QObject(*backend);
            const auto finishOnce = [state, finish, path](const QString &closedError) {
                if (std::exchange(state->finished, true))
                    return;
                const QString error = state->error.isEmpty() ? closedError : state->error;
                if (error.isEmpty())
                    finish({{"success", true}, {"path", path.toUserOutput()}});
                else
                    finish({{"success", false}, {"error", error}});
            };
            QObject::connect(*backend, &ProfilerTraceBackend::error, guard,
                             [state](const QString &message) {
                if (state->error.isEmpty())
                    state->error = message;
            });
            // A canceled write removes the file without reporting an error.
            QObject::connect(*backend, &QmlProfilerTraceBackend::saved, guard,
                             [finishOnce, guard, path] {
                finishOnce(path.exists() ? QString() : QString("The trace was not saved."));
                guard->deleteLater();
            });
            QObject::connect(guard, &QObject::destroyed, [finishOnce] {
                finishOnce("The trace was closed before it was saved.");
            });

            const Utils::Result<> started = document->save(path);
            if (!started) {
                state->error = started.error();
                finishOnce({});
                delete guard;
            }
            return Utils::ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_qml_get_statistics")
            .title("Get QML profiler statistics")
            .description(
                "Summarizes the trace of the last QML profiler session, optionally restricted to "
                "a time range: the most expensive QML/JavaScript events as the Statistics view "
                "shows them (bindings, signal handlers, JavaScript functions, object creation "
                "and compilation, with call count, total, self, median and maximum time and "
                "source location), the frame time distribution against a target frame rate, "
                "and the time spent in each scene graph and Quick3D stage. Times are in "
                "nanoseconds unless the name says otherwise, and timestamps are relative to the "
                "start of the trace. Fails while the profiler is still recording.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("start_ns",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 0},
                                             {"description",
                                              "Start of the range to summarize, relative to "
                                              "the start of the trace. Omit for the start of "
                                              "the trace."}})
                    .addProperty("end_ns",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 0},
                                             {"description",
                                              "End of the range to summarize, relative to the "
                                              "start of the trace. Omit for the end of the "
                                              "trace."}})
                    .addProperty("limit",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 0},
                                             {"maximum", 1000},
                                             {"description",
                                              "How many events to list. Defaults to 20."}})
                    .addProperty("sort_by",
                                 QJsonObject{{"type", "string"},
                                             {"enum",
                                              QJsonArray{"total", "self", "calls", "max",
                                                         "median"}},
                                             {"description",
                                              "What to rank the events by. Defaults to "
                                              "\"total\"."}})
                    .addProperty("target_fps",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 1},
                                             {"maximum", kMaxTargetFrameRate},
                                             {"description",
                                              "The frame rate the application is meant to "
                                              "reach, which the frames are rated against. "
                                              "Defaults to the QML profiler setting."}}))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("range", QJsonObject{{"type", "object"}})
                    .addProperty("num_event_types", QJsonObject{{"type", "integer"}})
                    .addProperty("events", QJsonObject{{"type", "array"}})
                    .addProperty("frames", QJsonObject{{"type", "object"}})
                    .addProperty("scene_graph", QJsonObject{{"type", "array"}})
                    .addProperty("quick3d", QJsonObject{{"type", "array"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})),
        [](const CallToolRequestParams &params,
           const Mcp::ToolInterface &toolInterface) -> Utils::Result<> {
            const auto finish = [toolInterface](const QJsonObject &result) {
                toolInterface.finish(
                    CallToolResult{}.structuredContent(result).isError(false));
            };
            const QJsonObject args = params.argumentsAsObject();
            const Utils::Result<QmlProfilerTraceBackend *> backend = idleLiveBackend();
            if (!backend) {
                finish({{"error", backend.error()}});
                return Utils::ResultOk;
            }

            QmlTraceSummaryOptions options;
            if (args.contains("start_ns"))
                options.start = std::chrono::nanoseconds(args.value("start_ns").toInteger());
            if (args.contains("end_ns"))
                options.end = std::chrono::nanoseconds(args.value("end_ns").toInteger());
            if (options.start && options.end && *options.end < *options.start) {
                finish({{"error", "end_ns must not be before start_ns."}});
                return Utils::ResultOk;
            }
            options.limit = std::clamp(args.value("limit").toInt(20), 0, 1000);
            static const QHash<QString, QmlTraceSummaryOptions::SortBy> sortKeys{
                {"total", QmlTraceSummaryOptions::TotalTime},
                {"self", QmlTraceSummaryOptions::SelfTime},
                {"calls", QmlTraceSummaryOptions::Calls},
                {"max", QmlTraceSummaryOptions::MaxTime},
                {"median", QmlTraceSummaryOptions::MedianTime},
            };
            options.sortBy = sortKeys.value(args.value("sort_by").toString("total"));
            options.targetFrameRate = args.value("target_fps").toInt(
                globalSettings().targetFrameRate());

            // The events are replayed in another thread, as the views do. Clearing,
            // loading or restricting the trace cancels the replay without calling
            // back; restrictionFinished() then still marks the end of it. The guard
            // dies with the trace.
            QmlProfilerModelManager *manager = (*backend)->modelManager();
            const auto summary = std::make_shared<QmlTraceSummary>(*manager, options);
            const auto finished = std::make_shared<bool>(false);
            const auto finishOnce = [finish, finished](const QJsonObject &result) {
                if (!std::exchange(*finished, true))
                    finish(result);
            };
            auto guard = new QObject(manager);
            QObject::connect(guard, &QObject::destroyed, [finishOnce] {
                finishOnce({{"error", "The trace was closed while it was summarized."}});
            });
            manager->replayQmlEventsInThread(
                summary.get(),
                [summary]() -> QmlProfilerModelManager::QmlEventLoader {
                    // The loader writes to the summary from the replay thread.
                    return [summary, load = summary->loader()](const QmlDebug::QmlEvent &event,
                                                               const QmlDebug::QmlEventType &type) {
                        load(event, type);
                    };
                },
                {},
                {},
                [summary, finishOnce, guard] {
                    finishOnce(summary->result());
                    guard->deleteLater();
                },
                [finishOnce, guard](const QString &message) {
                    finishOnce({{"error", message}});
                    guard->deleteLater();
                });
            QObject::connect(manager, &QmlProfilerModelManager::restrictionFinished, guard,
                             [finishOnce, guard] {
                finishOnce({{"error", "The trace changed while it was summarized."}});
                guard->deleteLater();
            });
            return Utils::ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_perf_start")
            .title("Start the CPU (perf) profiler")
            .description(
                "Starts the CPU (perf) profiler on the current startup project (perf profiler run "
                "mode), using its active run configuration and kit. Does not build first - use the "
                "build_project tool beforehand if it may be out of date. Recording starts "
                "automatically. Poll profiler_perf_get_status, and use profiler_perf_stop (or let "
                "the application exit) to finalize the trace. Returns as soon as the run is "
                "requested, unlike run_project with run_mode \"PerfProfiler.RunMode\", which "
                "waits for the run to finish and gives no access to the trace.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) -> QJsonObject {
            PerfProfilerTool *tool = PerfProfilerTool::instance();
            if (!tool)
                return unavailableError("perf profiler");
            const Utils::Result<> canRun = ProjectExplorerPlugin::canRunStartupProject(
                Constants::PERFPROFILER_RUN_MODE);
            if (!canRun)
                return {{"success", false}, {"error", canRun.error()}};
            tool->profileStartupProject();
            return {{"success", true},
                    {"message", "Perf profiler start requested for the startup project."}};
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_perf_get_status")
            .title("Get CPU (perf) profiler status")
            .description(
                "Returns whether the perf profiler is recording, whether a perf run is active, and "
                "a summary of the data collected so far: the sample/event count and the trace "
                "duration in nanoseconds.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("recording", QJsonObject{{"type", "boolean"}})
                    .addProperty("running", QJsonObject{{"type", "boolean"}})
                    .addProperty("num_events", QJsonObject{{"type", "integer"}})
                    .addProperty("trace_duration_ns", QJsonObject{{"type", "integer"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("recording")),
        wrap([](const QJsonObject &) -> QJsonObject {
            // The trace of the running session, or of the last one to have run.
            PerfProfilerTool *tool = PerfProfilerTool::instance();
            if (!tool) {
                return {{"recording", false},
                        {"running", false},
                        {"error", "The perf profiler is not available."}};
            }
            PerfProfilerTraceBackend *backend = tool->liveBackend();
            if (!backend) {
                return {{"recording", false},
                        {"running", false},
                        {"num_events", 0},
                        {"trace_duration_ns", 0}};
            }
            const PerfProfilerTraceManager *traceManager = backend->traceManager();
            return {
                {"recording", tool->isRecording()},
                {"running", runningRunControl(Constants::PERFPROFILER_RUN_MODE) != nullptr},
                {"num_events", traceManager->numEvents()},
                {"trace_duration_ns", qint64(traceManager->traceDuration().count())},
            };
        }));

    ToolRegistry::registerTool(
        Tool{}
            .name("profiler_perf_stop")
            .title("Stop the CPU (perf) profiler")
            .description(
                "Stops the running perf profiler session by requesting its run control to stop, "
                "which finalizes the trace. Returns an error if no perf session is running. Poll "
                "profiler_perf_get_status afterwards for the finalized sample count.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("success", QJsonObject{{"type", "boolean"}})
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .addProperty("error", QJsonObject{{"type", "string"}})
                    .addRequired("success")),
        wrap([](const QJsonObject &) -> QJsonObject {
            RunControl *runControl = runningRunControl(Constants::PERFPROFILER_RUN_MODE);
            if (!runControl)
                return {{"success", false}, {"error", "No perf profiler session is running."}};
            runControl->initiateStop();
            return {{"success", true}, {"message", "Perf profiler stop requested."}};
        }));
}

} // namespace Profiler::Internal
