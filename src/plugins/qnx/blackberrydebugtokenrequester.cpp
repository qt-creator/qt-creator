/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberrydebugtokenrequester.h"

namespace {
static const char PROCESS_NAME[] = "blackberry-debugtokenrequest";
static const char ERR_WRONG_CSK_PASS[] = "The signature on the code signing request didn't verify.";
static const char ERR_WRONG_KEYSTORE_PASS[] = "Failed to decrypt keystore, invalid password";
static const char ERR_ILLEGAL_DEVICE_PIN[] = "Illegal device PIN";
static const char ERR_NETWORK_UNREACHABLE[] = "Network is unreachable";
static const char ERR_NOT_YET_REGISTGERED[] = "Not yet registered to request debug tokens";
}

namespace Qnx {
namespace Internal {

BlackBerryDebugTokenRequester::BlackBerryDebugTokenRequester(QObject *parent) :
    BlackBerryNdkProcess(QLatin1String(PROCESS_NAME), parent)
{
    addErrorStringMapping(QLatin1String(ERR_WRONG_CSK_PASS), WrongCskPassword);
    addErrorStringMapping(QLatin1String(ERR_WRONG_KEYSTORE_PASS), WrongKeystorePassword);
    addErrorStringMapping(QLatin1String(ERR_WRONG_KEYSTORE_PASS), WrongKeystorePassword);
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
              << cskPassword
              << QLatin1String("-devicepin")
              << devicePin
              << path;

    start(arguments);

}

} // namespace Internal
} // namespace Qnx
