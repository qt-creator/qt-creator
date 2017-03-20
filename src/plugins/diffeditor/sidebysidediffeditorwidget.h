/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "diffeditorwidgetcontroller.h"

#include <QWidget>
#include <QTextCharFormat>

namespace TextEditor { class FontSettings; }

QT_BEGIN_NAMESPACE
class QMenu;
class QSplitter;
QT_END_NAMESPACE

namespace DiffEditor {

class DiffEditorController;
class FileData;

namespace Internal {

class DiffEditorDocument;
class SideDiffEditorWidget;

class SideBySideDiffEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SideBySideDiffEditorWidget(QWidget *parent = 0);

    void setDocument(DiffEditorDocument *document);
    DiffEditorDocument *diffDocument() const;

    void setDiff(const QList<FileData> &diffFileList,
                 const QString &workingDirectory);
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
    void slotLeftContextMenuRequested(QMenu *menu, int diffFileIndex,
                                      int chunkIndex);
    void slotRightContextMenuRequested(QMenu *menu, int diffFileIndex,
                                       int chunkIndex);
    void leftVSliderChanged();
    void rightVSliderChanged();
    void leftHSliderChanged();
    void rightHSliderChanged();
    void leftCursorPositionChanged();
    void rightCursorPositionChanged();

    void showDiff();

    SideDiffEditorWidget *m_leftEditor;
    SideDiffEditorWidget *m_rightEditor;
    QSplitter *m_splitter;

    DiffEditorWidgetController m_controller;

    bool m_horizontalSync = false;

    QTextCharFormat m_spanLineFormat;
};

} // namespace Internal
} // namespace DiffEditor
