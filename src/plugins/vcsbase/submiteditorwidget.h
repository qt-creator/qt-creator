/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SUBMITEDITORWIDGET_H
#define SUBMITEDITORWIDGET_H

#include "vcsbase_global.h"

#include <utils/completingtextedit.h>

#include <QWidget>
#include <QAbstractItemView>

QT_BEGIN_NAMESPACE
class QAction;
class QModelIndex;
QT_END_NAMESPACE

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
    explicit SubmitEditorWidget(QWidget *parent = 0);
    virtual ~SubmitEditorWidget();

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

signals:
    void diffSelected(const QList<int> &);
    void fileSelectionChanged(bool someFileSelected);
    void submitActionTextChanged(const QString &);
    void submitActionEnabledChanged(bool);

private slots:
    void updateCheckAllComboBox();
    void checkAllToggled();
    void checkAll();
    void uncheckAll();

protected:
    virtual QString cleanupDescription(const QString &) const;
    virtual void changeEvent(QEvent *e);
    virtual QString commitName() const;
    void insertTopWidget(QWidget *w);

protected slots:
    void descriptionTextChanged();
    void updateSubmitAction();

private slots:
    void triggerDiffSelected();
    void diffActivated(const QModelIndex &index);
    void diffActivatedDelayed();
    void updateActions();
    void updateDiffAction();
    void editorCustomContextMenuRequested(const QPoint &);
    void fileListCustomContextMenuRequested(const QPoint & pos);

private:
    bool hasSelection() const;
    int checkedFilesCount() const;

    SubmitEditorWidgetPrivate *d;
};

} // namespace VcsBase

#endif // SUBMITEDITORWIDGET_H
