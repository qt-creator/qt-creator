/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljsmetatypebackend.h"
#include "qmljstypesystem.h"

#ifndef NO_DECLARATIVE_BACKEND
#  include "qtdeclarativemetatypebackend.h"
#endif // NO_DECLARATIVE_BACKEND

#include <QDebug>

using namespace QmlJS;

TypeSystem::TypeSystem()
{
#ifndef NO_DECLARATIVE_BACKEND
    backends.append(new Internal::QtDeclarativeMetaTypeBackend(this));
#endif // NO_DECLARATIVE_BACKEND
}

TypeSystem::~TypeSystem()
{
    qDeleteAll(backends);
}

QList<Symbol *> TypeSystem::availableTypes(const QString &package, int majorVersion, int minorVersion)
{
    QList<Symbol *> results;

    foreach (MetaTypeBackend *backend, backends)
        results.append(backend->availableTypes(package, majorVersion, minorVersion));

    return results;
}

Symbol *TypeSystem::resolve(const QString &typeName, const QList<PackageInfo> &packages)
{
    foreach (MetaTypeBackend *backend, backends)
        if (Symbol *symbol = backend->resolve(typeName, packages))
            return symbol;

    return 0;
}
