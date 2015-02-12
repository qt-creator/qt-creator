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

#include "winrtqtversionfactory.h"

#include "winrtconstants.h"
#include "winrtphoneqtversion.h"

#include <proparser/profileevaluator.h>

#include <QFileInfo>

namespace WinRt {
namespace Internal {

WinRtQtVersionFactory::WinRtQtVersionFactory(QObject *parent)
    : QtSupport::QtVersionFactory(parent)
{
}

WinRtQtVersionFactory::~WinRtQtVersionFactory()
{
}

bool WinRtQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::WINRT_WINRTQT)
            || type == QLatin1String(Constants::WINRT_WINPHONEQT);
}

QtSupport::BaseQtVersion *WinRtQtVersionFactory::restore(const QString &type,
        const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    WinRtQtVersion *v = 0;
    if (type == QLatin1String(Constants::WINRT_WINPHONEQT))
        v = new WinRtPhoneQtVersion;
    else
        v = new WinRtQtVersion;
    v->fromMap(data);
    return v;
}

int WinRtQtVersionFactory::priority() const
{
    return 10;
}

QtSupport::BaseQtVersion *WinRtQtVersionFactory::create(const Utils::FileName &qmakePath,
        ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    bool isWinRt = false;
    bool isPhone = false;
    foreach (const QString &value, evaluator->values(QLatin1String("QMAKE_PLATFORM"))) {
        if (value == QStringLiteral("winrt")) {
            isWinRt = true;
        } else if (value == QStringLiteral("winphone")) {
            isWinRt = true;
            isPhone = true;
            break;
        }
    }

    if (!isWinRt)
        return 0;

    return isPhone ? new WinRtPhoneQtVersion(qmakePath, isAutoDetected, autoDetectionSource)
                   : new WinRtQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
}

} // Internal
} // WinRt
