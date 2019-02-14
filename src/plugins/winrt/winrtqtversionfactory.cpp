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

#include "winrtqtversionfactory.h"

#include "winrtconstants.h"
#include "winrtphoneqtversion.h"

#include <proparser/profileevaluator.h>

namespace WinRt {
namespace Internal {

WinRtQtVersionFactory::WinRtQtVersionFactory()
{
    setQtVersionCreator([] { return new WinRtQtVersion; });
    setSupportedType(Constants::WINRT_WINRTQT);
    setPriority(10);
}

QtSupport::BaseQtVersion *WinRtQtVersionFactory::create(const Utils::FileName &qmakePath,
        ProFileEvaluator *evaluator)
{
    foreach (const QString &value, evaluator->values(QLatin1String("QMAKE_PLATFORM"))) {
        if (value == QStringLiteral("winrt"))
            return new WinRtQtVersion(qmakePath);
    }

    return nullptr;
}



WinRtPhoneQtVersionFactory::WinRtPhoneQtVersionFactory()
{
    setQtVersionCreator([] { return new WinRtPhoneQtVersion; });
    setSupportedType(Constants::WINRT_WINPHONEQT);
    setPriority(10);
}

QtSupport::BaseQtVersion *WinRtPhoneQtVersionFactory::create(const Utils::FileName &qmakePath,
        ProFileEvaluator *evaluator)
{
    foreach (const QString &value, evaluator->values(QLatin1String("QMAKE_PLATFORM"))) {
        if (value == QStringLiteral("winphone"))
            return new WinRtPhoneQtVersion(qmakePath);
    }

    return nullptr;
}

} // Internal
} // WinRt
