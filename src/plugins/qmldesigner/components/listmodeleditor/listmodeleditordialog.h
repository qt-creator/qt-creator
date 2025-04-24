// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QTableView;
class QModelIndex;
QT_END_NAMESPACE

namespace Ui {
class ListModelEditorDialog;
}

namespace QmlDesigner {

class ListModelEditorModel;

class ListModelEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ListModelEditorDialog(QWidget *parent = nullptr);
    ~ListModelEditorDialog();

    void setModel(ListModelEditorModel *model);

protected:
    void keyPressEvent(QKeyEvent *) override;

private:
    void addRowAbove();
    void addRowBelow();
    void openColumnDialog();
    void removeRows();
    void removeColumns();
    void changeHeader(int column);
    void moveRowsDown();
    void moveRowsUp();
    void updateSelection();

    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onColumnsInserted(const QModelIndex &parent, int first, int last);

private:
    ListModelEditorModel *m_model{};
    QAction *m_addRowBelowAction{};
    QAction *m_addRowAboveAction{};
    QAction *m_removeRowsAction{};
    QAction *m_addColumnAction{};
    QAction *m_removeColumnsAction{};
    QAction *m_moveUpAction{};
    QAction *m_moveDownAction{};
    QTableView *m_tableView{};
};

} // namespace QmlDesigner
