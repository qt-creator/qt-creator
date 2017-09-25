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

#include "diffeditorcontroller.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>
#include <utils/guard.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QToolBar;
class QStackedWidget;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }

namespace DiffEditor {

namespace Internal {
class DescriptionEditorWidget;
class DiffEditorDocument;
class IDiffView;
class UnifiedView;
class SideBySideView;

class DiffEditor : public Core::IEditor
{
    Q_OBJECT

public:
    DiffEditor(DiffEditorDocument *doc);
    ~DiffEditor() override;

    Core::IEditor *duplicate() override;
    Core::IDocument *document() override;
    QWidget *toolBar() override;
    TextEditor::TextEditorWidget *descriptionWidget() const;
    TextEditor::TextEditorWidget *unifiedEditorWidget() const;
    TextEditor::TextEditorWidget *leftEditorWidget() const;
    TextEditor::TextEditorWidget *rightEditorWidget() const;

private:
    DiffEditor();
    void setDocument(QSharedPointer<DiffEditorDocument> doc);

    void documentHasChanged();
    void toggleDescription();
    void updateDescription();
    void contextLineCountHasChanged(int lines);
    void ignoreWhitespaceHasChanged();
    void prepareForReload();
    void reloadHasFinished(bool success);
    void setCurrentDiffFileIndex(int index);
    void documentStateChanged();

    void toggleSync();

    IDiffView *loadSettings();
    void saveSetting(const QString &key, const QVariant &value) const;
    void updateEntryToolTip();
    void showDiffView(IDiffView *view);
    void updateDiffEditorSwitcher();
    void addView(IDiffView *view);
    IDiffView *currentView() const;
    void setCurrentView(IDiffView *view);
    IDiffView *nextView();
    void setupView(IDiffView *view);

    QSharedPointer<DiffEditorDocument> m_document;
    DescriptionEditorWidget *m_descriptionWidget;
    UnifiedView *m_unifiedView;
    SideBySideView *m_sideBySideView;
    QStackedWidget *m_stackedWidget;
    QVector<IDiffView *> m_views;
    QToolBar *m_toolBar;
    QComboBox *m_entriesComboBox;
    QSpinBox *m_contextSpinBox;
    QAction *m_contextSpinBoxAction = nullptr;
    QAction *m_toggleSyncAction;
    QAction *m_whitespaceButtonAction;
    QAction *m_toggleDescriptionAction;
    QAction *m_reloadAction;
    QAction *m_contextLabelAction = nullptr;
    QAction *m_viewSwitcherAction;
    QPair<QString, QString> m_currentFileChunk;
    int m_currentViewIndex;
    int m_currentDiffFileIndex;
    Utils::Guard m_ignoreChanges;
    bool m_sync;
    bool m_showDescription;
};

} // namespace Internal
} // namespace DiffEditor
