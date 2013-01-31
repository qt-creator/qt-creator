/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "maemoqtversionfactory.h"
#include "maemoglobal.h"
#include "maemoqtversion.h"

#include <qtsupport/qtsupportconstants.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Madde {
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

QtSupport::BaseQtVersion *MaemoQtVersionFactory::create(const Utils::FileName &qmakeCommand, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    Q_UNUSED(evaluator);
    // we are the fallback :) so we don't care what kinf of qt it is
    QFileInfo fi = qmakeCommand.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    QString qmakePath = qmakeCommand.toString();
    if (MaemoGlobal::isValidMaemo5QtVersion(qmakePath)
            || MaemoGlobal::isValidHarmattanQtVersion(qmakePath)) {
        return new MaemoQtVersion(qmakeCommand, isAutoDetected, autoDetectionSource);
    }
    return 0;
}

} // namespace Internal
} // namespace Madde
