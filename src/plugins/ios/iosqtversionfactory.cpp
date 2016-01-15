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

#include "iosqtversionfactory.h"
#include "iosqtversion.h"
#include "iosconstants.h"
#include <qtsupport/qtsupportconstants.h>
#include <utils/qtcassert.h>
#include <proparser/profileevaluator.h>

#include <QFileInfo>

namespace Ios {
namespace Internal {

IosQtVersionFactory::IosQtVersionFactory(QObject *parent)
    : QtSupport::QtVersionFactory(parent)
{
}

bool IosQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::IOSQT);
}

QtSupport::BaseQtVersion *IosQtVersionFactory::restore(const QString &type,
    const QVariantMap &data)
{
    QTC_ASSERT(canRestore(type), return 0);
    IosQtVersion *v = new IosQtVersion;
    v->fromMap(data);
    return v;
}

int IosQtVersionFactory::priority() const
{
    return 90;
}

QtSupport::BaseQtVersion *IosQtVersionFactory::create(const Utils::FileName &qmakePath,
                                                      ProFileEvaluator *evaluator,
                                                      bool isAutoDetected,
                                                      const QString &autoDetectionSource)
{
    if (!(evaluator->values(QLatin1String("QMAKE_PLATFORM")).contains(QLatin1String("ios"))))
        return 0;
    return new IosQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
}

} // Internal
} // Ios
