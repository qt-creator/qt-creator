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

#ifndef SESSIONDIALOG_H
#define SESSIONDIALOG_H

#include <QString>
#include <QDialog>

#include "ui_sessiondialog.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace ProjectExplorer {

class SessionManager;

namespace Internal {

class SessionDialog : public QDialog
{
    Q_OBJECT
public:
    SessionDialog(SessionManager *sessionManager, QWidget *parent = 0);

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
    Ui::SessionDialog m_ui;
    SessionManager *m_sessionManager;
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
