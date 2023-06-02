// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cppmodelmanager.h"
#include "searchsymbols.h"

#include <cplusplus/CppDocument.h>

#include <QHash>

namespace CppEditor {

class CPPEDITOR_EXPORT CppLocatorData : public QObject
{
    Q_OBJECT

    // Only one instance, created by the CppModelManager.
    CppLocatorData();
    friend class Internal::CppModelManagerPrivate;

public:
    void filterAllFiles(IndexItem::Visitor func) const
    {
        QMutexLocker locker(&m_pendingDocumentsMutex);
        flushPendingDocument(true);
        QHash<QString, IndexItem::Ptr> infosByFile = m_infosByFile;
        locker.unlock();
        for (auto i = infosByFile.constBegin(), ei = infosByFile.constEnd(); i != ei; ++i)
            if (i.value()->visitAllChildren(func) == IndexItem::Break)
                return;
    }

    QList<IndexItem::Ptr> findSymbols(IndexItem::ItemType type, const QString &symbolName) const;

public slots:
    void onDocumentUpdated(const CPlusPlus::Document::Ptr &document);
    void onAboutToRemoveFiles(const QStringList &files);

private:
    // Ensure to protect every call to this method with m_pendingDocumentsMutex
    void flushPendingDocument(bool force) const;

    mutable SearchSymbols m_search;
    mutable QHash<QString, IndexItem::Ptr> m_infosByFile;

    mutable QMutex m_pendingDocumentsMutex;
    mutable QVector<CPlusPlus::Document::Ptr> m_pendingDocuments;
};

} // namespace CppEditor
