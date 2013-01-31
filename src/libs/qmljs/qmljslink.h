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
