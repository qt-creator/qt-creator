/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "symbianqtversionfactory.h"

#include "qt4projectmanagerconstants.h"
#include "symbianqtversion.h"

#include <qtsupport/profilereader.h>
#include <qtsupport/qtsupportconstants.h>

#include <QFileInfo>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

SymbianQtVersionFactory::SymbianQtVersionFactory(QObject *parent)
    : QtVersionFactory(parent)
{

}

SymbianQtVersionFactory::~SymbianQtVersionFactory()
{

}

bool SymbianQtVersionFactory::canRestore(const QString &type)
{
    return type == QLatin1String(QtSupport::Constants::SYMBIANQT);
}

QtSupport::BaseQtVersion *SymbianQtVersionFactory::restore(const QString &type, const QVariantMap &data)
{
    if (!canRestore(type))
        return 0;
    SymbianQtVersion *v = new SymbianQtVersion;
    v->fromMap(data);
    return v;
}

int SymbianQtVersionFactory::priority() const
{
    return 50;
}

QtSupport::BaseQtVersion *SymbianQtVersionFactory::create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator, bool isAutoDetected, const QString &autoDetectionSource)
{
    QFileInfo fi = qmakePath.toFileInfo();
    if (!fi.exists() || !fi.isExecutable() || !fi.isFile())
        return 0;

    QString makefileGenerator = evaluator->value(QLatin1String("MAKEFILE_GENERATOR"));
    if (makefileGenerator == QLatin1String("SYMBIAN_ABLD") ||
            makefileGenerator == QLatin1String("SYMBIAN_SBSV2") ||
            makefileGenerator == QLatin1String("SYMBIAN_UNIX")) {
        return new SymbianQtVersion(qmakePath, isAutoDetected, autoDetectionSource);
    }

    return 0;
}
