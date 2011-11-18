/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
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
        : QThread(parent)
        , m_wasCancelled(false)
{
}

SemanticInfoUpdater::~SemanticInfoUpdater()
{
}

void SemanticInfoUpdater::abort()
{
    QMutexLocker locker(&m_mutex);
    m_wasCancelled = true;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::update(const SemanticInfoUpdaterSource &source)
{
    QMutexLocker locker(&m_mutex);
    m_source = source;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::run()
{
    setPriority(QThread::LowestPriority);

    forever {
        m_mutex.lock();

        while (! (m_wasCancelled || m_source.isValid()))
            m_condition.wait(&m_mutex);

        const bool done = m_wasCancelled;
        const SemanticInfoUpdaterSource source = m_source;
        m_source.clear();

        m_mutex.unlock();

        if (done)
            break;

        const SemanticInfo info = makeNewSemanticInfo(source);

        m_mutex.lock();
        const bool cancelledOrNewData = m_wasCancelled || m_source.isValid();
        m_mutex.unlock();

        if (! cancelledOrNewData) {
            m_lastSemanticInfo = info;
            emit updated(info);
        }
    }
}

SemanticInfo SemanticInfoUpdater::makeNewSemanticInfo(const SemanticInfoUpdaterSource &source)
{
    using namespace QmlJS;

    SemanticInfo semanticInfo;
    const Document::Ptr &doc = semanticInfo.document = source.document;
    semanticInfo.snapshot = source.snapshot;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    Link link(semanticInfo.snapshot, modelManager->importPaths(), modelManager->builtins(doc));
    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    ScopeChain *scopeChain = new ScopeChain(doc, semanticInfo.context);
    semanticInfo.m_rootScopeChain = QSharedPointer<const ScopeChain>(scopeChain);

    if (doc->language() != Document::JsonLanguage) {
        Check checker(doc, semanticInfo.context);
        semanticInfo.staticAnalysisMessages = checker();
    }

    return semanticInfo;
}

} // namespace Internal
} // namespace QmlJSEditor
