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

#include <QIntValidator>
#include <QSettings>

#include "proxysettings.h"

QT_BEGIN_NAMESPACE

ProxySettings::ProxySettings (QWidget * parent)
    : QDialog (parent), Ui::ProxySettings()
{
    setupUi (this);

#if !defined Q_WS_MAEMO_5
    // the onscreen keyboard can't cope with masks
    proxyServerEdit->setInputMask ("000.000.000.000;_");
#endif
    QIntValidator *validator = new QIntValidator (0, 9999, this);
    proxyPortEdit->setValidator (validator);

    QSettings settings;
    proxyCheckBox->setChecked (settings.value ("http_proxy/use", 0).toBool ());
    proxyServerEdit->insert (settings.value ("http_proxy/hostname", "").toString ());
    proxyPortEdit->insert (settings.value ("http_proxy/port", "80").toString ());
    usernameEdit->insert (settings.value ("http_proxy/username", "").toString ());
    passwordEdit->insert (settings.value ("http_proxy/password", "").toString ());
}

ProxySettings::~ProxySettings()
{
}

void ProxySettings::accept ()
{
    QSettings settings;

    settings.setValue ("http_proxy/use", proxyCheckBox->isChecked ());
    settings.setValue ("http_proxy/hostname", proxyServerEdit->text ());
    settings.setValue ("http_proxy/port", proxyPortEdit->text ());
    settings.setValue ("http_proxy/username", usernameEdit->text ());
    settings.setValue ("http_proxy/password", passwordEdit->text ());

    QDialog::accept ();
}

QNetworkProxy ProxySettings::httpProxy ()
{
    QSettings settings;
    QNetworkProxy proxy;

    bool proxyInUse = settings.value ("http_proxy/use", 0).toBool ();
    if (proxyInUse) {
        proxy.setType (QNetworkProxy::HttpProxy);
        proxy.setHostName (settings.value ("http_proxy/hostname", "").toString ());// "192.168.220.5"
        proxy.setPort (settings.value ("http_proxy/port", 80).toInt ());  // 8080
        proxy.setUser (settings.value ("http_proxy/username", "").toString ());
        proxy.setPassword (settings.value ("http_proxy/password", "").toString ());
        //QNetworkProxy::setApplicationProxy (proxy);
    }
    else {
        proxy.setType (QNetworkProxy::NoProxy);
    }
    return proxy;
}

bool ProxySettings::httpProxyInUse()
{
    QSettings settings;
    return settings.value ("http_proxy/use", 0).toBool ();
}

QT_END_NAMESPACE
