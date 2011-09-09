/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljssemanticinfoupdater.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljslink.h>
#include <qmljstools/qmljsmodelmanager.h>

namespace QmlJSEditor {
namespace Internal {

SemanticInfoUpdater::SemanticInfoUpdater(QObject *parent)
        : QThread(parent),
          m_done(false),
          m_modelManager(0)
{
}

SemanticInfoUpdater::~SemanticInfoUpdater()
{
}

void SemanticInfoUpdater::abort()
{
    QMutexLocker locker(&m_mutex);
    m_done = true;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::update(const SemanticInfoUpdaterSource &source)
{
    QMutexLocker locker(&m_mutex);
    m_source = source;
    m_condition.wakeOne();
}

bool SemanticInfoUpdater::isOutdated()
{
    QMutexLocker locker(&m_mutex);
    const bool outdated = ! m_source.fileName.isEmpty() || m_done;
    return outdated;
}

void SemanticInfoUpdater::run()
{
    setPriority(QThread::LowestPriority);

    forever {
        m_mutex.lock();

        while (! (m_done || ! m_source.fileName.isEmpty()))
            m_condition.wait(&m_mutex);

        const bool done = m_done;
        const SemanticInfoUpdaterSource source = m_source;
        m_source.clear();

        m_mutex.unlock();

        if (done)
            break;

        const SemanticInfo info = semanticInfo(source);

        if (! isOutdated()) {
            m_mutex.lock();
            m_lastSemanticInfo = info;
            m_mutex.unlock();

            emit updated(info);
        }
    }
}

SemanticInfo SemanticInfoUpdater::semanticInfo(const SemanticInfoUpdaterSource &source)
{
    m_mutex.lock();
    const int revision = m_lastSemanticInfo.revision();
    m_mutex.unlock();

    QmlJS::Snapshot snapshot;
    QmlJS::Document::Ptr doc;

    if (! source.force && revision == source.revision) {
        m_mutex.lock();
        snapshot = m_lastSemanticInfo.snapshot;
        doc = m_lastSemanticInfo.document;
        m_mutex.unlock();
    }

    if (! doc) {
        snapshot = source.snapshot;
        QmlJS::Document::Language language;
        if (m_lastSemanticInfo.document)
            language = m_lastSemanticInfo.document->language();
        else
            language = QmlJSTools::languageOfFile(source.fileName);
        doc = snapshot.documentFromSource(source.code, source.fileName, language);
        doc->setEditorRevision(source.revision);
        doc->parse();
        snapshot.insert(doc);
    }

    SemanticInfo semanticInfo;
    semanticInfo.snapshot = snapshot;
    semanticInfo.document = doc;

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    QmlJS::Link link(snapshot, modelManager->importPaths(), modelManager->builtins(doc));
    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    QmlJS::ScopeChain *scopeChain = new QmlJS::ScopeChain(doc, semanticInfo.context);
    semanticInfo.m_rootScopeChain = QSharedPointer<const QmlJS::ScopeChain>(scopeChain);

    QmlJS::Check checker(doc, semanticInfo.context);
    semanticInfo.semanticMessages.append(checker());

    return semanticInfo;
}

void SemanticInfoUpdater::setModelManager(QmlJS::ModelManagerInterface *modelManager)
{
    m_modelManager = modelManager;
}

} // namespace Internal
} // namespace QmlJSEditor
