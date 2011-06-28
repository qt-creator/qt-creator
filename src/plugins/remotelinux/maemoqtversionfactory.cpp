/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "maemoqtversionfactory.h"
#include "maemoglobal.h"
#include "maemoqtversion.h"

#include <qtsupport/qtsupportconstants.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>

namespace RemoteLinux {
namespace Internal {

MaemoQtVersionFactory::MaemoQtVersionFactory(QObject *parent)
    : QtSupport::QtVersionFactory(parent)
{

}

MaemoQtVersionFactory::~MaemoQtVersionFactory()
{

}

bool MaemoQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(QtSupport::Constants::MAEMOQT);
}

QtSupport::BaseQtVersion *MaemoQtVersionFactory::restore(const QString &type,
    const QVariantMap &data)
{
    QTC_ASSERT(canRestore(type), return 0);
    MaemoQtVersion *v = new MaemoQtVersion;
    v->fromMap(data);
    return v;
}

int MaemoQtVersionFactory::priority() const
{
    return 50;
}

QtSupport::BaseQtVersion *MaemoQtVersionFactory::create(const QString &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    Q_UNUSED(evaluator);
    // we are the fallback :) so we don't care what kinf of qt it is
    QFileInfo fi(qmakePath);
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    if (MaemoGlobal::isValidMaemo5QtVersion(qmakePath)
            || MaemoGlobal::isValidHarmattanQtVersion(qmakePath)
            || MaemoGlobal::isValidMeegoQtVersion(qmakePath))
        return new MaemoQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
    return 0;
}

} // Internal
} // Qt4ProjectManager
