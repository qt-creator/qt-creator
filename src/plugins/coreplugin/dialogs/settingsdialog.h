/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "coreplugin/dialogs/ioptionspage.h"

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QPointer>
#include <QtCore/QEventLoop>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QSortFilterProxyModel;
class QStackedLayout;
class QLabel;
class QListView;
QT_END_NAMESPACE

namespace Utils {
    class FilterLineEdit;
}

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
    static SettingsDialog *getSettingsDialog(QWidget *parent,
                               const QString &initialCategory = QString(),
                               const QString &initialPage = QString());
    // Run the dialog and wait for it to finish.
    // Returns if the changes have been applied.
    bool execDialog();

    virtual QSize sizeHint() const;

public slots:
    void done(int);

private slots:
    void accept();
    void reject();
    void apply();
    void currentChanged(const QModelIndex &current);
    void currentTabChanged(int);
    void filter(const QString &text);

private:
    SettingsDialog(QWidget *parent);
    ~SettingsDialog();

    void createGui();
    void showCategory(int index);
    void showPage(const QString &categoryId, const QString &pageId);
    void updateEnabledTabs(Category *category, const QString &searchText);
    void ensureCategoryWidget(Category *category);
    void ensureAllCategoryWidgets();
    void disconnectTabWidgets();

    const QList<Core::IOptionsPage*> m_pages;

    QSet<Core::IOptionsPage*> m_visitedPages;
    QSortFilterProxyModel *m_proxyModel;
    CategoryModel *m_model;
    QString m_currentCategory;
    QString m_currentPage;
    QStackedLayout *m_stackedLayout;
    Utils::FilterLineEdit *m_filterLineEdit;
    QListView *m_categoryList;
    QLabel *m_headerLabel;
    bool m_running;
    bool m_applied;
    QList<QEventLoop *> m_eventLoops;
    static QPointer<SettingsDialog> m_instance;
};

} // namespace Internal
} // namespace Core

#endif // SETTINGSDIALOG_H
