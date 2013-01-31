/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GERRIT_INTERNAL_GERRITDIALOG_H
#define GERRIT_INTERNAL_GERRITDIALOG_H

#include <utils/filterlineedit.h>

#include <QDialog>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QTreeView;
class QLabel;
class QModelIndex;
class QSortFilterProxyModel;
class QStandardItem;
class QStringListModel;
class QPushButton;
class QDialogButtonBox;
class QTextBrowser;
QT_END_NAMESPACE

namespace Gerrit {
namespace Internal {
class GerritParameters;
class GerritModel;
class GerritChange;

class QueryValidatingLineEdit : public Utils::FilterLineEdit
{
    Q_OBJECT

public:
    explicit QueryValidatingLineEdit(QWidget *parent = 0);
    void setTextColor(const QColor &c);

public slots:
    void setValid();
    void setInvalid();

private:
    bool m_valid;
    const QColor m_okTextColor;
    const QColor m_errorTextColor;
};

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

public slots:
    void fetchStarted(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchFinished();

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
    void updateCompletions(const QString &query);
    void updateButtons();

    const QSharedPointer<GerritParameters> m_parameters;
    QSortFilterProxyModel *m_filterModel;
    GerritModel *m_model;
    QStringListModel *m_queryModel;
    QTreeView *m_treeView;
    QTextBrowser *m_detailsBrowser;
    QueryValidatingLineEdit *m_queryLineEdit;
    Utils::FilterLineEdit *m_filterLineEdit;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_displayButton;
    QPushButton *m_applyButton;
    QPushButton *m_checkoutButton;
    QPushButton *m_refreshButton;
    bool m_fetchRunning;
};

} // namespace Internal
} // namespace Gerrit

#endif // GERRIT_INTERNAL_GERRITDIALOG_H
