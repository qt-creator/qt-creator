/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidqtversionfactory.h"
#include "androidqtversion.h"
#include "androidconstants.h"
#include <qtsupport/qtsupportconstants.h>
#include <utils/qtcassert.h>
#include <proparser/profileevaluator.h>

#include <QFileInfo>

namespace Android {
namespace Internal {

AndroidQtVersionFactory::AndroidQtVersionFactory(QObject *parent)
    : QtSupport::QtVersionFactory(parent)
{
}

bool AndroidQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::ANDROIDQT);
}

QtSupport::BaseQtVersion *AndroidQtVersionFactory::restore(const QString &type,
    const QVariantMap &data)
{
    QTC_ASSERT(canRestore(type), return 0);
    AndroidQtVersion *v = new AndroidQtVersion;
    v->fromMap(data);
    return v;
}

int AndroidQtVersionFactory::priority() const
{
    return 90;
}

QtSupport::BaseQtVersion *AndroidQtVersionFactory::create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;
    if (!evaluator->values(QLatin1String("CONFIG")).contains(QLatin1String("android"))
            && evaluator->value(QLatin1String("QMAKE_PLATFORM")) != QLatin1String("android"))
        return 0;
    if (evaluator->values(QLatin1String("CONFIG")).contains(QLatin1String("android-no-sdk")))
        return 0;
    return new AndroidQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
}

} // Internal
} // Android
