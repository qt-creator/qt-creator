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

#include "vcsbase_global.h"

#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QAction;
class QModelIndex;
QT_END_NAMESPACE

namespace Utils { class CompletingTextEdit; }
namespace VcsBase {

class SubmitFieldWidget;
struct SubmitEditorWidgetPrivate;
class SubmitFileModel;

class VCSBASE_EXPORT SubmitEditorWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString descriptionText READ descriptionText WRITE setDescriptionText DESIGNABLE true)
    Q_PROPERTY(QAbstractItemView::SelectionMode fileListSelectionMode READ fileListSelectionMode WRITE setFileListSelectionMode DESIGNABLE true)
    Q_PROPERTY(bool lineWrap READ lineWrap WRITE setLineWrap DESIGNABLE true)
    Q_PROPERTY(int lineWrapWidth READ lineWrapWidth WRITE setLineWrapWidth DESIGNABLE true)
    Q_PROPERTY(bool descriptionMandatory READ isDescriptionMandatory WRITE setDescriptionMandatory DESIGNABLE false)
    Q_PROPERTY(bool emptyFileListEnabled READ isEmptyFileListEnabled WRITE setEmptyFileListEnabled DESIGNABLE true)

public:
    SubmitEditorWidget();
    ~SubmitEditorWidget() override;

    // Register/Unregister actions that are managed by ActionManager with this widget.
    // The submit action should have Core::Command::CA_UpdateText set as its text will
    // be updated.
    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

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

    virtual bool canSubmit() const;
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
    virtual QString cleanupDescription(const QString &) const;
    virtual QString commitName() const;
    void insertTopWidget(QWidget *w);
    void insertLeftWidget(QWidget *w);
    void addSubmitButtonMenu(QMenu *menu);
    void hideDescription();

protected slots:
    void descriptionTextChanged();

private:
    void updateCheckAllComboBox();
    void checkAllToggled();
    void checkAll();
    void uncheckAll();

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
