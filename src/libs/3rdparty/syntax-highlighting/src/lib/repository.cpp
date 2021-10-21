/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "repository.h"
#include "definition.h"
#include "definition_p.h"
#include "ksyntaxhighlighting_logging.h"
#include "repository_p.h"
#include "theme.h"
#include "themedata_p.h"
#include "wildcardmatcher.h"

#include <QCborMap>
#include <QCborValue>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QString>
#include <QStringView>

#ifndef NO_STANDARD_PATHS
#include <QStandardPaths>
#endif

#include <algorithm>
#include <iterator>
#include <limits>

using namespace KSyntaxHighlighting;

namespace
{
QString fileNameFromFilePath(const QString &filePath)
{
    return QFileInfo{filePath}.fileName();
}

auto anyWildcardMatches(QStringView str)
{
    return [str](const Definition &def) {
        const auto strings = def.extensions();
        return std::any_of(strings.cbegin(), strings.cend(), [str](QStringView wildcard) {
            return WildcardMatcher::exactMatch(str, wildcard);
        });
    };
}

auto anyMimeTypeEquals(QStringView mimeTypeName)
{
    return [mimeTypeName](const Definition &def) {
        const auto strings = def.mimeTypes();
        return std::any_of(strings.cbegin(), strings.cend(), [mimeTypeName](QStringView name) {
            return mimeTypeName == name;
        });
    };
}

// The two function templates below take defs - a map sorted by highlighting name - to be deterministic and independent of translations.

template<typename UnaryPredicate>
Definition findHighestPriorityDefinitionIf(const QMap<QString, Definition> &defs, UnaryPredicate predicate)
{
    const Definition *match = nullptr;
    auto matchPriority = std::numeric_limits<int>::lowest();
    for (const Definition &def : defs) {
        const auto defPriority = def.priority();
        if (defPriority > matchPriority && predicate(def)) {
            match = &def;
            matchPriority = defPriority;
        }
    }
    return match == nullptr ? Definition{} : *match;
}

template<typename UnaryPredicate>
QVector<Definition> findDefinitionsIf(const QMap<QString, Definition> &defs, UnaryPredicate predicate)
{
    QVector<Definition> matches;
    std::copy_if(defs.cbegin(), defs.cend(), std::back_inserter(matches), predicate);
    std::stable_sort(matches.begin(), matches.end(), [](const Definition &lhs, const Definition &rhs) {
        return lhs.priority() > rhs.priority();
    });
    return matches;
}
} // unnamed namespace

static void initResource()
{
#ifdef HAS_SYNTAX_RESOURCE
    Q_INIT_RESOURCE(syntax_data);
#endif
    Q_INIT_RESOURCE(theme_data);
}

RepositoryPrivate *RepositoryPrivate::get(Repository *repo)
{
    return repo->d.get();
}

Repository::Repository()
    : d(new RepositoryPrivate)
{
    initResource();
    d->load(this);
}

Repository::~Repository()
{
    // reset repo so we can detect in still alive definition instances
    // that the repo was deleted
    for (const auto &def : std::as_const(d->m_sortedDefs)) {
        DefinitionData::get(def)->repo = nullptr;
    }
}

Definition Repository::definitionForName(const QString &defName) const
{
    return d->m_defs.value(defName);
}

Definition Repository::definitionForFileName(const QString &fileName) const
{
    return findHighestPriorityDefinitionIf(d->m_defs, anyWildcardMatches(fileNameFromFilePath(fileName)));
}

QVector<Definition> Repository::definitionsForFileName(const QString &fileName) const
{
    return findDefinitionsIf(d->m_defs, anyWildcardMatches(fileNameFromFilePath(fileName)));
}

Definition Repository::definitionForMimeType(const QString &mimeType) const
{
    return findHighestPriorityDefinitionIf(d->m_defs, anyMimeTypeEquals(mimeType));
}

QVector<Definition> Repository::definitionsForMimeType(const QString &mimeType) const
{
    return findDefinitionsIf(d->m_defs, anyMimeTypeEquals(mimeType));
}

QVector<Definition> Repository::definitions() const
{
    return d->m_sortedDefs;
}

QVector<Theme> Repository::themes() const
{
    return d->m_themes;
}

Theme Repository::theme(const QString &themeName) const
{
    for (const auto &theme : std::as_const(d->m_themes)) {
        if (theme.name() == themeName) {
            return theme;
        }
    }

    return Theme();
}

Theme Repository::defaultTheme(Repository::DefaultTheme t) const
{
    if (t == DarkTheme) {
        return theme(QLatin1String("Breeze Dark"));
    }
    return theme(QLatin1String("Breeze Light"));
}

Theme Repository::defaultTheme(Repository::DefaultTheme t)
{
    return std::as_const(*this).defaultTheme(t);
}

Theme Repository::themeForPalette(const QPalette &palette) const
{
    const auto base = palette.color(QPalette::Base);
    const auto themes = d->m_themes;

    // find themes with matching background colors
    QVector<KSyntaxHighlighting::Theme> matchingThemes;
    for (const auto &theme : themes) {
        const auto background = theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::BackgroundColor);
        if (background == base.rgb()) {
            matchingThemes.append(theme);
        }
    }
    if (!matchingThemes.empty()) {
        // if there's multiple, search for one with a matching highlight color
        const auto highlight = palette.color(QPalette::Highlight);
        for (const auto &theme : std::as_const(matchingThemes)) {
            auto selection = theme.editorColor(KSyntaxHighlighting::Theme::EditorColorRole::TextSelection);
            if (selection == highlight.rgb()) {
                return theme;
            }
        }
        return matchingThemes.first();
    }

    // fallback to just use the default light or dark theme
    return defaultTheme((base.lightness() < 128) ? KSyntaxHighlighting::Repository::DarkTheme : KSyntaxHighlighting::Repository::LightTheme);
}

Theme Repository::themeForPalette(const QPalette &palette)
{
    return std::as_const(*this).themeForPalette(palette);
}

void RepositoryPrivate::load(Repository *repo)
{
    // always add invalid default "None" highlighting
    addDefinition(Definition());

    // do lookup in standard paths, if not disabled
#ifndef NO_STANDARD_PATHS
    for (const auto &dir : QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                     QStringLiteral("org.kde.syntax-highlighting/syntax"),
                                                     QStandardPaths::LocateDirectory)) {
        loadSyntaxFolder(repo, dir);
    }

    // backward compatibility with Kate
    for (const auto &dir :
         QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("katepart5/syntax"), QStandardPaths::LocateDirectory)) {
        loadSyntaxFolder(repo, dir);
    }
#endif

    // default resources are always used, this is the one location that has a index cbor file
    loadSyntaxFolderFromIndex(repo, QStringLiteral(":/org.kde.syntax-highlighting/syntax"));

    // extra resources provided by 3rdparty libraries/applications
    loadSyntaxFolder(repo, QStringLiteral(":/org.kde.syntax-highlighting/syntax-addons"));

    // user given extra paths
    for (const auto &path : std::as_const(m_customSearchPaths)) {
        loadSyntaxFolder(repo, path + QStringLiteral("/syntax"));
    }

    m_sortedDefs.reserve(m_defs.size());
    for (auto it = m_defs.constBegin(); it != m_defs.constEnd(); ++it) {
        m_sortedDefs.push_back(it.value());
    }
    std::sort(m_sortedDefs.begin(), m_sortedDefs.end(), [](const Definition &left, const Definition &right) {
        auto comparison = left.translatedSection().compare(right.translatedSection(), Qt::CaseInsensitive);
        if (comparison == 0) {
            comparison = left.translatedName().compare(right.translatedName(), Qt::CaseInsensitive);
        }
        return comparison < 0;
    });

    // load themes

    // do lookup in standard paths, if not disabled
#ifndef NO_STANDARD_PATHS
    for (const auto &dir : QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                     QStringLiteral("org.kde.syntax-highlighting/themes"),
                                                     QStandardPaths::LocateDirectory)) {
        loadThemeFolder(dir);
    }
#endif

    // default resources are always used
    loadThemeFolder(QStringLiteral(":/org.kde.syntax-highlighting/themes"));

    // extra resources provided by 3rdparty libraries/applications
    loadThemeFolder(QStringLiteral(":/org.kde.syntax-highlighting/themes-addons"));

    // user given extra paths
    for (const auto &path : std::as_const(m_customSearchPaths)) {
        loadThemeFolder(path + QStringLiteral("/themes"));
    }
}

void RepositoryPrivate::loadSyntaxFolder(Repository *repo, const QString &path)
{
    QDirIterator it(path, QStringList() << QLatin1String("*.xml"), QDir::Files);
    while (it.hasNext()) {
        Definition def;
        auto defData = DefinitionData::get(def);
        defData->repo = repo;
        if (defData->loadMetaData(it.next())) {
            addDefinition(def);
        }
    }
}

void RepositoryPrivate::loadSyntaxFolderFromIndex(Repository *repo, const QString &path)
{
    QFile indexFile(path + QLatin1String("/index.katesyntax"));
    if (!indexFile.open(QFile::ReadOnly)) {
        return;
    }

    const auto indexDoc(QCborValue::fromCbor(indexFile.readAll()));
    const auto index = indexDoc.toMap();
    for (auto it = index.begin(); it != index.end(); ++it) {
        if (!it.value().isMap()) {
            continue;
        }
        const auto fileName = QString(path + QLatin1Char('/') + it.key().toString());
        const auto defMap = it.value().toMap();
        Definition def;
        auto defData = DefinitionData::get(def);
        defData->repo = repo;
        if (defData->loadMetaData(fileName, defMap)) {
            addDefinition(def);
        }
    }
}

void RepositoryPrivate::addDefinition(const Definition &def)
{
    const auto it = m_defs.constFind(def.name());
    if (it == m_defs.constEnd()) {
        m_defs.insert(def.name(), def);
        return;
    }

    if (it.value().version() >= def.version()) {
        return;
    }
    m_defs.insert(def.name(), def);
}

void RepositoryPrivate::loadThemeFolder(const QString &path)
{
    QDirIterator it(path, QStringList() << QLatin1String("*.theme"), QDir::Files);
    while (it.hasNext()) {
        auto themeData = std::unique_ptr<ThemeData>(new ThemeData);
        if (themeData->load(it.next())) {
            addTheme(Theme(themeData.release()));
        }
    }
}

static int themeRevision(const Theme &theme)
{
    auto data = ThemeData::get(theme);
    return data->revision();
}

void RepositoryPrivate::addTheme(const Theme &theme)
{
    const auto it = std::lower_bound(m_themes.begin(), m_themes.end(), theme, [](const Theme &lhs, const Theme &rhs) {
        return lhs.name() < rhs.name();
    });
    if (it == m_themes.end() || (*it).name() != theme.name()) {
        m_themes.insert(it, theme);
        return;
    }
    if (themeRevision(*it) < themeRevision(theme)) {
        *it = theme;
    }
}

quint16 RepositoryPrivate::foldingRegionId(const QString &defName, const QString &foldName)
{
    const auto it = m_foldingRegionIds.constFind(qMakePair(defName, foldName));
    if (it != m_foldingRegionIds.constEnd()) {
        return it.value();
    }
    m_foldingRegionIds.insert(qMakePair(defName, foldName), ++m_foldingRegionId);
    return m_foldingRegionId;
}

quint16 RepositoryPrivate::nextFormatId()
{
    Q_ASSERT(m_formatId < std::numeric_limits<quint16>::max());
    return ++m_formatId;
}

void Repository::reload()
{
    qCDebug(Log) << "Reloading syntax definitions!";
    for (const auto &def : std::as_const(d->m_sortedDefs)) {
        DefinitionData::get(def)->clear();
    }
    d->m_defs.clear();
    d->m_sortedDefs.clear();

    d->m_themes.clear();

    d->m_foldingRegionId = 0;
    d->m_foldingRegionIds.clear();

    d->m_formatId = 0;

    d->load(this);
}

void Repository::addCustomSearchPath(const QString &path)
{
    d->m_customSearchPaths.append(path);
    reload();
}

QVector<QString> Repository::customSearchPaths() const
{
    return d->m_customSearchPaths;
}
