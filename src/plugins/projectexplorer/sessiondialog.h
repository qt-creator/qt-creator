/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SESSIONDIALOG_H
#define SESSIONDIALOG_H

#include "ui_sessiondialog.h"

#include <QString>
#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

class SessionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionDialog(QWidget *parent = 0);

    void setAutoLoadSession(bool);
    bool autoLoadSession() const;

private slots:
    void createNew();
    void clone();
    void remove();
    void rename();
    void switchToSession();

    void updateActions();

private:
    void addItems(bool setDefaultSession);
    void markItems();
    void addSessionToUi(const QString &name, bool switchTo);
    Ui::SessionDialog m_ui;
};

class SessionNameInputDialog : public QDialog
{
    Q_OBJECT
public:
    SessionNameInputDialog(const QStringList &sessions, QWidget *parent = 0);

    void setValue(const QString &value);
    QString value() const;
    bool isSwitchToRequested() const;

private slots:
    void clicked(QAbstractButton *button);

private:
    QLineEdit *m_newSessionLineEdit;
    QPushButton *m_switchToButton;
    bool m_usedSwitchTo;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // SESSIONDIALOG_H
