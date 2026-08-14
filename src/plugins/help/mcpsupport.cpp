// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "helpmanager.h"
#include "localhelpmanager.h"

#include <coreplugin/helpmanager.h>

#include <mcp/server/toolregistry.h>

#include <utils/filepath.h>
#include <utils/result.h>

#include <QHelpContentItem>
#include <QHelpContentWidget>
#include <QHelpEngine>
#include <QHelpFilterData>
#include <QHelpFilterEngine>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QTimer>
#include <QVersionNumber>

#include <algorithm>

using namespace Utils;

namespace Help::Internal {

using namespace Mcp::Schema;
using Mcp::ToolInterface;
using Mcp::ToolRegistry;

constexpr int defaultNodeLimit = 2000;
constexpr int maxNodeLimit = 20000;
constexpr int maxDepthLimit = 10;

static CallToolResult toolError(const QString &message)
{
    return CallToolResult{}.isError(true).addContent(TextContent{}.text(message));
}

static QJsonObject stringArraySchema()
{
    return QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}};
}

// Help mode loads the collection file lazily, and the filter engine knows the
// user's filters only afterwards. Without this a filter call made before Help
// was ever opened operates on an empty collection and just reports failure.
static QHelpFilterEngine *setupFilterEngine()
{
    LocalHelpManager::setupGuiHelpEngine();
    return LocalHelpManager::filterEngine();
}

// The name of the filter set_help_version_filter created, so that walking
// several versions leaves at most one "Version x.y.z" entry behind rather than
// one per version visited.
static QString &createdVersionFilter()
{
    static QString name;
    return name;
}

// Walks the content model down to maxDepth levels, collecting node titles. The
// whole walk goes into a single structuredContent and the Contents tree of a
// few registered Qt versions runs to thousands of nodes, so the budget is what
// keeps a caller from asking for an answer it cannot use.
static QJsonArray contentsAt(QHelpContentModel *model, const QModelIndex &parent,
                             int maxDepth, int *budget, bool *truncated)
{
    QJsonArray nodes;
    const int rows = model->rowCount(parent);
    for (int row = 0; row < rows; ++row) {
        if (*budget <= 0) {
            *truncated = true;
            break;
        }
        --*budget;
        const QModelIndex index = model->index(row, 0, parent);
        QHelpContentItem *item = model->contentItemAt(index);
        QJsonObject node;
        node["title"] = item ? item->title() : QString();
        if (maxDepth > 1 && model->rowCount(index) > 0)
            node["children"] = contentsAt(model, index, maxDepth - 1, budget, truncated);
        nodes.append(node);
    }
    return nodes;
}

void registerMcpTools()
{
    ToolRegistry::registerTool(
        Tool{}
            .name("get_registered_documentation")
            .title("List registered help documentation")
            .description("Returns the namespaces of all documentation registered in the Help "
                         "system. Several registered Qt versions produce several namespaces that "
                         "differ only by their version suffix (for example "
                         "\"org.qt-project.qtcore.680\").")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(Tool::OutputSchema{}
                              .addProperty("namespaces", stringArraySchema())
                              .addProperty("count", QJsonObject{{"type", "integer"}})
                              .addRequired("namespaces")),
        [](const CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            const QStringList namespaces = HelpManager::registeredNamespaces();
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"namespaces", QJsonArray::fromStringList(namespaces)},
                {"count", int(namespaces.size())}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("register_documentation")
            .title("Register help documentation")
            .description("Registers the given .qch documentation files with the Help system and "
                         "waits for the registration to finish. Returns the resulting namespaces. "
                         "Registering several versions of the same component is how duplicate "
                         "top-level nodes appear in the Contents tree. \"timed_out\" tells a "
                         "caller that the wait was given up on, so the namespaces are whatever "
                         "was registered at that point rather than the finished result.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("paths",
                                 QJsonObject{{"type", "array"},
                                             {"items", QJsonObject{{"type", "string"}}},
                                             {"description",
                                              "Absolute paths to .qch documentation files."}})
                    .addRequired("paths"))
            .outputSchema(Tool::OutputSchema{}
                              .addProperty("namespaces", stringArraySchema())
                              .addProperty("timed_out", QJsonObject{{"type", "boolean"}})
                              .addRequired("namespaces")),
        [](const CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            FilePaths files;
            const QJsonArray paths = params.argumentsAsObject().value("paths").toArray();
            for (const QJsonValue &path : paths)
                files.append(FilePath::fromUserInput(path.toString()));
            if (files.isEmpty()) {
                toolInterface.finish(toolError("No documentation files given."));
                return ResultOk;
            }

            const auto connection = std::make_shared<QMetaObject::Connection>();
            auto finishOnce = std::make_shared<bool>(false);
            const auto reportResult = [toolInterface, connection, finishOnce](bool timedOut) {
                if (*finishOnce)
                    return;
                *finishOnce = true;
                QObject::disconnect(*connection);
                const QStringList namespaces = HelpManager::registeredNamespaces();
                toolInterface.finish(CallToolResult{}.isError(false).structuredContent(QJsonObject{
                    {"namespaces", QJsonArray::fromStringList(namespaces)},
                    {"timed_out", timedOut}}));
            };
            *connection = QObject::connect(Core::HelpManager::Signals::instance(),
                                           &Core::HelpManager::Signals::documentationChanged,
                                           Core::HelpManager::Signals::instance(),
                                           [reportResult] { reportResult(false); });
            // Registration is asynchronous; guard against a missing signal.
            QTimer::singleShot(20000, Core::HelpManager::Signals::instance(),
                               [reportResult] { reportResult(true); });
            HelpManager::registerUserDocumentation(files);
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("list_help_filters")
            .title("List help documentation filters")
            .description("Returns the documentation filters known to the Help system, the "
                         "available component versions, and the currently active filter. "
                         "Selecting a filter narrows the Contents tree to matching documentation.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(Tool::OutputSchema{}
                              .addProperty("filters", stringArraySchema())
                              .addProperty("versions", stringArraySchema())
                              .addProperty("active", QJsonObject{{"type", "string"}})
                              .addRequired("filters")),
        [](const CallToolRequestParams &) -> Utils::Result<CallToolResult> {
            QHelpFilterEngine *engine = setupFilterEngine();
            if (!engine)
                return toolError("The help filter engine is not available.");
            QJsonArray versions;
            const QList<QVersionNumber> versionList = engine->availableVersions();
            for (const QVersionNumber &version : versionList)
                versions.append(version.toString());
            return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                {"filters", QJsonArray::fromStringList(engine->filters())},
                {"versions", versions},
                {"active", engine->activeFilter()}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("set_help_filter")
            .title("Set the active help filter")
            .description("Sets the active documentation filter by name. Pass an empty name to "
                         "clear the filter (show all documentation).")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(Tool::InputSchema{}.addProperty(
                "filter",
                QJsonObject{{"type", "string"},
                            {"description", "Filter name, or empty for no filter."}}))
            .outputSchema(Tool::OutputSchema{}
                              .addProperty("active", QJsonObject{{"type", "string"}})
                              .addProperty("ok", QJsonObject{{"type", "boolean"}})
                              .addRequired("ok")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            QHelpFilterEngine *engine = setupFilterEngine();
            if (!engine)
                return toolError("The help filter engine is not available.");
            const QString filter = params.argumentsAsObject().value("filter").toString();
            const bool ok = engine->setActiveFilter(filter);
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"active", engine->activeFilter()}, {"ok", ok}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("set_help_version_filter")
            .title("Filter help documentation by version")
            .description("Creates (if needed) and activates a documentation filter that shows only "
                         "the given component version, for example \"6.12.0\". Pass an empty "
                         "version to clear the filter. Use this to collapse duplicate top-level "
                         "nodes that come from several registered versions. The filter this tool "
                         "created is removed again when the filter is cleared or another version "
                         "replaces it, so the user's Help configuration is left as it was.")
            .annotations(ToolAnnotations{}.readOnlyHint(false))
            .inputSchema(Tool::InputSchema{}.addProperty(
                "version",
                QJsonObject{{"type", "string"},
                            {"description", "Version like \"6.12.0\", or empty to clear."}}))
            .outputSchema(Tool::OutputSchema{}
                              .addProperty("active", QJsonObject{{"type", "string"}})
                              .addProperty("ok", QJsonObject{{"type", "boolean"}})
                              .addRequired("ok")),
        [](const CallToolRequestParams &params) -> Utils::Result<CallToolResult> {
            QHelpFilterEngine *engine = setupFilterEngine();
            if (!engine)
                return toolError("The help filter engine is not available.");
            const QString version = params.argumentsAsObject().value("version").toString();
            QString &created = createdVersionFilter();
            bool ok = true;
            if (version.isEmpty()) {
                ok = engine->setActiveFilter(QString());
                if (!created.isEmpty()) {
                    engine->removeFilter(created);
                    created.clear();
                }
            } else {
                const QString name = QString("Version %1").arg(version);
                QHelpFilterData data;
                data.setVersions({QVersionNumber::fromString(version)});
                ok = engine->setFilterData(name, data) && engine->setActiveFilter(name);
                if (ok) {
                    if (!created.isEmpty() && created != name)
                        engine->removeFilter(created);
                    created = name;
                }
            }
            return CallToolResult{}.isError(false).structuredContent(
                QJsonObject{{"active", engine->activeFilter()}, {"ok", ok}});
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("get_help_contents")
            .title("Get the help Contents tree")
            .description("Returns the top-level nodes of the Help Contents tree (as shown in the "
                         "Help mode sidebar), built for the active filter. Reports "
                         "\"duplicate_titles\" - titles that appear on more than one top-level "
                         "node, which happens when several versions of a component are registered "
                         "without a narrowing filter. \"truncated\" says nodes were left out "
                         "because \"max_nodes\" ran out, \"timed_out\" that the wait for the tree "
                         "to be built was given up on.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("max_depth",
                                 QJsonObject{{"type", "integer"},
                                             {"description",
                                              "Tree depth to return (default 1, top level only, "
                                              "at most 10)."}})
                    .addProperty("max_nodes",
                                 QJsonObject{{"type", "integer"},
                                             {"description",
                                              "Maximum number of nodes to return (default 2000, "
                                              "at most 20000)."}}))
            .outputSchema(Tool::OutputSchema{}
                              .addProperty("contents", QJsonObject{{"type", "array"}})
                              .addProperty("top_level_count", QJsonObject{{"type", "integer"}})
                              .addProperty("duplicate_titles", stringArraySchema())
                              .addProperty("truncated", QJsonObject{{"type", "boolean"}})
                              .addProperty("timed_out", QJsonObject{{"type", "boolean"}})
                              .addRequired("contents")),
        [](const CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const int maxDepth = std::clamp(args.value("max_depth").toInt(1), 1, maxDepthLimit);
            const int requested = args.value("max_nodes").toInt();
            const int maxNodes
                = std::clamp(requested > 0 ? requested : defaultNodeLimit, 1, maxNodeLimit);

            LocalHelpManager::setupGuiHelpEngine();
            QHelpContentModel *model = LocalHelpManager::helpEngine().contentModel();
            if (!model) {
                toolInterface.finish(toolError("The help content model is not available."));
                return ResultOk;
            }

            const auto connection = std::make_shared<QMetaObject::Connection>();
            auto finishOnce = std::make_shared<bool>(false);
            const auto report = [toolInterface, connection, finishOnce, model, maxDepth, maxNodes](
                                    bool timedOut) {
                if (*finishOnce)
                    return;
                *finishOnce = true;
                QObject::disconnect(*connection);

                int budget = maxNodes;
                bool truncated = false;
                const QJsonArray contents
                    = contentsAt(model, QModelIndex(), maxDepth, &budget, &truncated);
                QHash<QString, int> titleCounts;
                for (const QJsonValue &node : contents)
                    ++titleCounts[node.toObject().value("title").toString()];
                QStringList duplicates;
                for (auto it = titleCounts.cbegin(); it != titleCounts.cend(); ++it) {
                    if (it.value() > 1)
                        duplicates.append(it.key());
                }
                // A stable order, so that a caller can compare two answers.
                std::sort(duplicates.begin(), duplicates.end());
                toolInterface.finish(CallToolResult{}.isError(false).structuredContent(QJsonObject{
                    {"contents", contents},
                    {"top_level_count", int(model->rowCount(QModelIndex()))},
                    {"duplicate_titles", QJsonArray::fromStringList(duplicates)},
                    {"truncated", truncated},
                    {"timed_out", timedOut}}));
            };
            *connection = QObject::connect(model, &QHelpContentModel::contentsCreated,
                                           model, [report] { report(false); });
            QTimer::singleShot(20000, model, [report] { report(true); });
            model->createContentsForCurrentFilter();
            return ResultOk;
        });
}

} // namespace Help::Internal
