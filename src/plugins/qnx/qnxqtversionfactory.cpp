/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxqtversionfactory.h"

#include "qnxconstants.h"
#include "qnxutils.h"
#include "qnxqtversion.h"

#include <qtsupport/profilereader.h>

#include <QFileInfo>

using namespace Qnx;
using namespace Qnx::Internal;

QnxQtVersionFactory::QnxQtVersionFactory(QObject *parent) :
    QtSupport::QtVersionFactory(parent)
{
}

QnxQtVersionFactory::~QnxQtVersionFactory()
{
}

bool QnxQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::QNX_QNX_QT);
}

QtSupport::BaseQtVersion *QnxQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    QnxQtVersion *v = new QnxQtVersion();
    v->fromMap(data);
    return v;
}

int QnxQtVersionFactory::priority() const
{
    return 50;
}

QtSupport::BaseQtVersion *QnxQtVersionFactory::create(const Utils::FileName &qmakePath,
                                                      ProFileEvaluator *evaluator,
                                                      bool isAutoDetected,
                                                      const QString &autoDetectionSource)
{
    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    if (evaluator->contains(QLatin1String("QNX_CPUDIR"))) {
        return new QnxQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
    }

    return 0;
}
