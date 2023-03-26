// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QTableView)

namespace QmlDesigner {

class EventList;
class EventListDelegate;
class RewriterView;
class NotIndentingTextEditModifier;

class EventListDialog : public QDialog
{
    Q_OBJECT

public:
    EventListDialog(QWidget *parent = nullptr);
    void initialize(EventList &events);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    EventListDelegate *m_delegate;
    NotIndentingTextEditModifier *m_modifier;
    RewriterView *m_rewriter;
    QTableView *m_table;
    QAction *m_addAction;
    QAction *m_removeAction;
    QPlainTextEdit *m_textEdit;
};

} // namespace QmlDesigner.
