/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxqtversionfactory.h"

#include "qnxconstants.h"
#include "qnxutils.h"
#include "qnxqtversion.h"

#include <qtsupport/profilereader.h>

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
    return Constants::QNX_QNX_QT_FACTORY_PRIO;
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
        QString cpuDir = evaluator->value(QLatin1String("QNX_CPUDIR"));
        return new QnxQtVersion(QnxUtils::cpudirToArch(cpuDir), qmakePath,
                                isAutoDetected, autoDetectionSource);
    }

    return 0;
}
