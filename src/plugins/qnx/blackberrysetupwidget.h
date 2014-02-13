/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#ifndef BLACKBERRYSETUPWIDGET_H
#define BLACKBERRYSETUPWIDGET_H

#include <QFrame>
#include <QWidget>
#include <QCoreApplication>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {


class SetupItem : public QFrame {
    Q_OBJECT

public:
    enum Status {
        Ok, Info, Warning, Error
    };

    SetupItem(const QString &desc = QString(), QWidget *parent = 0);

protected:
    void set(Status status, const QString &message, const QString &fixText = QString());
    Q_SLOT virtual void validate() = 0;
    virtual void fix() = 0;

private slots:
    void onFixPressed();
    void validateLater();

private:
    QLabel *m_icon;
    QLabel *m_label;
    QPushButton *m_button;
    QLabel *m_desc;
    QTimer m_timer;
};

class APILevelSetupItem : public SetupItem
{
    Q_OBJECT

public:
    APILevelSetupItem(QWidget *parent = 0);

    enum FoundType {
        Any = (1 << 0),
        Valid = (1 << 1),
        Active = (1 << 2),
        V_10_2 = (1 << 3),
        V_10_2_AS_DEFAULT = (1 << 4)
    };
    Q_DECLARE_FLAGS(FoundTypes, FoundType)

protected:
    virtual void validate();
    virtual void fix();

private slots:
    void handleInstallationFinished();

private:
    FoundTypes resolvedFoundType();
    void installAPILevel();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(APILevelSetupItem::FoundTypes)

class SigningKeysSetupItem : public SetupItem
{
    Q_OBJECT

public:
    SigningKeysSetupItem(QWidget *parent = 0);

protected:
    virtual void validate();
    virtual void fix();

private slots:
    void defaultCertificateLoaded(int status);
};

class DeviceSetupItem : public SetupItem
{
    Q_OBJECT

public:
    DeviceSetupItem(QWidget *parent = 0);

protected:
    virtual void validate();
    virtual void fix();
};

class BlackBerrySetupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BlackBerrySetupWidget(QWidget *parent = 0);
};

} // namespace Internal
} // namespeace Qnx

#endif // BLACKBERRYSETUPWIDGET_H
