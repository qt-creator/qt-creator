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

#include "blackberrydebugtokenuploader.h"

#include "qnxconstants.h"

namespace {
static const char ERR_NO_ROUTE_HOST[] = "Cannot connect";
static const char ERR_AUTH_FAILED[] = "Authentication failed";
static const char ERR_DEVELOPMENT_MODE_DISABLED[] = "Device is not in the Development Mode";
static const char ERR_FILE_NOT_EXIST[] = "File does not exist";
}

namespace Qnx {
namespace Internal {

BlackBerryDebugTokenUploader::BlackBerryDebugTokenUploader(QObject *parent) :
    BlackBerryNdkProcess(QLatin1String(Constants::QNX_BLACKBERRY_DEPLOY_CMD), parent)
{
    addErrorStringMapping(QLatin1String(ERR_NO_ROUTE_HOST), NoRouteToHost);
    addErrorStringMapping(QLatin1String(ERR_AUTH_FAILED), AuthenticationFailed);
    addErrorStringMapping(QLatin1String(ERR_DEVELOPMENT_MODE_DISABLED), DevelopmentModeDisabled);
    addErrorStringMapping(QLatin1String(ERR_FILE_NOT_EXIST), InvalidDebugTokenPath);
}

void BlackBerryDebugTokenUploader::uploadDebugToken(const QString &path,
        const QString &deviceIp, const QString &devicePassword)
{
    QStringList arguments;

    arguments << QLatin1String("-installDebugToken")
              << path
              << QLatin1String("-device")
              << deviceIp
              << QLatin1String("-password")
              << devicePassword;

    start(arguments);
}

} // namespace Internal
} // namespace Qnx
