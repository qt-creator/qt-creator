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

#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <languageutils/componentversion.h>

#include <QCoreApplication>

namespace QmlJS {

class NameId;
class LinkPrivate;

/*
    Helper for building a context.
*/
class QMLJS_EXPORT Link
{
    Q_DISABLE_COPY(Link)
    Q_DECLARE_TR_FUNCTIONS(QmlJS::Link)

public:
    Link(const Snapshot &snapshot, const QStringList &importPaths, const LibraryInfo &builtins);

    // Link all documents in snapshot, collecting all diagnostic messages (if messages != 0)
    ContextPtr operator()(QHash<QString, QList<DiagnosticMessage> > *messages = 0);

    // Link all documents in snapshot, appending the diagnostic messages
    // for 'doc' in 'messages'
    ContextPtr operator()(const Document::Ptr &doc, QList<DiagnosticMessage> *messages);

    ~Link();

private:
    LinkPrivate *d;
    friend class LinkPrivate;
};

} // namespace QmlJS

#endif // QMLJSLINK_H
