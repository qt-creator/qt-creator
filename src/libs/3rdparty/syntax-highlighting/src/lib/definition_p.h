/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_DEFINITION_P_H
#define KSYNTAXHIGHLIGHTING_DEFINITION_P_H

#include "definition.h"
#include "definitionref_p.h"
#include "worddelimiters_p.h"

#include <QHash>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
class QCborMap;
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class Repository;

class DefinitionData
{
public:
    DefinitionData();
    ~DefinitionData();

    DefinitionData(const DefinitionData &) = delete;
    DefinitionData &operator=(const DefinitionData &) = delete;

    static DefinitionData *get(const Definition &def);

    bool isLoaded() const;
    bool loadMetaData(const QString &definitionFileName);
    bool loadMetaData(const QString &fileName, const QCborMap &obj);

    void clear();

    enum class OnlyKeywords : bool;

    bool load(OnlyKeywords onlyKeywords = OnlyKeywords(false));
    bool loadLanguage(QXmlStreamReader &reader);
    void loadHighlighting(QXmlStreamReader &reader, OnlyKeywords onlyKeywords);
    void loadContexts(QXmlStreamReader &reader);
    void loadItemData(QXmlStreamReader &reader);
    void loadGeneral(QXmlStreamReader &reader);
    void loadComments(QXmlStreamReader &reader);
    void loadFoldingIgnoreList(QXmlStreamReader &reader);
    void loadSpellchecking(QXmlStreamReader &reader);
    bool checkKateVersion(QStringView verStr);

    void resolveIncludeKeywords();

    KeywordList *keywordList(const QString &name);

    Context *initialContext() const;
    Context *contextByName(const QString &name) const;

    Format formatByName(const QString &name) const;

    quint16 foldingRegionId(const QString &foldName);

    DefinitionRef q;

    Repository *repo = nullptr;
    QHash<QString, KeywordList> keywordLists;
    QVector<Context *> contexts;
    QHash<QString, Format> formats;
    WordDelimiters wordDelimiters;
    WordDelimiters wordWrapDelimiters;
    bool keywordIsLoaded = false;
    bool hasFoldingRegions = false;
    bool indentationBasedFolding = false;
    QStringList foldingIgnoreList;
    QString singleLineCommentMarker;
    CommentPosition singleLineCommentPosition = CommentPosition::StartOfLine;
    QString multiLineCommentStartMarker;
    QString multiLineCommentEndMarker;
    QVector<QPair<QChar, QString>> characterEncodings;

    QString fileName;
    QString name = QStringLiteral(QT_TRANSLATE_NOOP("Language", "None"));
    QString section;
    QString style;
    QString indenter;
    QString author;
    QString license;
    QVector<QString> mimetypes;
    QVector<QString> extensions;
    Qt::CaseSensitivity caseSensitive = Qt::CaseSensitive;
    int version = 0;
    int priority = 0;
    bool hidden = false;
};
}

#endif
