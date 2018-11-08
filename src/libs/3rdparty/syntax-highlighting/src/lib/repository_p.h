/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef KSYNTAXHIGHLIGHTING_REPOSITORY_P_H
#define KSYNTAXHIGHLIGHTING_REPOSITORY_P_H

#include <QHash>
#include <QVector>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace KSyntaxHighlighting {

class Definition;
class Repository;
class Theme;

class RepositoryPrivate
{
public:
    RepositoryPrivate() = default;

    static RepositoryPrivate* get(Repository *repo);

    void load(Repository *repo);
    void loadSyntaxFolder(Repository *repo, const QString &path);
    bool loadSyntaxFolderFromIndex(Repository *repo, const QString &path);

    void addDefinition(const Definition &def);

    void loadThemeFolder(const QString &path);
    void addTheme(const Theme &theme);

    quint16 foldingRegionId(const QString &defName, const QString &foldName);
    quint16 nextFormatId();

    QVector<QString> m_customSearchPaths;

    QHash<QString, Definition> m_defs;
    QVector<Definition> m_sortedDefs;

    QVector<Theme> m_themes;

    QHash<QPair<QString, QString>, quint16> m_foldingRegionIds;
    quint16 m_foldingRegionId = 0;
    quint16 m_formatId = 0;
};
}

#endif
