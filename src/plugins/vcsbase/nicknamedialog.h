/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef NICKNAMEDIALOG_H
#define NICKNAMEDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QStandardItemModel;
class QModelIndex;
class QPushButton;
QT_END_NAMESPACE

namespace VCSBase {
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
} // namespace VCSBase

#endif // NICKNAMEDIALOG_H
