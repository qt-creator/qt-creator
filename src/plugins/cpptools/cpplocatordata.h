/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPLOCATORDATA_H
#define CPPLOCATORDATA_H

#include <functional>
#include <QHash>

#include <cplusplus/CppDocument.h>

#include "cpptools_global.h"
#include "cppmodelmanager.h"
#include "searchsymbols.h"
#include "stringtable.h"

namespace CppTools {

namespace Internal {
class CppToolsPlugin;
} // Internal namespace

class CppLocatorData : public QObject
{
    Q_OBJECT

    // Only one instance, created by the CppToolsPlugin.
    CppLocatorData();
    friend class Internal::CppToolsPlugin;

public:
    void filterAllFiles(IndexItem::Visitor func) const
    {
        flushPendingDocument(true);
        QMutexLocker locker(&m_pendingDocumentsMutex);
        QHash<QString, IndexItem::Ptr> infosByFile = m_infosByFile;
        locker.unlock();
        for (auto i = infosByFile.constBegin(), ei = infosByFile.constEnd(); i != ei; ++i)
            if (i.value()->visitAllChildren(func) == IndexItem::Break)
                return;
    }

public slots:
    void onDocumentUpdated(const CPlusPlus::Document::Ptr &document);
    void onAboutToRemoveFiles(const QStringList &files);

private:
    void flushPendingDocument(bool force) const;
    QList<IndexItem::Ptr> allIndexItems(const QHash<QString, QList<IndexItem::Ptr>> &items) const;

    QString findOrInsertFilePath(const QString &path) const
    { return m_strings->insert(path); }

private:
    Internal::StringTable *m_strings; // Used to avoid QString duplication

    mutable SearchSymbols m_search;
    mutable QHash<QString, IndexItem::Ptr> m_infosByFile;

    mutable QMutex m_pendingDocumentsMutex;
    mutable QVector<CPlusPlus::Document::Ptr> m_pendingDocuments;
};

} // CppTools namespace

#endif // CPPLOCATORDATA_H
