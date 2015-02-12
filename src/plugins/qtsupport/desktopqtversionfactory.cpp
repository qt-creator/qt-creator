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
#include "desktopqtversionfactory.h"
#include "desktopqtversion.h"
#include <qtsupport/qtsupportconstants.h>

#include <QFileInfo>

using namespace QtSupport;
using namespace QtSupport::Internal;

DesktopQtVersionFactory::DesktopQtVersionFactory(QObject *parent)
    : QtVersionFactory(parent)
{

}

DesktopQtVersionFactory::~DesktopQtVersionFactory()
{

}

bool DesktopQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(Constants::DESKTOPQT);
}

BaseQtVersion *DesktopQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    DesktopQtVersion *v = new DesktopQtVersion;
    v->fromMap(data);
    return v;
}

int DesktopQtVersionFactory::priority() const
{
    // Lowest of all, we want to be the fallback
    return 0;
}

BaseQtVersion *DesktopQtVersionFactory::create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    Q_UNUSED(evaluator);
    // we are the fallback :) so we don't care what kind of qt it is
    QFileInfo fi = qmakePath.toFileInfo();
    if (fi.exists() && fi.isExecutable() && fi.isFile())
        return new DesktopQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
    return 0;
}
