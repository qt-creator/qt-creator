// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpsupport.h"

#include "gitclient.h"

#include <mcp/server/toolregistry.h>

#include <utils/commandline.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

#include <functional>

using namespace Utils;

namespace Git::Internal {

using ToolResult = Result<Mcp::Schema::CallToolResult>;
using OutputHandler = std::function<Mcp::Schema::CallToolResult(const QString &)>;

static ToolResult toolError(const QString &message)
{
    return Mcp::Schema::CallToolResult{}.isError(true).addContent(
        Mcp::Schema::TextContent{}.text(message));
}

// Resolve the Git repository that contains the given file or directory path.
static Result<FilePath> repositoryFor(const QString &path)
{
    if (path.isEmpty())
        return ResultError(QString("Missing required argument \"path\"."));
    const FilePath fp = FilePath::fromUserInput(path);
    const FilePath dir = fp.isDir() ? fp : fp.parentDir();
    const FilePath repo = gitClient().findRepositoryForDirectory(dir);
    if (repo.isEmpty()) {
        return ResultError(
            QString("No Git repository found for \"%1\".").arg(fp.toUserOutput()));
    }
    return repo;
}

// Run a read-only git command asynchronously and deliver the result via the
// tool interface. The MCP server dispatches tool callbacks on Creator's GUI
// thread, so git must never be run blocking here.
static void runGitAsync(const FilePath &repository, const QStringList &arguments,
                        const Mcp::ToolInterface &toolInterface, const OutputHandler &onOutput)
{
    const FilePath git = gitClient().vcsBinary(repository);
    if (git.isEmpty()) {
        toolInterface.finish(toolError(QString("No Git executable is configured.")));
        return;
    }
    // Capture a COPY of the interface: returning from the callback would
    // otherwise finalize the call, so the copy keeps it alive until done.
    auto process = new Process;
    QObject::connect(process, &Process::done, process, [process, toolInterface, onOutput] {
        if (process->result() == ProcessResult::FinishedWithSuccess) {
            toolInterface.finish(onOutput(process->cleanedStdOut()));
        } else {
            const QString error = process->cleanedStdErr().trimmed();
            toolInterface.finish(toolError(error.isEmpty() ? process->exitMessage() : error));
        }
        process->deleteLater();
    });
    process->setCommand({git, arguments});
    process->setWorkingDirectory(repository);
    process->setEnvironment(gitClient().processEnvironment(repository));
    process->setUtf8Codec(); // git emits UTF-8; do not decode with the system locale.
    process->start();
}

void registerMcpTools()
{
    using namespace Mcp::Schema;
    using Mcp::ToolInterface;
    using Mcp::ToolRegistry;

    ToolRegistry::registerTool(
        Tool{}
            .name("git_status")
            .title("Get Git status")
            .description(
                "Returns the Git working-tree status of the repository that contains a "
                "path: the current branch and the list of changed files, each with its "
                "two-character porcelain status code. Renames and copies also report the "
                "\"original_path\". On a detached HEAD \"branch\" is empty and "
                "\"detached\" is true. Give any file or directory inside the repository "
                "as \"path\".")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to a file or directory inside the repository."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("branch", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "detached",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "True if HEAD is detached; \"branch\" is empty then."}})
                    .addProperty(
                        "changes",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addRequired("changes")),
        [](const CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const Utils::Result<FilePath> repo = repositoryFor(
                params.argumentsAsObject().value("path").toString());
            if (!repo) {
                toolInterface.finish(toolError(repo.error()));
                return ResultOk;
            }
            const QString repository = repo->toUserOutput();
            runGitAsync(*repo, {"status", "--porcelain=v1", "--branch"}, toolInterface,
                        [repository](const QString &out) -> CallToolResult {
                QString branch;
                bool detached = false;
                QJsonArray changes;
                const QList<QStringView> lines = QStringView(out).split(u'\n');
                for (const QStringView &line : lines) {
                    if (line.isEmpty())
                        continue;
                    if (line.startsWith(u"## ")) {
                        // The header is a branch name only some of the time: a
                        // detached HEAD reads "HEAD (no branch)" and a repository
                        // without commits "No commits yet on <branch>".
                        const QStringView rest = line.mid(3);
                        if (rest == u"HEAD (no branch)") {
                            detached = true;
                            continue;
                        }
                        const qsizetype track = rest.indexOf(u"...");
                        QStringView name = track >= 0 ? rest.left(track) : rest;
                        if (name.startsWith(u"No commits yet on "))
                            name = name.mid(18);
                        branch = name.toString();
                        continue;
                    }
                    // A rename or copy is "R  old -> new" / "C  old -> new".
                    const QStringView rest = line.mid(3);
                    const qsizetype arrow = rest.indexOf(u" -> ");
                    QJsonObject change{{"status", line.left(2).toString()}};
                    if (arrow >= 0) {
                        change.insert("original_path", rest.left(arrow).toString());
                        change.insert("path", rest.mid(arrow + 4).toString());
                    } else {
                        change.insert("path", rest.toString());
                    }
                    changes.append(change);
                }
                return CallToolResult{}.isError(false).structuredContent(QJsonObject{
                    {"repository", repository},
                    {"branch", branch},
                    {"detached", detached},
                    {"clean", changes.isEmpty()},
                    {"changes", changes}});
            });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("git_log")
            .title("Get Git commit log")
            .description(
                "Returns recent Git commits for the repository that contains a path, most "
                "recent first, each with its hash, author, date and subject. Give any file "
                "or directory inside the repository as \"path\"; optionally cap the count "
                "with \"max_count\" and restrict history to one \"file\".")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to a file or directory inside the repository."}})
                    .addProperty(
                        "max_count",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Maximum number of commits (default 20)."}})
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Optional: restrict the log to this file's history."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "commits",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addRequired("commits")),
        [](const CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const Utils::Result<FilePath> repo = repositoryFor(args.value("path").toString());
            if (!repo) {
                toolInterface.finish(toolError(repo.error()));
                return ResultOk;
            }
            int maxCount = args.value("max_count").toInt();
            if (maxCount <= 0)
                maxCount = 20;

            // Separate fields with the unit separator so subjects can contain anything.
            QStringList arguments{"log",
                                  "--max-count=" + QString::number(maxCount),
                                  "--date=short",
                                  "--pretty=format:%H%x1f%an%x1f%ad%x1f%s"};
            const QString file = args.value("file").toString();
            if (!file.isEmpty())
                arguments << "--" << FilePath::fromUserInput(file).path();
            const QString repository = repo->toUserOutput();
            runGitAsync(*repo, arguments, toolInterface,
                        [repository](const QString &out) -> CallToolResult {
                QJsonArray commits;
                const QList<QStringView> lines = QStringView(out).split(u'\n', Qt::SkipEmptyParts);
                for (const QStringView &line : lines) {
                    const QList<QStringView> fields = line.split(u'\x1f');
                    if (fields.size() != 4)
                        continue;
                    commits.append(QJsonObject{
                        {"hash", fields.at(0).toString()},
                        {"author", fields.at(1).toString()},
                        {"date", fields.at(2).toString()},
                        {"subject", fields.at(3).toString()}});
                }
                return CallToolResult{}.isError(false).structuredContent(
                    QJsonObject{{"repository", repository}, {"commits", commits}});
            });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("git_diff")
            .title("Get Git diff")
            .description(
                "Returns the unified diff of uncommitted changes in the repository that "
                "contains a path. Give any file or directory inside the repository as "
                "\"path\"; set \"staged\" to diff the index against HEAD, or restrict the "
                "diff to one \"file\". The diff is cut at the last full line that fits "
                "into \"limit\" characters; \"total_characters\" and \"truncated\" report "
                "what was left out.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "path",
                        QJsonObject{
                            {"type", "string"},
                            {"description",
                             "Absolute path to a file or directory inside the repository."}})
                    .addProperty(
                        "staged",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description",
                             "Diff staged changes (index vs HEAD) instead of the working "
                             "tree (default false)."}})
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Optional: restrict the diff to this file."}})
                    .addProperty(
                        "limit",
                        QJsonObject{
                            {"type", "integer"},
                            {"description",
                             "Maximum number of characters of diff to return "
                             "(default 100000)."}})
                    .addRequired("path"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("diff", QJsonObject{{"type", "string"}})
                    .addProperty(
                        "total_characters",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Size of the full diff before the limit was "
                                            "applied."}})
                    .addProperty(
                        "truncated",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "True if the diff was cut at the limit."}})
                    .addRequired("diff")),
        [](const CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const Utils::Result<FilePath> repo = repositoryFor(args.value("path").toString());
            if (!repo) {
                toolInterface.finish(toolError(repo.error()));
                return ResultOk;
            }

            // --no-color: color.ui=always in the user's config would otherwise
            // leave ANSI escapes in the output.
            QStringList arguments{"diff", "--no-color"};
            if (args.value("staged").toBool())
                arguments << "--staged";
            const QString file = args.value("file").toString();
            if (!file.isEmpty())
                arguments << "--" << FilePath::fromUserInput(file).path();
            int limit = args.value("limit").toInt();
            if (limit <= 0)
                limit = 100000;
            const QString repository = repo->toUserOutput();
            runGitAsync(*repo, arguments, toolInterface,
                        [repository, limit](const QString &out) -> CallToolResult {
                // A whole-tree diff can be megabytes, and makeResponse()
                // duplicates it into a TextContent, so cap it - at a line
                // boundary, to not hand out half a diff line.
                QString diff = out;
                const bool truncated = diff.size() > limit;
                if (truncated) {
                    const qsizetype lastNewline = QStringView(diff).left(limit).lastIndexOf(u'\n');
                    diff.truncate(lastNewline >= 0 ? lastNewline + 1 : limit);
                }
                return CallToolResult{}.isError(false).structuredContent(
                    QJsonObject{{"repository", repository},
                                {"diff", diff},
                                {"total_characters", int(out.size())},
                                {"truncated", truncated}});
            });
            return ResultOk;
        });

    ToolRegistry::registerTool(
        Tool{}
            .name("git_blame")
            .title("Get Git blame")
            .description(
                "Returns line-by-line authorship (git blame) for a file: for each line, "
                "the commit hash, author and commit subject. Give the \"file\" and "
                "optionally a \"start_line\"/\"end_line\" range (1-based) to limit the "
                "output; \"start_line\" alone blames from there to the end, "
                "\"end_line\" alone from the beginning. An \"end_line\" past the end of "
                "the file makes git fail, so omit it to blame to the end. At most "
                "\"limit\" lines are returned; \"total_lines\" and \"truncated\" report "
                "what was left out.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty(
                        "file",
                        QJsonObject{
                            {"type", "string"},
                            {"description", "Absolute path to the file to blame."}})
                    .addProperty(
                        "start_line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Optional: first 1-based line to blame."}})
                    .addProperty(
                        "end_line",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Optional: last 1-based line to blame."}})
                    .addProperty(
                        "limit",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Maximum number of lines to return "
                                            "(default 2000)."}})
                    .addRequired("file"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty(
                        "lines",
                        QJsonObject{{"type", "array"},
                                    {"items", QJsonObject{{"type", "object"}}}})
                    .addProperty(
                        "total_lines",
                        QJsonObject{
                            {"type", "integer"},
                            {"description", "Number of blamed lines before the limit was "
                                            "applied."}})
                    .addProperty(
                        "truncated",
                        QJsonObject{
                            {"type", "boolean"},
                            {"description", "True if more lines were blamed than "
                                            "returned."}})
                    .addRequired("lines")),
        [](const CallToolRequestParams &params,
           const ToolInterface &toolInterface) -> Utils::Result<> {
            const QJsonObject args = params.argumentsAsObject();
            const QString file = args.value("file").toString();
            const Utils::Result<FilePath> repo = repositoryFor(file);
            if (!repo) {
                toolInterface.finish(toolError(repo.error()));
                return ResultOk;
            }

            QStringList arguments{"blame", "--line-porcelain"};
            const int startLine = args.value("start_line").toInt();
            const int endLine = args.value("end_line").toInt();
            // Both bounds are independently optional, so a missing start means
            // line 1 and a missing end the end of the file.
            if (startLine > 0 || endLine > 0) {
                const int first = qMax(startLine, 1);
                arguments << "-L"
                          << (endLine >= first ? QString("%1,%2").arg(first).arg(endLine)
                                               : QString("%1,").arg(first));
            }
            arguments << "--" << FilePath::fromUserInput(file).path();
            int limit = args.value("limit").toInt();
            if (limit <= 0)
                limit = 2000;
            const QString repository = repo->toUserOutput();
            const QString fileOut = FilePath::fromUserInput(file).toUserOutput();
            runGitAsync(*repo, arguments, toolInterface,
                        [repository, fileOut, limit](const QString &out) -> CallToolResult {
                // 40 hex digits for SHA-1, 64 for a SHA-256 repository.
                static const QRegularExpression header(
                    QStringLiteral("^([0-9a-f]{40,64}) \\d+ (\\d+)"));
                QJsonArray lines;
                QString hash;
                QString author;
                QString summary;
                int finalLine = 0;
                int totalLines = 0;
                const QList<QStringView> outLines = QStringView(out).split(u'\n');
                for (const QStringView &line : outLines) {
                    if (line.startsWith(u'\t')) {
                        ++totalLines;
                        if (lines.size() >= limit)
                            continue;
                        lines.append(QJsonObject{
                            {"line", finalLine},
                            {"hash", hash.left(12)},
                            {"author", author},
                            {"summary", summary}});
                    } else if (line.startsWith(u"author ")) {
                        author = line.mid(7).toString();
                    } else if (line.startsWith(u"summary ")) {
                        summary = line.mid(8).toString();
                    } else {
                        const QRegularExpressionMatch match = header.matchView(line);
                        if (match.hasMatch()) {
                            hash = match.captured(1);
                            finalLine = match.captured(2).toInt();
                        }
                    }
                }
                QJsonObject result;
                result.insert("repository", repository);
                result.insert("file", fileOut);
                result.insert("lines", lines);
                result.insert("total_lines", totalLines);
                result.insert("truncated", totalLines > limit);
                return CallToolResult{}.isError(false).structuredContent(result);
            });
            return ResultOk;
        });
}

} // namespace Git::Internal
