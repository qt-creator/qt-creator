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

#ifndef NICKNAMEDIALOG_H
#define NICKNAMEDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QStandardItemModel;
class QModelIndex;
class QPushButton;
QT_END_NAMESPACE

namespace VcsBase {
namespace Internal {

namespace Ui { class NickNameDialog; }

class NickNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NickNameDialog(QStandardItemModel *model, QWidget *parent = 0);
    virtual ~NickNameDialog();

    QString nickName() const;

    // Utilities to initialize/populate the model
    static QStandardItemModel *createModel(QObject *parent);
    static bool populateModelFromMailCapFile(const QString &file,
                                             QStandardItemModel *model,
                                             QString *errorMessage);

    // Return a list for a completer on the field line edits
    static QStringList nickNameList(const QStandardItemModel *model);

private slots:
    void slotCurrentItemChanged(const QModelIndex &);
    void slotDoubleClicked(const QModelIndex &);

private:
    QPushButton *okButton() const;

    Ui::NickNameDialog *m_ui;
    QStandardItemModel *m_model;
    QSortFilterProxyModel *m_filterModel;
};

} // namespace Internal
} // namespace VcsBase

#endif // NICKNAMEDIALOG_H
