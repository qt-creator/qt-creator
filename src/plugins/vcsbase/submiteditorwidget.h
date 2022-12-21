// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QAction;
class QModelIndex;
QT_END_NAMESPACE

namespace Utils { class CompletingTextEdit; }
namespace VcsBase {

struct SubmitEditorWidgetPrivate;
class SubmitFieldWidget;
class SubmitFileModel;

class VCSBASE_EXPORT SubmitEditorWidget : public QWidget
{
    Q_OBJECT

public:
    SubmitEditorWidget();
    ~SubmitEditorWidget() override;

    // Register/Unregister actions that are managed by ActionManager with this widget.
    // The submit action should have Core::Command::CA_UpdateText set as its text will
    // be updated.
    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = nullptr, QAction *diffAction = nullptr);

    QString descriptionText() const;
    void setDescriptionText(const QString &text);

    // 'Commit' action enabled despite empty file list
    bool isEmptyFileListEnabled() const;
    void setEmptyFileListEnabled(bool e);

    bool lineWrap() const;
    void setLineWrap(bool);

    int lineWrapWidth() const;
    void setLineWrapWidth(int);

    bool isDescriptionMandatory() const;
    void setDescriptionMandatory(bool);

    QAbstractItemView::SelectionMode fileListSelectionMode() const;
    void setFileListSelectionMode(QAbstractItemView::SelectionMode sm);

    void setFileModel(SubmitFileModel *model);
    SubmitFileModel *fileModel() const;

    // Files to be included in submit
    QStringList checkedFiles() const;

    Utils::CompletingTextEdit *descriptionEdit() const;

    void addDescriptionEditContextMenuAction(QAction *a);
    void insertDescriptionEditContextMenuAction(int pos, QAction *a);

    void addSubmitFieldWidget(SubmitFieldWidget *f);
    QList<SubmitFieldWidget *> submitFieldWidgets() const;

    virtual bool canSubmit(QString *whyNot = nullptr) const;
    bool isEdited() const;
    void setUpdateInProgress(bool value);
    bool updateInProgress() const;

    QList<int> selectedRows() const;
    void setSelectedRows(const QList<int> &rows);

public slots:
    void updateSubmitAction();

signals:
    void diffSelected(const QList<int> &);
    void fileSelectionChanged(bool someFileSelected);
    void submitActionTextChanged(const QString &);
    void submitActionEnabledChanged(bool);

protected:
    virtual void changeEvent(QEvent *event) override;
    virtual QString cleanupDescription(const QString &) const;
    virtual QString commitName() const;
    void insertTopWidget(QWidget *w);
    void insertLeftWidget(QWidget *w);
    void addSubmitButtonMenu(QMenu *menu);
    void hideDescription();
    void descriptionTextChanged();
    void verifyDescription();

private:
    void updateCheckAllComboBox();
    void checkAllToggled();

    void triggerDiffSelected();
    void diffActivated(const QModelIndex &index);
    void diffActivatedDelayed();
    void updateActions();
    void updateDiffAction();
    void editorCustomContextMenuRequested(const QPoint &);
    void fileListCustomContextMenuRequested(const QPoint & pos);

    bool hasSelection() const;
    int checkedFilesCount() const;
    void wrapDescription();
    void trimDescription();

    SubmitEditorWidgetPrivate *d;
};

} // namespace VcsBase
