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

    if (evaluator->values(QLatin1String("CONFIG")).contains(QLatin1String("blackberry"))) {
        QString cpuDir = evaluator->value(QLatin1String("QNX_CPUDIR"));
        return new BlackBerryQtVersion(QnxUtils::cpudirToArch(cpuDir), qmakePath,
                                isAutoDetected, autoDetectionSource);
    }

    return 0;
}
