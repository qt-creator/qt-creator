/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BLACKBERRYDEBUGTOKENSDIALOG_H
#define BLACKBERRYDEBUGTOKENSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class Ui_BlackBerryDebugTokenPinsDialog;

class BlackBerryDebugTokenPinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BlackBerryDebugTokenPinsDialog(const QString &debugToken, QWidget *parent = 0);
    ~BlackBerryDebugTokenPinsDialog();

private slots:
    void addPin();
    void editPin();
    void removePin();
    void updateUi(const QModelIndex& index);

    void emitUpdatedPins();
    QString promptPIN(const QString& defaultValue, bool *ok = 0);

signals:
    void pinsUpdated(const QStringList &pins);

private:
    Ui_BlackBerryDebugTokenPinsDialog *ui;
    QStandardItemModel *m_model;

    QPushButton *m_okButton;

    QString m_debugTokenPath;
    bool m_updated;
};

} // Internal
} // Qnx
#endif // BLACKBERRYDEBUGTOKENSDIALOG_H
