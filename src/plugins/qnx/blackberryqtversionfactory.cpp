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

#include "blackberryqtversionfactory.h"

#include "qnxconstants.h"
#include "blackberryqtversion.h"
#include "qnxutils.h"

#include <qtsupport/profilereader.h>

using namespace Qnx;
using namespace Qnx::Internal;

BlackBerryQtVersionFactory::BlackBerryQtVersionFactory(QObject *parent) :
    QtSupport::QtVersionFactory(parent)
{
}

BlackBerryQtVersionFactory::~BlackBerryQtVersionFactory()
{
}

bool BlackBerryQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::QNX_BB_QT);
}

QtSupport::BaseQtVersion *BlackBerryQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    BlackBerryQtVersion *v = new BlackBerryQtVersion();
    v->fromMap(data);
    return v;
}

int BlackBerryQtVersionFactory::priority() const
{
    return Constants::QNX_BB_QT_FACTORY_PRIO;
}

QtSupport::BaseQtVersion *BlackBerryQtVersionFactory::create(const Utils::FileName &qmakePath,
                                                      ProFileEvaluator *evaluator,
                                                      bool isAutoDetected,
                                                      const QString &autoDetectionSource)
{
    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    if (evaluator->value(QLatin1String("CONFIG")).contains(QLatin1String("blackberry"))) {
        QString cpuDir = evaluator->value(QLatin1String("QNX_CPUDIR"));
        return new BlackBerryQtVersion(QnxUtils::cpudirToArch(cpuDir), qmakePath,
                                isAutoDetected, autoDetectionSource);
    }

    return 0;
}
