/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangtoolslogfilereader.h"

#include <cpptools/cppprojectfile.h>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QRegularExpression>
#include <QXmlStreamReader>

#include <utils/executeondestruction.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/textutils.h>

#include <clang-c/Index.h>

#include <yaml-cpp/yaml.h>

namespace ClangTools {
namespace Internal {

static QString fromCXString(CXString &&cxString)
{
    QString result = QString::fromUtf8(clang_getCString(cxString));
    clang_disposeString(cxString);
    return result;
}

static Debugger::DiagnosticLocation diagLocationFromSourceLocation(CXSourceLocation cxLocation)
{
    CXFile file;
    unsigned line;
    unsigned column;
    clang_getSpellingLocation(cxLocation, &file, &line, &column, nullptr);

    Debugger::DiagnosticLocation location;
    location.filePath = fromCXString(clang_getFileName(file));
    location.filePath = QDir::cleanPath(location.filePath); // Normalize to find duplicates later
    location.line = line;
    location.column = column;
    return location;
}

static QString cxDiagnosticType(const CXDiagnostic cxDiagnostic)
{
    const CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(cxDiagnostic);
    switch (severity) {
    case CXDiagnostic_Note:
        return QString("note");
    case CXDiagnostic_Warning:
        return QString("warning");
    case CXDiagnostic_Error:
        return QString("error");
    case CXDiagnostic_Fatal:
        return QString("fatal");
    case CXDiagnostic_Ignored:
        return QString("ignored");
    }
    return QString("ignored");
}

static ExplainingStep buildChildDiagnostic(const CXDiagnostic cxDiagnostic)
{
    ExplainingStep diagnosticStep;
    QString type = cxDiagnosticType(cxDiagnostic);
    if (type == QStringLiteral("ignored"))
        return diagnosticStep;

    const CXSourceLocation cxLocation = clang_getDiagnosticLocation(cxDiagnostic);
    diagnosticStep.location = diagLocationFromSourceLocation(cxLocation);
    diagnosticStep.message = fromCXString(clang_getDiagnosticSpelling(cxDiagnostic));
    return diagnosticStep;
}

static bool isInvalidDiagnosticLocation(const Diagnostic &diagnostic, const ExplainingStep &child,
                                        const QString &nativeFilePath)
{
    // When main file is considered included by itself - this diagnostic has invalid location.
    // This case usually happens when original diagnostic comes from system header but
    // has main file name set in the source location instead (which is incorrect).
    return child.message.indexOf(nativeFilePath) >= 0
            && child.message.indexOf("in file included from") >= 0
            && diagnostic.location.filePath == nativeFilePath;
}

static ExplainingStep buildFixIt(const CXDiagnostic cxDiagnostic, unsigned index)
{
    ExplainingStep fixItStep;
    CXSourceRange cxFixItRange;
    fixItStep.isFixIt = true;
    fixItStep.message = fromCXString(clang_getDiagnosticFixIt(cxDiagnostic, index, &cxFixItRange));
    fixItStep.location = diagLocationFromSourceLocation(clang_getRangeStart(cxFixItRange));
    fixItStep.ranges.push_back(fixItStep.location);
    fixItStep.ranges.push_back(diagLocationFromSourceLocation(clang_getRangeEnd(cxFixItRange)));
    return fixItStep;
}

static Diagnostic buildDiagnostic(const CXDiagnostic cxDiagnostic,
                                  const AcceptDiagsFromFilePath &acceptFromFilePath,
                                  const QString &nativeFilePath)
{
    Diagnostic diagnostic;
    diagnostic.type = cxDiagnosticType(cxDiagnostic);
    if (diagnostic.type == QStringLiteral("ignored"))
        return diagnostic;

    const CXSourceLocation cxLocation = clang_getDiagnosticLocation(cxDiagnostic);
    if (clang_Location_isInSystemHeader(cxLocation))
        return diagnostic;

    diagnostic.location = diagLocationFromSourceLocation(cxLocation);
    const auto diagnosticFilePath = Utils::FilePath::fromString(diagnostic.location.filePath);
    if (acceptFromFilePath && !acceptFromFilePath(diagnosticFilePath))
        return diagnostic;

    // TODO: Introduce CppTools::ProjectFile::isGenerated to filter these out properly
    const QString fileName = diagnosticFilePath.fileName();
    if ((fileName.startsWith("ui_") && fileName.endsWith(".h")) || fileName.endsWith(".moc"))
        return diagnostic;

    CXDiagnosticSet cxChildDiagnostics = clang_getChildDiagnostics(cxDiagnostic);
    Utils::ExecuteOnDestruction onBuildExit([&]() {
        clang_disposeDiagnosticSet(cxChildDiagnostics);
    });

    using CppTools::ProjectFile;
    const bool isHeaderFile = ProjectFile::isHeader(
        ProjectFile::classify(diagnostic.location.filePath));

    for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(cxChildDiagnostics); ++i) {
        CXDiagnostic cxDiagnostic = clang_getDiagnosticInSet(cxChildDiagnostics, i);
        Utils::ExecuteOnDestruction cleanUpDiagnostic([&]() {
            clang_disposeDiagnostic(cxDiagnostic);
        });
        const ExplainingStep diagnosticStep = buildChildDiagnostic(cxDiagnostic);
        if (diagnosticStep.isValid())
            continue;

        if (isHeaderFile && diagnosticStep.message.contains("in file included from"))
            continue;

        if (isInvalidDiagnosticLocation(diagnostic, diagnosticStep, nativeFilePath))
            return diagnostic;

        diagnostic.explainingSteps.push_back(diagnosticStep);
    }

    const unsigned fixItCount = clang_getDiagnosticNumFixIts(cxDiagnostic);
    diagnostic.hasFixits = fixItCount != 0;
    for (unsigned i = 0; i < fixItCount; ++i)
        diagnostic.explainingSteps.push_back(buildFixIt(cxDiagnostic, i));

    diagnostic.description = fromCXString(clang_getDiagnosticSpelling(cxDiagnostic));
    diagnostic.category = fromCXString(clang_getDiagnosticCategoryText(cxDiagnostic));

    return diagnostic;
}

static Diagnostics readSerializedDiagnostics_helper(const Utils::FilePath &logFilePath,
                                                    const Utils::FilePath &mainFilePath,
                                                    const AcceptDiagsFromFilePath &acceptFromFilePath)
{
    Diagnostics list;
    CXLoadDiag_Error error;
    CXString errorString;

    CXDiagnosticSet diagnostics = clang_loadDiagnostics(logFilePath.toString().toStdString().c_str(),
                                                        &error,
                                                        &errorString);
    if (error != CXLoadDiag_None || !diagnostics)
        return list;

    Utils::ExecuteOnDestruction onReadExit([&]() {
        clang_disposeDiagnosticSet(diagnostics);
    });

    const QString nativeFilePath = QDir::toNativeSeparators(mainFilePath.toString());
    for (unsigned i = 0; i < clang_getNumDiagnosticsInSet(diagnostics); ++i) {
        CXDiagnostic cxDiagnostic = clang_getDiagnosticInSet(diagnostics, i);
        Utils::ExecuteOnDestruction cleanUpDiagnostic([&]() {
            clang_disposeDiagnostic(cxDiagnostic);
        });
        const Diagnostic diagnostic = buildDiagnostic(cxDiagnostic, acceptFromFilePath, nativeFilePath);
        if (!diagnostic.isValid())
            continue;

        list.push_back(diagnostic);
    }

    return list;
}

static bool checkFilePath(const Utils::FilePath &filePath, QString *errorMessage)
{
    QFileInfo fi(filePath.toFileInfo());
    if (!fi.exists() || !fi.isReadable()) {
        if (errorMessage) {
            *errorMessage
                    = QString(QT_TRANSLATE_NOOP("LogFileReader",
                                                "File \"%1\" does not exist or is not readable."))
                    .arg(filePath.toUserOutput());
        }
        return false;
    }
    return true;
}

Diagnostics readSerializedDiagnostics(const Utils::FilePath &logFilePath,
                                      const Utils::FilePath &mainFilePath,
                                      const AcceptDiagsFromFilePath &acceptFromFilePath,
                                      QString *errorMessage)
{
    if (!checkFilePath(logFilePath, errorMessage))
        return {};

    return readSerializedDiagnostics_helper(logFilePath, mainFilePath, acceptFromFilePath);
}

Utils::optional<LineColumnInfo> byteOffsetInUtf8TextToLineColumn(const char *text,
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
        if (reader.fetch(filePath, QIODevice::ReadOnly))
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
        , m_filePath(QDir::cleanPath(asString(node["FilePath"])))
        , m_fileOffsetKey(fileOffsetKey)
        , m_extraOffset(extraOffset)
    {}

    QString filePath() const { return m_filePath; }

    Debugger::DiagnosticLocation toDiagnosticLocation() const
    {
        FileCache::Item &cacheItem = m_fileCache.item(m_filePath);
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
    QString m_filePath;
    const char *m_fileOffsetKey = nullptr;
    int m_extraOffset = 0;
};

} // namespace

Diagnostics readExportedDiagnostics(const Utils::FilePath &logFilePath,
                                    const AcceptDiagsFromFilePath &acceptFromFilePath,
                                    QString *errorMessage)
{
    if (!checkFilePath(logFilePath, errorMessage))
        return {};

    FileCache fileCache;
    Diagnostics diagnostics;

    try {
        YAML::Node document = YAML::LoadFile(logFilePath.toString().toStdString());
        for (const auto &diagNode : document["Diagnostics"]) {
            // Since llvm/clang 9.0 the diagnostic items are wrapped in a "DiagnosticMessage" node.
            const auto msgNode = diagNode["DiagnosticMessage"];
            const YAML::Node &node = msgNode ? msgNode : diagNode;

            Location loc(node, fileCache);
            if (loc.filePath().isEmpty())
                continue;
            if (acceptFromFilePath
                   && !acceptFromFilePath(Utils::FilePath::fromString(loc.filePath()))) {
                continue;
            }

            Diagnostic diag;
            diag.location = loc.toDiagnosticLocation();
            diag.type = "warning";
            diag.description = asString(node["Message"]) + " ["
                               + (asString(diagNode["DiagnosticName"])) + "]";

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
    } catch (std::exception &e) {
        if (errorMessage) {
            *errorMessage = QString(
                                QT_TRANSLATE_NOOP("LogFileReader",
                                                  "Error: Failed to parse YAML file \"%1\": %2."))
                                .arg(logFilePath.toUserOutput(), QString::fromUtf8(e.what()));
        }
    }

    return diagnostics;
}

} // namespace Internal
} // namespace ClangTools
