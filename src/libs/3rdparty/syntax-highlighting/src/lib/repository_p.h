/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_REPOSITORY_P_H
#define KSYNTAXHIGHLIGHTING_REPOSITORY_P_H

#include <QHash>
#include <QList>
#include <QString>

#include <map>

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
    void computeAlternativeDefLists();

    void addDefinition(Definition &&def);

    void loadThemeFolder(const QString &path);
    void addTheme(const Theme &theme);

    int foldingRegionId(const QString &defName, const QString &foldName);
    int nextFormatId();

    QList<QString> m_customSearchPaths;

    // sorted map to have deterministic iteration order
    std::map<QString, Definition> m_defs;
    // flat version of m_defs for speed up iterations for e.g. definitionsForFileName
    QList<Definition> m_flatDefs;

    // map relating all names and alternative names, case insensitively to the correct definition.
    QHash<QString, Definition> m_fullDefs;

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
