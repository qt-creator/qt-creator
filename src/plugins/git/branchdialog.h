/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef BRANCHDIALOG_H
#define BRANCHDIALOG_H

#include <QDialog>
#include <QItemSelection>

QT_BEGIN_NAMESPACE
class QPushButton;
class QModelIndex;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

namespace Ui {
class BranchDialog;
}

class BranchAddDialog;
class BranchModel;

/**
 * Branch dialog. Displays a list of local branches at the top and remote
 * branches below. Offers to checkout/delete local branches.
 *
 */
class BranchDialog : public QDialog {
    Q_OBJECT

public:
    explicit BranchDialog(QWidget *parent = 0);
    ~BranchDialog();

public slots:
    void refresh(const QString &repository, bool force);

private slots:
    void enableButtons();
    void refresh();
    void add();
    void checkout();
    void remove();
    void diff();
    void log();

protected:
    void changeEvent(QEvent *e);

private:
    QModelIndex selectedIndex();

    Ui::BranchDialog *m_ui;

    BranchModel *m_model;

    QString m_repository;
};

} // namespace Internal
} // namespace Git

#endif // BRANCHDIALOG_H
