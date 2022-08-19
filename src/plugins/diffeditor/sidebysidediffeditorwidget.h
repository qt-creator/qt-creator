// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "diffeditorwidgetcontroller.h"

#include <QWidget>
#include <QTextCharFormat>

namespace Core { class IContext; }

namespace TextEditor {
class FontSettings;
class TextEditorWidget;
}

QT_BEGIN_NAMESPACE
class QMenu;
class QSplitter;
QT_END_NAMESPACE

namespace DiffEditor {

class FileData;

namespace Internal {

class DiffEditorDocument;
class SideDiffEditorWidget;

class SideBySideDiffEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SideBySideDiffEditorWidget(QWidget *parent = nullptr);

    TextEditor::TextEditorWidget *leftEditorWidget() const;
    TextEditor::TextEditorWidget *rightEditorWidget() const;

    void setDocument(DiffEditorDocument *document);
    DiffEditorDocument *diffDocument() const;

    void setDiff(const QList<FileData> &diffFileList);
    void setCurrentDiffFileIndex(int diffFileIndex);

    void setHorizontalSync(bool sync);

    void saveState();
    void restoreState();

    void clear(const QString &message = QString());

signals:
    void currentDiffFileIndexChanged(int index);

private:
    void setFontSettings(const TextEditor::FontSettings &fontSettings);
    void slotLeftJumpToOriginalFileRequested(int diffFileIndex,
                                             int lineNumber, int columnNumber);
    void slotRightJumpToOriginalFileRequested(int diffFileIndex,
                                              int lineNumber, int columnNumber);
    void slotLeftContextMenuRequested(QMenu *menu, int fileIndex,
                                      int chunkIndex, const ChunkSelection &selection);
    void slotRightContextMenuRequested(QMenu *menu, int fileIndex,
                                       int chunkIndex, const ChunkSelection &selection);
    void leftVSliderChanged();
    void rightVSliderChanged();
    void leftHSliderChanged();
    void rightHSliderChanged();
    void leftCursorPositionChanged();
    void rightCursorPositionChanged();
    void syncHorizontalScrollBarPolicy();
    void handlePositionChange(SideDiffEditorWidget *source, SideDiffEditorWidget *dest);
    void syncCursor(SideDiffEditorWidget *source, SideDiffEditorWidget *dest);

    void showDiff();

    SideDiffEditorWidget *m_leftEditor = nullptr;
    SideDiffEditorWidget *m_rightEditor = nullptr;
    QSplitter *m_splitter = nullptr;

    DiffEditorWidgetController m_controller;

    bool m_horizontalSync = false;

    QTextCharFormat m_spanLineFormat;
};

} // namespace Internal
} // namespace DiffEditor
