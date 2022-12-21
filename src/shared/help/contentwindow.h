// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QUrl>
#include <QModelIndex>
#include <QWidget>

QT_BEGIN_NAMESPACE

class QHelpContentItem;
class QHelpContentModel;

QT_END_NAMESPACE

namespace Utils {
class NavigationTreeView;
}

class ContentWindow : public QWidget
{
    Q_OBJECT

public:
    ContentWindow();
    ~ContentWindow();

    void setOpenInNewPageActionVisible(bool visible);

signals:
    void linkActivated(const QUrl &link, bool newPage);

private:
    void showContextMenu(const QPoint &pos);
    void expandTOC();
    void itemActivated(const QModelIndex &index);
    void expandToDepth(int depth);
    bool eventFilter(QObject *o, QEvent *e) override;

    Utils::NavigationTreeView *m_contentWidget;
    QHelpContentModel *m_contentModel;
    int m_expandDepth;
    bool m_isOpenInNewPageActionVisible;
};
