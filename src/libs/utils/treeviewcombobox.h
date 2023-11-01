// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QComboBox>
#include <QTreeView>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TreeViewComboBoxView : public QTreeView
{
public:
    TreeViewComboBoxView(QWidget *parent = nullptr);
    void adjustWidth(int width);
};

class QTCREATOR_UTILS_EXPORT TreeViewComboBox : public QComboBox
{
public:
    TreeViewComboBox(QWidget *parent = nullptr);

    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void setCurrentIndex(const QModelIndex &index);
    bool eventFilter(QObject* object, QEvent* event) override;
    void showPopup() override;
    void hidePopup() override;

    TreeViewComboBoxView *view() const;

private:
    QModelIndex indexBelow(QModelIndex index);
    QModelIndex indexAbove(QModelIndex index);
    QModelIndex lastIndex(const QModelIndex &index);

    TreeViewComboBoxView *m_view;
    bool m_skipNextHide = false;
};
}
