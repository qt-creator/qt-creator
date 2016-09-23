/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

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
    explicit SessionDialog(QWidget *parent = nullptr);

    void setAutoLoadSession(bool);
    bool autoLoadSession() const;

private:
    void updateActions(const QString &session);

    Ui::SessionDialog m_ui;
};

class SessionNameInputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SessionNameInputDialog(const QStringList &sessions, QWidget *parent = nullptr);

    void setValue(const QString &value);
    QString value() const;
    bool isSwitchToRequested() const;

private:
    void clicked(QAbstractButton *button);

    QLineEdit *m_newSessionLineEdit;
    QPushButton *m_switchToButton;
    bool m_usedSwitchTo;
};

} // namespace Internal
} // namespace ProjectExplorer
