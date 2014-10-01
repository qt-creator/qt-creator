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

#include "blackberrydebugtokenrequester.h"

namespace {
static const char PROCESS_NAME[] = "blackberry-debugtokenrequest";
static const char ERR_WRONG_CSK_PASS[] = "The signature on the code signing request didn't verify.";
static const char ERR_WRONG_CSK_PASS_10_2[] = "The specified CSK password is not valid.";
static const char ERR_WRONG_KEYSTORE_PASS[] = "Failed to decrypt keystore, invalid password";
static const char ERR_WRONG_KEYSTORE_PASS_10_2[] = "Failed to decrypt keystore, invalid store password or store password not supplied.";
static const char ERR_NETWORK_UNREACHABLE[] = "Network is unreachable";
static const char ERR_NOT_YET_REGISTGERED[] = "Not yet registered to request debug tokens";
}

namespace Qnx {
namespace Internal {

BlackBerryDebugTokenRequester::BlackBerryDebugTokenRequester(QObject *parent) :
    BlackBerryNdkProcess(QLatin1String(PROCESS_NAME), parent)
{
    addErrorStringMapping(QLatin1String(ERR_WRONG_CSK_PASS), WrongCskPassword);
    addErrorStringMapping(QLatin1String(ERR_WRONG_CSK_PASS_10_2), WrongCskPassword);
    addErrorStringMapping(QLatin1String(ERR_WRONG_KEYSTORE_PASS), WrongKeystorePassword);
    addErrorStringMapping(QLatin1String(ERR_WRONG_KEYSTORE_PASS_10_2), WrongKeystorePassword);
    addErrorStringMapping(QLatin1String(ERR_NETWORK_UNREACHABLE), NetworkUnreachable);
    addErrorStringMapping(QLatin1String(ERR_NOT_YET_REGISTGERED), NotYetRegistered);
}

void BlackBerryDebugTokenRequester::requestDebugToken(const QString &path,
                                                      const QString &cskPassword, const QString &keyStore,
                                                      const QString &keyStorePassword, const QString &devicePin)
{
    QStringList arguments;

    arguments << QLatin1String("-keystore")
              << keyStore
              << QLatin1String("-storepass")
              << keyStorePassword
              << QLatin1String("-cskpass")
              << cskPassword;

    // devicePin may contain multiple pins
    QStringList pins = devicePin.split(QLatin1Char(','));
    foreach (const QString &pin, pins)
        arguments << QLatin1String("-devicepin") << pin;

    arguments << path;

    start(arguments);

}

} // namespace Internal
} // namespace Qnx
