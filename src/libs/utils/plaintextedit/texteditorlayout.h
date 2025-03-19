// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintextedit.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT TextEditorLayout : public PlainTextDocumentLayout
{
public:
    explicit TextEditorLayout(PlainTextDocumentLayout *docLayout);

    void handleBlockVisibilityChanged(const QTextBlock &block);

    QTextLayout *blockLayout(const QTextBlock &block) const override;
    void clearBlockLayout(QTextBlock &block) const override;
    void clearBlockLayout(QTextBlock &start, QTextBlock &end, bool &blockVisibilityChanged) const override;
    void relayout() override;
    int additionalBlockHeight(const QTextBlock &block) const override;
    QRectF replacementBlockBoundingRect(const QTextBlock &block) const override;
    int lineSpacing() const override;
    int relativeLineSpacing() const override;

    void documentChanged(int from, int charsRemoved, int charsAdded) override;

    int blockLineCount(const QTextBlock &block) const override;
    void setBlockLineCount(QTextBlock &block, int lineCount) const override;

    void setBlockLayedOut(const QTextBlock &block) const override;

    int lineCount() const override;

    int firstLineNumberOf(const QTextBlock &block) const override;

    bool blockLayoutValid(int index) const;

    int blockHeight(const QTextBlock &block, int lineSpacing) const;

    int offsetForBlock(const QTextBlock &b) const;

    int offsetForLine(int line) const;

    int lineForOffset(int offset) const;

    int documentPixelHeight() const;

    QTextBlock findBlockByLineNumber(int lineNumber) const override;

    bool moveCursorImpl(
        QTextCursor &cursor,
        QTextCursor::MoveOperation operation,
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor) const;

    bool moveCursor(
        QTextCursor &cursor,
        QTextCursor::MoveOperation operation,
        QTextCursor::MoveMode mode = QTextCursor::MoveAnchor,
        int steps = 1) const override;

private:
    struct LayoutData
    {
        std::unique_ptr<QTextLayout> layout;
        int lineCount = 1;
        bool layedOut = false;

        void clearLayout()
        {
            if (layout)
                layout->clearLayout();
            lineCount = 1;
            layedOut = false;
        }
    };

    LayoutData &layoutData(int index) const;
    void resetOffsetCache(int blockNumber) const;
    void resizeOffsetCache(int blockNumber) const;

    struct OffsetData
    {
        int offset = -1;
        int firstLine = -1;
    };

    mutable std::vector<LayoutData> m_layoutData;
    mutable std::vector<OffsetData> m_offsetCache;
    bool updatingFormats = false;
};

} // namespace Utils
