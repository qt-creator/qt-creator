// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Help {
    namespace Internal {

class OpenPagesWidget;

class OpenPagesSwitcher : public QFrame
{
    Q_OBJECT

public:
    OpenPagesSwitcher(QAbstractItemModel *model);
    ~OpenPagesSwitcher() override;

    void gotoNextPage();
    void gotoPreviousPage();

    void selectAndHide();
    void selectCurrentPage(int index);

    void setVisible(bool visible) override;
    void focusInEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void closePage(const QModelIndex &index);
    void setCurrentPage(const QModelIndex &index);

private:
    void selectPageUpDown(int summand);

private:
    QAbstractItemModel *m_openPagesModel = nullptr;
    OpenPagesWidget *m_openPagesWidget = nullptr;
};

    }   // namespace Internal
}   // namespace Help
