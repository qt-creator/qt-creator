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

#include "coreplugin/dialogs/ioptionspage.h"

#include <QList>
#include <QSet>
#include <QEventLoop>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QSortFilterProxyModel;
class QStackedLayout;
class QLabel;
class QListView;
QT_END_NAMESPACE

namespace Utils { class FancyLineEdit; }

namespace Core {
namespace Internal {

class Category;
class CategoryModel;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    // Returns a settings dialog. This makes sure that always only
    // a single settings dialog instance is running.
    // The dialog will be deleted automatically on close.
    static SettingsDialog *getSettingsDialog(QWidget *parent, Id initialPage);
    // Run the dialog and wait for it to finish.
    // Returns if the changes have been applied.
    bool execDialog();

    virtual QSize sizeHint() const;

public slots:
    void done(int);

private:
    SettingsDialog(QWidget *parent);
    ~SettingsDialog();

    void accept();
    void reject();
    void apply();
    void currentChanged(const QModelIndex &current);
    void currentTabChanged(int);
    void filter(const QString &text);

    void createGui();
    void showCategory(int index);
    void showPage(Id pageId);
    void updateEnabledTabs(Category *category, const QString &searchText);
    void ensureCategoryWidget(Category *category);
    void disconnectTabWidgets();

    const QList<IOptionsPage *> m_pages;

    QSet<IOptionsPage *> m_visitedPages;
    QSortFilterProxyModel *m_proxyModel;
    CategoryModel *m_model;
    Id m_currentCategory;
    Id m_currentPage;
    QStackedLayout *m_stackedLayout;
    Utils::FancyLineEdit *m_filterLineEdit;
    QListView *m_categoryList;
    QLabel *m_headerLabel;
    bool m_running;
    bool m_applied;
    bool m_finished;
    QList<QEventLoop *> m_eventLoops;
};

} // namespace Internal
} // namespace Core
