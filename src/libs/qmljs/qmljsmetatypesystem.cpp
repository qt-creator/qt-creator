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

#include "qmljsinterpreter.h"
#include "qmljsmetatypesystem.h"

#ifndef NO_DECLARATIVE_BACKEND
#include <QtDeclarative/private/qdeclarativemetatype_p.h>
#endif // NO_DECLARATIVE_BACKEND

using namespace QmlJS;
using namespace QmlJS::Interpreter;

void MetaTypeSystem::reload(Interpreter::Engine *interpreter)
{
    _importedTypes.clear();

#ifndef NO_DECLARATIVE_BACKEND
    foreach (QDeclarativeType *type, QDeclarativeMetaType::qmlTypes()) {
        const QString fqType = type->qmlTypeName();
        const int sepIdx = fqType.lastIndexOf(QLatin1Char('/'));
        QString typeName;
        QString package;
        if (sepIdx == -1) {
            typeName = fqType;
        } else {
            typeName = fqType.mid(sepIdx + 1);
            package = fqType.left(sepIdx);
        }

        _importedTypes[package].append(interpreter->newQmlObject(typeName, package, type->majorVersion(), type->minorVersion()));
    }
}

QList<QmlObjectValue *> MetaTypeSystem::staticTypesForImport(const QString &prefix, int majorVersion, int minorVersion) const
{
    QMap<QString, QmlObjectValue *> objectValuesByName;

    foreach (QmlObjectValue *qmlObjectValue, _importedTypes.value(prefix)) {
        if (qmlObjectValue->majorVersion() < majorVersion ||
            (qmlObjectValue->majorVersion() == majorVersion && qmlObjectValue->minorVersion() <= minorVersion)) {
            // we got a candidate.
            const QString typeName = qmlObjectValue->qmlTypeName();
            QmlObjectValue *previousCandidate = objectValuesByName.value(typeName, 0);
            if (previousCandidate) {
                // check if our new candidate is newer than the one we found previously
                if (qmlObjectValue->majorVersion() > previousCandidate->majorVersion() ||
                    (qmlObjectValue->majorVersion() == previousCandidate->majorVersion() && qmlObjectValue->minorVersion() > previousCandidate->minorVersion())) {
                    // the new candidate has a higher version no. than the one we found previously, so replace it
                    objectValuesByName.insert(typeName, qmlObjectValue);
                }
            } else {
                objectValuesByName.insert(typeName, qmlObjectValue);
            }
        }
    }

    return objectValuesByName.values();
#endif // NO_DECLARATIVE_BACKEND
}
