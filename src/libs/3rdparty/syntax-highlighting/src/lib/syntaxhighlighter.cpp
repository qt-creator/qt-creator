/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "syntaxhighlighter.h"
#include "abstracthighlighter_p.h"
#include "definition.h"
#include "definition_p.h"
#include "foldingregion.h"
#include "format.h"
#include "format_p.h"
#include "state.h"
#include "theme.h"
#include "themedata_p.h"

Q_DECLARE_METATYPE(QTextBlock)

using namespace KSyntaxHighlighting;

namespace KSyntaxHighlighting
{
class TextBlockUserData : public QTextBlockUserData
{
public:
    State state;
    QList<FoldingRegion> foldingRegions;
};

class SyntaxHighlighterPrivate : public AbstractHighlighterPrivate
{
public:
    static FoldingRegion foldingRegion(const QTextBlock &startBlock);
    void initTextFormat(QTextCharFormat &tf, const Format &format);
    void computeTextFormats();

    struct TextFormat {
        QTextCharFormat tf;
        /**
         * id to check that the format belongs to the definition
         */
        std::intptr_t ptrId;
    };

    QList<FoldingRegion> foldingRegions;
    std::vector<TextFormat> tfs;
};

}

FoldingRegion SyntaxHighlighterPrivate::foldingRegion(const QTextBlock &startBlock)
{
    const auto data = dynamic_cast<TextBlockUserData *>(startBlock.userData());
    if (!data) {
        return FoldingRegion();
    }
    for (int i = data->foldingRegions.size() - 1; i >= 0; --i) {
        if (data->foldingRegions.at(i).type() == FoldingRegion::Begin) {
            return data->foldingRegions.at(i);
        }
    }
    return FoldingRegion();
}

void SyntaxHighlighterPrivate::initTextFormat(QTextCharFormat &tf, const Format &format)
{
    // always set the foreground color to avoid palette issues
    tf.setForeground(format.textColor(m_theme));

    if (format.hasBackgroundColor(m_theme)) {
        tf.setBackground(format.backgroundColor(m_theme));
    }
    if (format.isBold(m_theme)) {
        tf.setFontWeight(QFont::Bold);
    }
    if (format.isItalic(m_theme)) {
        tf.setFontItalic(true);
    }
    if (format.isUnderline(m_theme)) {
        tf.setFontUnderline(true);
    }
    if (format.isStrikeThrough(m_theme)) {
        tf.setFontStrikeOut(true);
    }
}

void SyntaxHighlighterPrivate::computeTextFormats()
{
    auto definitions = m_definition.includedDefinitions();
    definitions.append(m_definition);

    int maxId = 0;
    for (const auto &definition : std::as_const(definitions)) {
        for (const auto &format : std::as_const(DefinitionData::get(definition)->formats)) {
            maxId = qMax(maxId, format.id());
        }
    }
    tfs.clear();
    tfs.resize(maxId + 1);

    // initialize tfs
    for (const auto &definition : std::as_const(definitions)) {
        for (const auto &format : std::as_const(DefinitionData::get(definition)->formats)) {
            auto &tf = tfs[format.id()];
            tf.ptrId = FormatPrivate::ptrId(format);
            initTextFormat(tf.tf, format);
        }
    }
}

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
    , AbstractHighlighter(new SyntaxHighlighterPrivate)
{
    qRegisterMetaType<QTextBlock>();
}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
    , AbstractHighlighter(new SyntaxHighlighterPrivate)
{
    qRegisterMetaType<QTextBlock>();
}

SyntaxHighlighter::~SyntaxHighlighter()
{
}

void SyntaxHighlighter::setDefinition(const Definition &def)
{
    Q_D(SyntaxHighlighter);

    const auto needsRehighlight = d->m_definition != def;
    if (DefinitionData::get(d->m_definition) != DefinitionData::get(def)) {
        d->m_definition = def;
        d->tfs.clear();
    }
    if (needsRehighlight) {
        rehighlight();
    }
}

void SyntaxHighlighter::setTheme(const Theme &theme)
{
    Q_D(SyntaxHighlighter);
    if (ThemeData::get(d->m_theme) != ThemeData::get(theme)) {
        d->m_theme = theme;
        d->tfs.clear();
    }
}

bool SyntaxHighlighter::startsFoldingRegion(const QTextBlock &startBlock) const
{
    return SyntaxHighlighterPrivate::foldingRegion(startBlock).type() == FoldingRegion::Begin;
}

QTextBlock SyntaxHighlighter::findFoldingRegionEnd(const QTextBlock &startBlock) const
{
    const auto region = SyntaxHighlighterPrivate::foldingRegion(startBlock);

    auto block = startBlock;
    int depth = 1;
    while (block.isValid()) {
        block = block.next();
        const auto data = dynamic_cast<TextBlockUserData *>(block.userData());
        if (!data) {
            continue;
        }
        for (const auto &foldingRegion : std::as_const(data->foldingRegions)) {
            if (foldingRegion.id() != region.id()) {
                continue;
            }
            if (foldingRegion.type() == FoldingRegion::End) {
                --depth;
            } else if (foldingRegion.type() == FoldingRegion::Begin) {
                ++depth;
            }
            if (depth == 0) {
                return block;
            }
        }
    }

    return QTextBlock();
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    Q_D(SyntaxHighlighter);

    static const State emptyState;
    const State *previousState = &emptyState;
    if (currentBlock().position() > 0) {
        const auto prevBlock = currentBlock().previous();
        const auto prevData = dynamic_cast<TextBlockUserData *>(prevBlock.userData());
        if (prevData) {
            previousState = &prevData->state;
        }
    }
    d->foldingRegions.clear();
    auto newState = highlightLine(text, *previousState);

    auto data = dynamic_cast<TextBlockUserData *>(currentBlockUserData());
    if (!data) { // first time we highlight this
        data = new TextBlockUserData;
        data->state = std::move(newState);
        data->foldingRegions = d->foldingRegions;
        setCurrentBlockUserData(data);
        return;
    }

    if (data->state == newState && data->foldingRegions == d->foldingRegions) { // we ended up in the same state, so we are done here
        return;
    }
    data->state = std::move(newState);
    data->foldingRegions = d->foldingRegions;

    const auto nextBlock = currentBlock().next();
    if (nextBlock.isValid()) {
        QMetaObject::invokeMethod(this, "rehighlightBlock", Qt::QueuedConnection, Q_ARG(QTextBlock, nextBlock));
    }
}

void SyntaxHighlighter::applyFormat(int offset, int length, const Format &format)
{
    if (length == 0) {
        return;
    }

    Q_D(SyntaxHighlighter);

    if (Q_UNLIKELY(d->tfs.empty())) {
        d->computeTextFormats();
    }

    const auto id = static_cast<std::size_t>(format.id());
    // This doesn't happen when format comes from the definition.
    // But as the user can override the function to pass any format, this is a possible scenario.
    if (id < d->tfs.size() && d->tfs[id].ptrId == FormatPrivate::ptrId(format)) {
        QSyntaxHighlighter::setFormat(offset, length, d->tfs[id].tf);
    } else {
        QTextCharFormat tf;
        d->initTextFormat(tf, format);
        QSyntaxHighlighter::setFormat(offset, length, tf);
    }
}

void SyntaxHighlighter::applyFolding(int offset, int length, FoldingRegion region)
{
    Q_UNUSED(offset);
    Q_UNUSED(length);
    Q_D(SyntaxHighlighter);

    if (region.type() == FoldingRegion::Begin) {
        d->foldingRegions.push_back(region);
    }

    if (region.type() == FoldingRegion::End) {
        for (int i = d->foldingRegions.size() - 1; i >= 0; --i) {
            if (d->foldingRegions.at(i).id() != region.id() || d->foldingRegions.at(i).type() != FoldingRegion::Begin) {
                continue;
            }
            d->foldingRegions.remove(i);
            return;
        }
        d->foldingRegions.push_back(region);
    }
}

#include "moc_syntaxhighlighter.cpp"
