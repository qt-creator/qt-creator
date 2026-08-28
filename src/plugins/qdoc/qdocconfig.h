// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qdocparser.h"

#include <utils/filepath.h>

#include <QHash>
#include <QString>
#include <QStringList>

#include <optional>

namespace QDoc::Internal {

class ConfigEntry
{
public:
    QString value;
    Utils::FilePath dir;
    Utils::FilePath file;
    int line = 0;
};

class ConfigProblem
{
public:
    QString message;
    Utils::FilePath file;
    int line = 0;
};

class Config
{
public:
    void set(const QString &key, const QList<ConfigEntry> &values, bool replace);
    QList<ConfigEntry> entries(const QString &key) const { return m_vars.value(key); }
    QStringList values(const QString &key) const;
    QString value(const QString &key, const QString &fallback = {}) const;
    bool boolValue(const QString &key, bool fallback = false) const;
    int intValue(const QString &key, int fallback) const;
    Utils::FilePaths pathList(const QString &key, bool mustExist = true) const;
    Utils::FilePaths fileList(const QString &key) const;
    bool excludes(const Utils::FilePath &filePath) const;
    QList<ConfigProblem> missingPaths(const QStringList &keys) const;
    QStringList keysWithPrefix(const QString &prefix) const;

    Utils::FilePath rootFile;
    Utils::FilePaths files;
    QList<ConfigProblem> problems;
    QHash<QString, QStringList> expandedIn;

private:
    friend Config parseConfig(const Utils::FilePath &rootFile,
                              const QHash<QString, QString> &variables);
    QHash<QString, QList<ConfigEntry>> m_vars;
    // Declaration order: $key references are settled in it, so a value built from a
    // key that is itself pending must be expanded after that key.
    QStringList m_order;
};

Config parseConfig(const Utils::FilePath &rootFile,
                   const QHash<QString, QString> &variables = {});

QStringList expandKey(const QString &key);
bool wildcardMatches(const QString &pattern, const QString &filePath);

QHash<QString, Macro> buildMacroTable(const Config &config, const QString &format = "HTML");

// Variables the documentation build supplies that a preview cannot know, with the
// empty value they take until something fills them in. Every one is set by
// qt_internal_add_docs(); defining them as empty means nothing is reported about
// them, which is the point.
QHash<QString, QString> buildVariables();

class QuoteResult
{
public:
    bool ok = false;
    QString text;
    QString lang;
    Utils::FilePath path;
    QString reason;
};

class QuoteCache
{
public:
    // Anything derived from a file's contents is dropped before each render, so an
    // edited snippet is picked up; the path lookups survive, since they only change
    // when a file appears or disappears.
    void forgetContents()
    {
        sessionSegments.clear();
        fileLines.clear();
    }

    // Keyed by the quoted file and the steps that walk it, so a session split over
    // several listings reads its file once while two sessions over the same file keep
    // their own cursors.
    QHash<QString, QStringList> sessionSegments;
    QHash<QString, Utils::FilePath> fileSearch;
    // The lines of each quoted file: a page often takes dozens of snippets out of
    // one file, and splitting it again for each of them was the largest cost of
    // rendering such a page.
    QHash<QString, QStringList> fileLines;
};

QuoteResult resolveQuote(const Node &node,
                         const QList<QList<QuoteStep>> &sessionGroups,
                         const Utils::FilePaths &searchDirs,
                         QuoteCache *cache = nullptr);

std::optional<QString> extractSnippet(const QString &text, const QString &identifier,
                                      const QString &marker);
QString commentMarkerFor(const Utils::FilePath &filePath);
QString languageFor(const Utils::FilePath &filePath);
QStringList runQuoteSegments(const QString &text, const QList<QList<QuoteStep>> &groups);
QString trimWhiteSpace(const QString &text);
bool matchesQuotePattern(const QString &line, const QString &pattern);
Utils::FilePath findQuoteFile(const QString &name,
                              const Utils::FilePaths &searchDirs,
                              QHash<QString, Utils::FilePath> *cache = nullptr);
Utils::FilePath findImageFile(const QString &name, const Utils::FilePaths &searchDirs);
Utils::FilePath findRawImageFile(const QString &src,
                                 const Utils::FilePaths &searchDirs,
                                 const Utils::FilePaths &extraImageFiles);
Utils::FilePaths findExampleImageDirs(const Utils::FilePaths &exampleDirs);

// Everything the preview needs for one file, taken from the .qdocconf that governs
// it.
class DocContext
{
public:
    Utils::FilePath confFile;
    // Every file the configuration was read from, so a change to any of them can
    // refresh what was rendered against it.
    Utils::FilePaths confFiles;
    QString project;
    QHash<QString, Macro> macros;
    Utils::FilePaths imageSearchDirs;
    Utils::FilePaths exampleDirs;
    Utils::FilePaths extraImageFiles;
    Utils::FilePaths sourceDirs;
    QSet<QString> defines;
    QSet<QString> codeLanguages;
    QStringList spurious;
    int tabSize = 8;
    bool reportMissingAltText = false;
    QList<ConfigProblem> problems;
};

// .qdocconf files that may govern a file in startDir: those beside it and in the doc
// directories of its ancestors.
Utils::FilePaths findConfigFiles(const Utils::FilePath &startDir, int maxLevels = 12);

class ConfigResolver
{
public:
    void setVariables(const QHash<QString, QString> &variables);
    DocContext contextFor(const Utils::FilePath &file, const Utils::FilePaths &candidates);
    void clearCaches();

private:
    class ParsedConfig
    {
    public:
        Config config;
        QHash<QString, Macro> macros;
        Utils::FilePaths exampleImageDirs;
        QHash<Utils::FilePath, QDateTime> stamps;
        bool exampleImageDirsKnown = false;
    };

    ParsedConfig &parseCached(const Utils::FilePath &confFile);
    Utils::FilePath pick(const Utils::FilePath &file, const Utils::FilePaths &candidates);
    Utils::FilePath verifyBySourceDirs(const Utils::FilePath &file,
                                       const Utils::FilePaths &candidates);

    QHash<QString, QString> m_variables;
    QHash<Utils::FilePath, ParsedConfig> m_parsed;
    QHash<Utils::FilePath, Utils::FilePath> m_association;
};

} // namespace QDoc::Internal
