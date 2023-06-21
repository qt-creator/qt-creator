// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolslogfilereader.h"

#include <cppeditor/cppprojectfile.h>

#include <QDir>
#include <QFileInfo>

#include <utils/fileutils.h>
#include <utils/textutils.h>

#include <QFuture>

#include <yaml-cpp/yaml.h>

namespace ClangTools {
namespace Internal {

std::optional<LineColumnInfo> byteOffsetInUtf8TextToLineColumn(const char *text,
                                                                 int offset,
                                                                 int startLine)
{
    if (text == nullptr || offset < 0)
        return {};

    int lineCounter = startLine;
    const char *lineStart = text;

    for (const char *c = text; *c != '\0'; ++c) {
        // Advance to line
        if (c > text && *(c - 1) == '\n') {
            ++lineCounter;
            lineStart = c;
        }

        // Advance to column
        if (c - text == offset) {
            int columnCounter = 1;
            c = lineStart;
            while (c < text + offset && Utils::Text::utf8AdvanceCodePoint(c))
                ++columnCounter;
            if (c == text + offset)
                return LineColumnInfo{lineCounter, columnCounter, static_cast<int>(lineStart - text)};
            return {}; // Ops, offset was not pointing to start of multi byte code point.
        }
    }

    return {};
}

static QString asString(const YAML::Node &node)
{
    return QString::fromStdString(node.as<std::string>());
}

namespace  {
class FileCache
{
public:
    class LineInfo {
    public:
        bool isValid() { return line != 0; }
        int line = 0; // 1-based
        int lineStartOffset = 0;
    };

    class Item {
    public:
        friend class FileCache;

        QByteArray fileContents()
        {
            if (data.isNull())
                data = readFile(filePath);
            return data;
        }

        LineInfo &lineInfo() { return lastLookup; }

    private:
        QString filePath;
        LineInfo lastLookup;
        QByteArray data;
    };

    Item &item(const QString &filePath)
    {
        Item &i = m_cache[filePath];
        if (i.filePath.isEmpty())
            i.filePath = filePath;
        return i;
    }

private:
    static QByteArray readFile(const QString &filePath)
    {
        if (filePath.isEmpty())
            return {};

        Utils::FileReader reader;
        // Do not use QIODevice::Text as we have to deal with byte offsets.
        if (reader.fetch(Utils::FilePath::fromString(filePath), QIODevice::ReadOnly))
            return reader.data();

        return {};
    }

private:
    QHash<QString, Item> m_cache;
};

class Location
{
public:
    Location(const YAML::Node &node,
             FileCache &fileCache,
             const char *fileOffsetKey = "FileOffset",
             int extraOffset = 0)
        : m_node(node)
        , m_fileCache(fileCache)
        , m_filePath(Utils::FilePath::fromUserInput(asString(node["FilePath"])))
        , m_fileOffsetKey(fileOffsetKey)
        , m_extraOffset(extraOffset)
    {}

    Utils::FilePath filePath() const { return m_filePath; }

    Debugger::DiagnosticLocation toDiagnosticLocation() const
    {
        FileCache::Item &cacheItem = m_fileCache.item(m_filePath.toString());
        const QByteArray fileContents = cacheItem.fileContents();

        const char *data = fileContents.data();
        int fileOffset = m_node[m_fileOffsetKey].as<int>() + m_extraOffset;
        int startLine = 1;

        // Check cache for last lookup
        FileCache::LineInfo &cachedLineInfo = cacheItem.lineInfo();
        if (cachedLineInfo.isValid() && fileOffset >= cachedLineInfo.lineStartOffset) {
            // Cache hit, adjust inputs in order not to start from the beginning of the file again.
            data = data + cachedLineInfo.lineStartOffset;
            fileOffset = fileOffset - cachedLineInfo.lineStartOffset;
            startLine = cachedLineInfo.line;
        }

        // Convert
        OptionalLineColumnInfo info = byteOffsetInUtf8TextToLineColumn(data, fileOffset, startLine);
        if (!info)
            return {m_filePath, 1, 1};

        // Save/update lookup
        int lineStartOffset = info->lineStartOffset;
        if (data != fileContents.data())
            lineStartOffset += cachedLineInfo.lineStartOffset;
        cachedLineInfo = FileCache::LineInfo{info->line, lineStartOffset};
        return Debugger::DiagnosticLocation{m_filePath, info->line, info->column};
    }

    static QVector<Debugger::DiagnosticLocation> toRange(const YAML::Node &node,
                                                         FileCache &fileCache)
    {
        // The Replacements nodes use "Offset" instead of "FileOffset" as the key name.
        auto startLoc = Location(node, fileCache, "Offset");
        auto endLoc = Location(node, fileCache, "Offset", node["Length"].as<int>());
        return {startLoc.toDiagnosticLocation(), endLoc.toDiagnosticLocation()};
    }

private:
    const YAML::Node &m_node;
    FileCache &m_fileCache;
    Utils::FilePath m_filePath;
    const char *m_fileOffsetKey = nullptr;
    int m_extraOffset = 0;
};

} // namespace

void parseDiagnostics(QPromise<Utils::expected_str<Diagnostics>> &promise,
                      const Utils::FilePath &logFilePath,
                      const AcceptDiagsFromFilePath &acceptFromFilePath)
{
    const Utils::expected_str<QByteArray> localFileContents = logFilePath.fileContents();
    if (!localFileContents.has_value()) {
        promise.addResult(Utils::make_unexpected(localFileContents.error()));
        promise.future().cancel();
        return;
    }

    FileCache fileCache;
    Diagnostics diagnostics;

    try {
        YAML::Node document = YAML::Load(*localFileContents);
        for (const auto &diagNode : document["Diagnostics"]) {
            if (promise.isCanceled())
                return;
            // Since llvm/clang 9.0 the diagnostic items are wrapped in a "DiagnosticMessage" node.
            const auto msgNode = diagNode["DiagnosticMessage"];
            const YAML::Node &node = msgNode ? msgNode : diagNode;

            Location loc(node, fileCache);
            if (loc.filePath().isEmpty())
                continue;
            if (acceptFromFilePath && !acceptFromFilePath(loc.filePath()))
                continue;

            Diagnostic diag;
            diag.location = loc.toDiagnosticLocation();
            diag.type = "warning";
            diag.name = asString(diagNode["DiagnosticName"]);
            diag.description = asString(node["Message"]) + " [" + diag.name + "]";

            // Process fixits/replacements
            const YAML::Node &replacementsNode = node["Replacements"];
            for (const YAML::Node &replacementNode : replacementsNode) {
                ExplainingStep step;
                step.isFixIt = true;
                step.message = asString(replacementNode["ReplacementText"]);
                step.ranges = Location::toRange(replacementNode, fileCache);
                step.location = step.ranges[0];

                if (step.location.isValid())
                    diag.explainingSteps.append(step);
            }
            diag.hasFixits = !diag.explainingSteps.isEmpty();

            // Process notes
            const auto notesNode = diagNode["Notes"];
            for (const YAML::Node &noteNode : notesNode) {
                Location loc(noteNode, fileCache);
                // Ignore a note like
                //   - FileOffset: 0
                //     FilePath: ''
                //     Message: this fix will not be applied because it overlaps with another fix
                if (loc.filePath().isEmpty())
                    continue;

                ExplainingStep step;
                step.message = asString(noteNode["Message"]);
                step.location = loc.toDiagnosticLocation();
                diag.explainingSteps.append(step);
            }

            diagnostics.append(diag);
        }
        promise.addResult(diagnostics);
    } catch (std::exception &e) {
        const QString errorMessage
            = QString(QT_TRANSLATE_NOOP("QtC::ClangTools",
                                        "Error: Failed to parse YAML file \"%1\": %2."))
                  .arg(logFilePath.toUserOutput(), QString::fromUtf8(e.what()));
        promise.addResult(Utils::make_unexpected(errorMessage));
        promise.future().cancel();
    }
}

Utils::expected_str<Diagnostics> readExportedDiagnostics(
    const Utils::FilePath &logFilePath, const AcceptDiagsFromFilePath &acceptFromFilePath)
{
    QPromise<Utils::expected_str<Diagnostics>> promise;
    promise.start();
    parseDiagnostics(promise, logFilePath, acceptFromFilePath);
    return promise.future().result();
}

} // namespace Internal
} // namespace ClangTools
