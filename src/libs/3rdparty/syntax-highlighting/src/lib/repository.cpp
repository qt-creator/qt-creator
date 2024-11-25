/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "repository.h"
#include "definition.h"
#include "definition_p.h"
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
Definition findHighestPriorityDefinitionIf(const QList<Definition> &defs, UnaryPredicate predicate)
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
QList<Definition> findDefinitionsIf(const QList<Definition> &defs, UnaryPredicate predicate)
{
    QList<Definition> matches;
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
    for (const auto &def : std::as_const(d->m_flatDefs)) {
        DefinitionData::get(def)->repo = nullptr;
    }
}

Definition Repository::definitionForName(const QString &defName) const
{
    return d->m_fullDefs.value(defName.toLower());
}

Definition Repository::definitionForFileName(const QString &fileName) const
{
    return findHighestPriorityDefinitionIf(d->m_flatDefs, anyWildcardMatches(fileNameFromFilePath(fileName)));
}

QList<Definition> Repository::definitionsForFileName(const QString &fileName) const
{
    return findDefinitionsIf(d->m_flatDefs, anyWildcardMatches(fileNameFromFilePath(fileName)));
}

Definition Repository::definitionForMimeType(const QString &mimeType) const
{
    return findHighestPriorityDefinitionIf(d->m_flatDefs, anyMimeTypeEquals(mimeType));
}

QList<Definition> Repository::definitionsForMimeType(const QString &mimeType) const
{
    return findDefinitionsIf(d->m_flatDefs, anyMimeTypeEquals(mimeType));
}

QList<Definition> Repository::definitions() const
{
    return d->m_sortedDefs;
}

QList<Theme> Repository::themes() const
{
    return d->m_themes;
}

static auto lowerBoundTheme(const QList<KSyntaxHighlighting::Theme> &themes, QStringView themeName)
{
    return std::lower_bound(themes.begin(), themes.end(), themeName, [](const Theme &lhs, QStringView rhs) {
        return lhs.name() < rhs;
    });
}

Theme Repository::theme(const QString &themeName) const
{
    const auto &themes = d->m_themes;
    const auto it = lowerBoundTheme(themes, themeName);
    if (it != themes.end() && (*it).name() == themeName) {
        return *it;
    }
    return Theme();
}

Theme Repository::defaultTheme(Repository::DefaultTheme t) const
{
    if (t == DarkTheme) {
        return theme(QStringLiteral("Breeze Dark"));
    }
    return theme(QStringLiteral("Breeze Light"));
}

Theme Repository::themeForPalette(const QPalette &palette) const
{
    const auto base = palette.color(QPalette::Base);
    const auto highlight = palette.color(QPalette::Highlight).rgb();

    // find themes with matching background and highlight colors
    const Theme *firstMatchingTheme = nullptr;
    for (const auto &theme : std::as_const(d->m_themes)) {
        const auto background = theme.editorColor(Theme::EditorColorRole::BackgroundColor);
        if (background == base.rgb()) {
            // find theme with a matching highlight color
            auto selection = theme.editorColor(Theme::EditorColorRole::TextSelection);
            if (selection == highlight) {
                return theme;
            }
            if (!firstMatchingTheme) {
                firstMatchingTheme = &theme;
            }
        }
    }
    if (firstMatchingTheme) {
        return *firstMatchingTheme;
    }

    // fallback to just use the default light or dark theme
    return defaultTheme((base.lightness() < 128) ? Repository::DarkTheme : Repository::LightTheme);
}

void RepositoryPrivate::load(Repository *repo)
{
    // always add invalid default "None" highlighting
    m_defs.emplace(QString(), Definition());

    // do lookup in standard paths, if not disabled
#ifndef NO_STANDARD_PATHS
    // do lookup in installed path when has no syntax resource
#ifndef HAS_SYNTAX_RESOURCE
    for (const auto &dir : QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                     QStringLiteral("org.kde.syntax-highlighting/syntax-bundled"),
                                                     QStandardPaths::LocateDirectory)) {
        if (!loadSyntaxFolderFromIndex(repo, dir)) {
            loadSyntaxFolder(repo, dir);
        }
    }
#endif

    for (const auto &dir : QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                     QStringLiteral("org.kde.syntax-highlighting/syntax"),
                                                     QStandardPaths::LocateDirectory)) {
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

    computeAlternativeDefLists();

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

void RepositoryPrivate::computeAlternativeDefLists()
{
    m_flatDefs.clear();
    m_flatDefs.reserve(m_defs.size());
    for (const auto &[_, def] : m_defs) {
        m_flatDefs.push_back(def);
    }

    m_sortedDefs = m_flatDefs;
    std::sort(m_sortedDefs.begin(), m_sortedDefs.end(), [](const Definition &left, const Definition &right) {
        auto comparison = left.translatedSection().compare(right.translatedSection(), Qt::CaseInsensitive);
        if (comparison == 0) {
            comparison = left.translatedName().compare(right.translatedName(), Qt::CaseInsensitive);
        }
        return comparison < 0;
    });

    m_fullDefs.clear();
    for (const auto &def : std::as_const(m_sortedDefs)) {
        m_fullDefs.insert(def.name().toLower(), def);
        const auto &alternativeNames = def.alternativeNames();
        for (const auto &altName : alternativeNames) {
            m_fullDefs.insert(altName.toLower(), def);
        }
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
            addDefinition(std::move(def));
        }
    }
}

bool RepositoryPrivate::loadSyntaxFolderFromIndex(Repository *repo, const QString &path)
{
    QFile indexFile(path + QLatin1String("/index.katesyntax"));
    if (!indexFile.open(QFile::ReadOnly)) {
        return false;
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
            addDefinition(std::move(def));
        }
    }

    return true;
}

void RepositoryPrivate::addDefinition(Definition &&def)
{
    const auto [it, inserted] = m_defs.try_emplace(def.name(), std::move(def));
    if (inserted) {
        return;
    }

    if (it->second.version() >= def.version()) {
        return;
    }
    it->second = std::move(def);
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
    const auto &constThemes = m_themes;
    const auto themeName = theme.name();
    const auto constIt = lowerBoundTheme(constThemes, themeName);
    const auto index = constIt - constThemes.begin();
    if (constIt == constThemes.end() || (*constIt).name() != themeName) {
        m_themes.insert(index, theme);
        return;
    }
    if (themeRevision(*constIt) < themeRevision(theme)) {
        m_themes[index] = theme;
    }
}

int RepositoryPrivate::foldingRegionId(const QString &defName, const QString &foldName)
{
    const auto it = m_foldingRegionIds.constFind(qMakePair(defName, foldName));
    if (it != m_foldingRegionIds.constEnd()) {
        return it.value();
    }
    Q_ASSERT(m_foldingRegionId < std::numeric_limits<int>::max());
    m_foldingRegionIds.insert(qMakePair(defName, foldName), ++m_foldingRegionId);
    return m_foldingRegionId;
}

int RepositoryPrivate::nextFormatId()
{
    Q_ASSERT(m_formatId < std::numeric_limits<int>::max());
    return ++m_formatId;
}

void Repository::reload()
{
    Q_EMIT aboutToReload();

    for (const auto &def : std::as_const(d->m_flatDefs)) {
        DefinitionData::get(def)->clear();
    }
    d->m_defs.clear();
    d->m_flatDefs.clear();
    d->m_sortedDefs.clear();
    d->m_fullDefs.clear();

    d->m_themes.clear();

    d->m_foldingRegionId = 0;
    d->m_foldingRegionIds.clear();

    d->m_formatId = 0;

    d->load(this);

    Q_EMIT reloaded();
}

void Repository::addCustomSearchPath(const QString &path)
{
    Q_EMIT aboutToReload();

    d->m_customSearchPaths.append(path);
    d->loadThemeFolder(path + QStringLiteral("/themes"));
    d->loadSyntaxFolder(this, path + QStringLiteral("/syntax"));
    d->computeAlternativeDefLists();

    Q_EMIT reloaded();
}

QList<QString> Repository::customSearchPaths() const
{
    return d->m_customSearchPaths;
}

#include "moc_repository.cpp"
