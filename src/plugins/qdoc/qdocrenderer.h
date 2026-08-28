// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qdocconfig.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include <functional>

namespace QDoc::Internal {

QString canonicalName(const QString &text);

QStringList nameKeys(const QString &canonical);

QStringList keysForTopic(const QString &command, const QString &arg);

class AnchorIndex
{
public:
    void add(const QString &key, const QString &id);
    void addAll(const QStringList &keys, const QString &id);

    QString lookup(const QString &target) const;

    bool isEmpty() const { return m_byKey.isEmpty(); }

private:
    QHash<QString, QString> m_byKey;
};

QString resolveLinks(const QString &html, const AnchorIndex &anchors);

class RenderContext
{
public:
    DocContext doc;
    QuoteCache *quoteCache = nullptr;

    bool showUnsupported = true;
    bool showTopicHeaders = true;
    // How the description of an image is shown to a sighted reader: caption, hover
    // or hidden. The alt attribute is always written.
    QString altTextDisplay = "caption";
};

class Heading
{
public:
    int level = 1;
    QString text;
    QString id;
};

class Renderer
{
public:
    explicit Renderer(const RenderContext &context = {});

    QString renderBlock(const ParsedDoc &doc, int index);

    QString resolveSamePageLinks(const QString &html) const;

    const QList<Problem> &problems() const { return m_problems; }
    const QList<Heading> &headings() const { return m_headings; }

private:
    void report(const QString &message, int offset, const QString &severity = QString("warning"));
    void checkBrief(const ParsedDoc &doc);
    void checkSeeAlso(const ParsedDoc &doc);
    void checkDocumentedParameters(const ParsedDoc &doc);

    QString renderTopicHeader(const ParsedDoc &doc);
    QString renderMetaBadges(const ParsedDoc &doc);

    QString blocks(const Nodes &nodes);
    QString block(const NodePtr &node);
    QString list(const NodePtr &node);
    QString table(const NodePtr &node);
    QString valueList(const NodePtr &node);
    QString codeBlock(const QString &text, const QString &lang, bool bad);
    QString codeGroup(const NodePtr &node);
    QString quoteBlock(const NodePtr &node);
    QuoteResult resolveQuotePart(const NodePtr &node);
    QString image(const NodePtr &node);
    QString rawHtml(const QString &html);
    QString badMarkup(const NodePtr &node, bool isInline);
    QString placeholder(const QString &command, const QString &detail);

    QString inlineNodes(const Nodes &nodes);
    QString inlineNode(const NodePtr &node);
    QString link(const QString &target, const QString &innerHtml, const QString &params = {});
    void noteTarget(const NodePtr &node);
    QString uniqueId(const QString &base);

    RenderContext m_context;
    QList<Problem> m_problems;
    QList<Heading> m_headings;
    QSet<QString> m_usedIds;
    AnchorIndex m_anchors;
    QStringList m_documentedParameters;
    QHash<QString, int> m_parameterOffsets;
    QHash<QString, int> m_seenTargets;
    QHash<int, QList<QList<QuoteStep>>> m_quoteSessions;
    QSet<QString> m_reportedQuoteFailures;
    QuoteCache m_ownQuoteCache;
    int m_currentBlockLine = 0;
};

QString escapeHtml(const QString &text);
QString escapeAttr(const QString &text);

QString highlight(const QString &text, const QString &lang);

// Where a rendered comment came from, so that the preview and the source can follow
// each other.
class PreviewBlock
{
public:
    int line = 0;
    int endLine = 0;
};

class PreviewPage
{
public:
    QString body;
    QList<Problem> problems;
    QList<Heading> headings;
    QList<PreviewBlock> blocks;
};

PreviewPage renderPreviewPage(const QString &text,
                              const Utils::FilePath &file,
                              const RenderContext &context = {});

} // namespace QDoc::Internal
