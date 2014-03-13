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

#include "cpplocatordata.h"
#include "cpptoolsplugin.h"

using namespace CppTools;
using namespace CppTools::Internal;

static const int MaxPendingDocuments = 10;

CppLocatorData::CppLocatorData(CppModelManager *modelManager)
    : m_modelManager(modelManager)
    , m_strings(CppToolsPlugin::stringTable())
    , m_search(m_strings)
    , m_pendingDocumentsMutex(QMutex::Recursive)
{
    m_search.setSymbolsToSearchFor(SymbolSearcher::Enums
                                 | SymbolSearcher::Classes
                                 | SymbolSearcher::Functions);
    m_pendingDocuments.reserve(MaxPendingDocuments);

    connect(m_modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(onDocumentUpdated(CPlusPlus::Document::Ptr)));

    connect(m_modelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            this, SLOT(onAboutToRemoveFiles(QStringList)));
}

QList<ModelItemInfo::Ptr> CppLocatorData::enums()
{
    flushPendingDocument(true);
    return allModelItemInfos(m_allEnums);
}

QList<ModelItemInfo::Ptr> CppLocatorData::classes()
{
    flushPendingDocument(true);
    return allModelItemInfos(m_allClasses);
}

QList<ModelItemInfo::Ptr> CppLocatorData::functions()
{
    flushPendingDocument(true);
    return allModelItemInfos(m_allFunctions);
}

void CppLocatorData::onDocumentUpdated(const CPlusPlus::Document::Ptr &document)
{
    QMutexLocker locker(&m_pendingDocumentsMutex);

    int i = 0, ei = m_pendingDocuments.size();
    for (; i < ei; ++i) {
        const CPlusPlus::Document::Ptr &doc = m_pendingDocuments.at(i);
        if (doc->fileName() == document->fileName()
                && doc->revision() < document->revision()) {
            m_pendingDocuments[i] = document;
            break;
        }
    }

    if (i == ei)
        m_pendingDocuments.append(document);

    flushPendingDocument(false);
}

void CppLocatorData::onAboutToRemoveFiles(const QStringList &files)
{
    QMutexLocker locker(&m_pendingDocumentsMutex);

    for (int i = 0; i < m_pendingDocuments.size(); ) {
        if (files.contains(m_pendingDocuments.at(i)->fileName()))
            m_pendingDocuments.remove(i);
        else
            ++i;
    }

    foreach (const QString &file, files) {
        m_allEnums.remove(file);
        m_allClasses.remove(file);
        m_allFunctions.remove(file);
    }

    m_strings.scheduleGC();
}

void CppLocatorData::flushPendingDocument(bool force)
{
    QMutexLocker locker(&m_pendingDocumentsMutex);
    if (!force && m_pendingDocuments.size() < MaxPendingDocuments)
        return;

    foreach (CPlusPlus::Document::Ptr doc, m_pendingDocuments) {
        const QString fileName = findOrInsertFilePath(doc->fileName());

        QList<ModelItemInfo::Ptr> resultsEnums;
        QList<ModelItemInfo::Ptr> resultsClasses;
        QList<ModelItemInfo::Ptr> resultsFunctions;

        const int sizeHint = m_allEnums[fileName].size() + m_allClasses[fileName].size()
                + m_allFunctions[fileName].size() + 10;
        m_search(doc, sizeHint)->visitAllChildren([&](const ModelItemInfo::Ptr &info) {
            switch (info->type()) {
            case ModelItemInfo::Enum:
                resultsEnums.append(info);
                break;
            case ModelItemInfo::Class:
                resultsClasses.append(info);
                break;
            case ModelItemInfo::Function:
                resultsFunctions.append(info);
                break;
            default:
                break;
            }
        });

        m_allEnums[fileName] = resultsEnums;
        m_allClasses[fileName] = resultsClasses;
        m_allFunctions[fileName] = resultsFunctions;
    }

    m_pendingDocuments.clear();
    m_pendingDocuments.reserve(MaxPendingDocuments);
}

QList<ModelItemInfo::Ptr> CppLocatorData::allModelItemInfos(const QHash<QString,
                                                            QList<ModelItemInfo::Ptr>> &items) const
{
    QList<ModelItemInfo::Ptr> result;
    QHashIterator<QString, QList<ModelItemInfo::Ptr> > it(items);
    while (it.hasNext()) {
        it.next();
        result.append(it.value());
    }
    return result;
}
