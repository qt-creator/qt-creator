/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef SUBCOMPONENTMANAGER_H
#define SUBCOMPONENTMANAGER_H

#include "corelib_global.h"

#include <metainfo.h>

#include <QObject>
#include <QString>
#include <QUrl>
#include <private/qdeclarativedom_p.h>

namespace QmlDesigner {

class CORESHARED_EXPORT SubComponentManager : public QObject
{
    Q_OBJECT
public:
    explicit SubComponentManager(MetaInfo metaInfo, QObject *parent = 0);
    ~SubComponentManager();

    void update(const QUrl &fileUrl, const QByteArray &data);
    void update(const QUrl &fileUrl, const QList<QDeclarativeDomImport> &imports);

    QStringList qmlFiles() const;
    QStringList directories() const;

signals:
    void qmlFilesChanged(const QStringList &oldPathList, const QStringList &newPathList);

private:
    friend class Internal::SubComponentManagerPrivate;
    class Internal::SubComponentManagerPrivate *m_d;
};

} // namespace QmlDesigner


#endif // SUBCOMPONENTMANAGER_H
