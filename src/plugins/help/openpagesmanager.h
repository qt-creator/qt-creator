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

#include <QObject>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QListView;
class QModelIndex;
class QPoint;
class QUrl;
class QWidget;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class HelpWidget;
class HelpViewer;
class OpenPagesSwitcher;
class OpenPagesWidget;

class OpenPagesManager : public QObject
{
    Q_OBJECT

public:
    OpenPagesManager(HelpWidget *helpWidget);
    ~OpenPagesManager() override;

    QWidget *openPagesWidget() const;
    QComboBox *openPagesComboBox() const;

    void setupInitialPages();

    void closeCurrentPage();
    void closePage(const QModelIndex &index);
    void closePagesExcept(const QModelIndex &index);

    void gotoNextPage();
    void gotoPreviousPage();

private:
    void removePage(int index);
    void showTwicherOrSelectPage() const;
    void openPagesContextMenu(const QPoint &point);

    QComboBox *m_comboBox = nullptr;
    HelpWidget *m_helpWidget = nullptr;
    mutable OpenPagesWidget *m_openPagesWidget = nullptr;
    OpenPagesSwitcher *m_openPagesSwitcher = nullptr;
};

} // namespace Internal
} // namespace Help
