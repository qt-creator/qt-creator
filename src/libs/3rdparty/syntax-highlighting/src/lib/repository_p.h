/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_REPOSITORY_P_H
#define KSYNTAXHIGHLIGHTING_REPOSITORY_P_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>

#include "dynamicregexpcache_p.h"

namespace KSyntaxHighlighting
{
class Definition;
class Repository;
class Theme;

class RepositoryPrivate
{
public:
    RepositoryPrivate() = default;

    static RepositoryPrivate *get(Repository *repo);

    void load(Repository *repo);
    void loadSyntaxFolder(Repository *repo, const QString &path);
    bool loadSyntaxFolderFromIndex(Repository *repo, const QString &path);

    void addDefinition(const Definition &def);

    void loadThemeFolder(const QString &path);
    void addTheme(const Theme &theme);

    int foldingRegionId(const QString &defName, const QString &foldName);
    int nextFormatId();

    QList<QString> m_customSearchPaths;

    // sorted map to have deterministic iteration order for e.g. definitionsForFileName
    QMap<QString, Definition> m_defs;

    // this vector is sorted by translated sections/names
    QList<Definition> m_sortedDefs;

    QList<Theme> m_themes;

    QHash<QPair<QString, QString>, int> m_foldingRegionIds;
    int m_foldingRegionId = 0;
    int m_formatId = 0;

    DynamicRegexpCache m_dynamicRegexpCache;
};
}

#endif
