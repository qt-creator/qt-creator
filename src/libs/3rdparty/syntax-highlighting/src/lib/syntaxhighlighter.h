/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_QSYNTAXHIGHLIGHTER_H
#define KSYNTAXHIGHLIGHTING_QSYNTAXHIGHLIGHTER_H

#include "ksyntaxhighlighting_export.h"

#include "abstracthighlighter.h"

#include <QSyntaxHighlighter>

namespace KSyntaxHighlighting
{
class SyntaxHighlighterPrivate;

/** A QSyntaxHighlighter implementation for use with QTextDocument.
 *  This supports partial re-highlighting during editing and
 *  tracks syntax-based code folding regions.
 *
 *  @since 5.28
 */
class KSYNTAXHIGHLIGHTING_EXPORT SyntaxHighlighter : public QSyntaxHighlighter, public AbstractHighlighter
{
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QObject *parent = nullptr);
    explicit SyntaxHighlighter(QTextDocument *document);
    ~SyntaxHighlighter() override;

    void setDefinition(const Definition &def) override;

    /** Returns whether there is a folding region beginning at @p startBlock.
     *  This only considers syntax-based folding regions,
     *  not indention-based ones as e.g. found in Python.
     *
     *  @see findFoldingRegionEnd
     */
    bool startsFoldingRegion(const QTextBlock &startBlock) const;

    /** Finds the end of the folding region starting at @p startBlock.
     *  If multiple folding regions begin at @p startBlock, the end of
     *  the last/innermost one is returned.
     *  This returns an invalid block if no folding region end is found,
     *  which typically indicates an unterminated region and thus folding
     *  until the document end.
     *  This method performs a sequential search starting at @p startBlock
     *  for the matching folding region end, which is a potentially expensive
     *  operation.
     *
     *  @see startsFoldingRegion
     */
    QTextBlock findFoldingRegionEnd(const QTextBlock &startBlock) const;

protected:
    void highlightBlock(const QString &text) override;
    void applyFormat(int offset, int length, const Format &format) override;
    void applyFolding(int offset, int length, FoldingRegion region) override;

private:
    Q_DECLARE_PRIVATE_D(AbstractHighlighter::d_ptr, SyntaxHighlighter)
};
}

#endif // KSYNTAXHIGHLIGHTING_QSYNTAXHIGHLIGHTER_H
