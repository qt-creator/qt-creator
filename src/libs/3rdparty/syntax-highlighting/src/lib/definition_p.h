/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_DEFINITION_P_H
#define KSYNTAXHIGHLIGHTING_DEFINITION_P_H

#include "definition.h"
#include "highlightingdata_p.hpp"
#include "state.h"
#include "worddelimiters_p.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QVarLengthArray>

#include <vector>

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

    static DefinitionData *get(const Definition &def)
    {
        return def.d.get();
    }

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

    void resolveContexts();

    void resolveIncludeKeywords();

    KeywordList *keywordList(const QString &name);

    Context *initialContext();
    Context *contextByName(QStringView name);

    Format formatByName(QStringView name) const;

    quint16 foldingRegionId(const QString &foldName);

    struct ResolvedContext {
        DefinitionData *def;
        Context *context;
    };

    ResolvedContext resolveIncludedContext(QStringView defName, QStringView contextName);

    enum class FoldingRegionsState : uint8_t {
        Undetermined,
        ContainsFoldingRegions,
        NoFoldingRegions,
    };

    std::weak_ptr<DefinitionData> q;
    uint64_t id = 0;

    Repository *repo = nullptr;
    QHash<QString, KeywordList> keywordLists;
    std::vector<Context> contexts;
    QHash<QStringView, Format> formats;
    // data loaded from xml file and emptied after loading contexts
    std::vector<HighlightingContextData> contextDatas;
    // Definition referenced by IncludeRules and ContextSwitch
    QVarLengthArray<const DefinitionData *, 4> immediateIncludedDefinitions;
    WordDelimiters wordDelimiters;
    WordDelimiters wordWrapDelimiters;
    bool keywordIsLoaded = false;
    FoldingRegionsState foldingRegionsState = FoldingRegionsState::Undetermined;
    bool indentationBasedFolding = false;
    QStringList foldingIgnoreList;
    QString singleLineCommentMarker;
    CommentPosition singleLineCommentPosition = CommentPosition::StartOfLine;
    QString multiLineCommentStartMarker;
    QString multiLineCommentEndMarker;
    QList<QPair<QChar, QString>> characterEncodings;

    QString fileName;
    QString name = QStringLiteral(QT_TRANSLATE_NOOP("Language", "None"));
    QStringList alternativeNames;
    QByteArray nameUtf8;
    mutable QString translatedName;
    QString section;
    QByteArray sectionUtf8;
    mutable QString translatedSection;
    QString style;
    QString indenter;
    QString author;
    QString license;
    QList<QString> mimetypes;
    QList<QString> extensions;
    Qt::CaseSensitivity caseSensitive = Qt::CaseSensitive;
    int version = 0;
    int priority = 0;
    bool hidden = false;

    // cache that is used to unify states in AbstractHighlighter::highlightLine
    mutable QSet<State> unify;
};
}

#endif
