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

#ifndef QMLJSLINK_H
#define QMLJSLINK_H

#include <qmljs/qmljscontext.h>

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
    Link(const Snapshot &snapshot, const ViewerContext &vContext, const LibraryInfo &builtins);

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
