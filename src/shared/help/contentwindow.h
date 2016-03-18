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
    bool eventFilter(QObject *o, QEvent *e);

    Utils::NavigationTreeView *m_contentWidget;
    QHelpContentModel *m_contentModel;
    int m_expandDepth;
    bool m_isOpenInNewPageActionVisible;
};
