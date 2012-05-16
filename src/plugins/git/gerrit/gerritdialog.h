/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITDIALOG_H
#define GERRIT_INTERNAL_GERRITDIALOG_H

#include <QDialog>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QTreeView;
class QLabel;
class QModelIndex;
class QSortFilterProxyModel;
class QStandardItem;
class QPushButton;
class QDialogButtonBox;
class QTextBrowser;
QT_END_NAMESPACE

namespace Utils {
class FilterLineEdit;
}
namespace Gerrit {
namespace Internal {
class GerritParameters;
class GerritModel;
class GerritChange;

class GerritDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GerritDialog(const QSharedPointer<GerritParameters> &p,
                          QWidget *parent = 0);
    ~GerritDialog();

signals:
    void fetchDisplay(const QSharedPointer<Gerrit::Internal::GerritChange> &);
    void fetchApply(const QSharedPointer<Gerrit::Internal::GerritChange> &);
    void fetchCheckout(const QSharedPointer<Gerrit::Internal::GerritChange> &);

private slots:
    void slotCurrentChanged();
    void slotDoubleClicked(const QModelIndex &);
    void slotRefreshStateChanged(bool);
    void slotFetchDisplay();
    void slotFetchApply();
    void slotFetchCheckout();
    void slotRefresh();

private:
    const QStandardItem *itemAt(const QModelIndex &i, int column = 0) const;
    const QStandardItem *currentItem(int column = 0) const;
    QPushButton *addActionButton(const QString &text, const char *buttonSlot);

    const QSharedPointer<GerritParameters> m_parameters;
    QSortFilterProxyModel *m_filterModel;
    GerritModel *m_model;
    QTreeView *m_treeView;
    QTextBrowser *m_detailsBrowser;
    Utils::FilterLineEdit *m_filterLineEdit;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_displayButton;
    QPushButton *m_applyButton;
    QPushButton *m_checkoutButton;
    QPushButton *m_refreshButton;
};

} // namespace Internal
} // namespace Gerrit

#endif // GERRIT_INTERNAL_GERRITDIALOG_H
