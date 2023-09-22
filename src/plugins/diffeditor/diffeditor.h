// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "diffenums.h"

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

namespace DiffEditor::Internal {

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
    Core::IDocument *document() const override;
    QWidget *toolBar() override;
    TextEditor::TextEditorWidget *descriptionWidget() const;
    TextEditor::TextEditorWidget *unifiedEditorWidget() const;
    TextEditor::TextEditorWidget *sideEditorWidget(DiffSide side) const;

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
    void currentIndexChanged(int index);
    void setCurrentDiffFileIndex(int index);
    void documentStateChanged();

    void toggleSync();

    IDiffView *loadSettings();
    void saveSetting(const Utils::Key &key, const QVariant &value) const;
    void updateEntryToolTip();
    void showDiffView(IDiffView *view);
    void updateDiffEditorSwitcher();
    void addView(IDiffView *view);
    IDiffView *currentView() const;
    void setCurrentView(IDiffView *view);
    IDiffView *nextView();
    void setupView(IDiffView *view);

    QSharedPointer<DiffEditorDocument> m_document;
    DescriptionEditorWidget *m_descriptionWidget = nullptr;
    UnifiedView *m_unifiedView = nullptr;
    SideBySideView *m_sideBySideView = nullptr;
    QStackedWidget *m_stackedWidget = nullptr;
    QList<IDiffView *> m_views;
    QToolBar *m_toolBar = nullptr;
    QComboBox *m_entriesComboBox = nullptr;
    QSpinBox *m_contextSpinBox = nullptr;
    QAction *m_contextSpinBoxAction = nullptr;
    QAction *m_toggleSyncAction;
    QAction *m_whitespaceButtonAction;
    QAction *m_toggleDescriptionAction;
    QAction *m_reloadAction;
    QAction *m_contextLabelAction = nullptr;
    QAction *m_viewSwitcherAction;
    QPair<QString, QString> m_currentFileChunk;
    int m_currentViewIndex = -1;
    int m_currentDiffFileIndex = -1;
    int m_descriptionHeight = 8;
    Utils::Guard m_ignoreChanges;
    bool m_sync = false;
    bool m_showDescription = true;
};

} // namespace DiffEditor::Internal
