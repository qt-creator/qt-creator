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

#ifndef QMLJSLOCATORDATA_H
#define QMLJSLOCATORDATA_H

#include <qmljs/qmljsdocument.h>

#include <QObject>
#include <QHash>
#include <QMutex>

namespace QmlJSTools {
namespace Internal {

class LocatorData : public QObject
{
    Q_OBJECT
public:
    explicit LocatorData(QObject *parent = 0);
    ~LocatorData();

    enum EntryType
    {
        Function
    };

    class Entry
    {
    public:
        EntryType type;
        QString symbolName;
        QString displayName;
        QString extraInfo;
        QString fileName;
        int line;
        int column;
    };

    QHash<QString, QList<Entry> > entries() const;

private slots:
    void onDocumentUpdated(const QmlJS::Document::Ptr &doc);
    void onAboutToRemoveFiles(const QStringList &files);

private:
    mutable QMutex m_mutex;
    QHash<QString, QList<Entry> > m_entries;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSLOCATORDATA_H
