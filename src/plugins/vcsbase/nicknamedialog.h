/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef NICKNAMEDIALOG_H
#define NICKNAMEDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
    class NickNameDialog;
}
class QSortFilterProxyModel;
class QModelIndex;
class QPushButton;
QT_END_NAMESPACE

namespace VCSBase {
namespace Internal {

/* Nick name dialog: Manages a list of users read from an extended
 * mail cap file, consisting of 4 columns:
 * "Name Mail [AliasName [AliasMail]]".
 * The names can be used for insertion into "RevBy:" fields; aliases will
 * be preferred. The static functions to read/clear the mail map
 * files access a global model which is shared by all instances of the
 * dialog to achieve updating. */

class NickNameDialog : public QDialog {
    Q_OBJECT
public:
    explicit NickNameDialog(QWidget *parent = 0);
    virtual ~NickNameDialog();

    QString nickName() const;

    // Fill/clear the global nick name cache
    static bool readNickNamesFromMailCapFile(const QString &file, QString *errorMessage);
    static void clearNickNames();
    // Return a list for a completer on the field line edits
    static QStringList nickNameList();

private slots:
    void slotCurrentItemChanged(const QModelIndex &);
    void slotDoubleClicked(const QModelIndex &);

private:
    QPushButton *okButton() const;

    Ui::NickNameDialog *m_ui;
    QSortFilterProxyModel *m_filterModel;
};

} // namespace Internal
} // namespace VCSBase

#endif // NICKNAMEDIALOG_H
