/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_REPOSITORY_P_H
#define KSYNTAXHIGHLIGHTING_REPOSITORY_P_H

#include <QHash>
#include <QVector>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

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

    quint16 foldingRegionId(const QString &defName, const QString &foldName);
    quint16 nextFormatId();

    QVector<QString> m_customSearchPaths;

    // sorted map to have deterministic iteration order for e.g. definitionsForFileName
    QMap<QString, Definition> m_defs;

    // this vector is sorted by translated sections/names
    QVector<Definition> m_sortedDefs;

    QVector<Theme> m_themes;

    QHash<QPair<QString, QString>, quint16> m_foldingRegionIds;
    quint16 m_foldingRegionId = 0;
    quint16 m_formatId = 0;
};
}

#endif
