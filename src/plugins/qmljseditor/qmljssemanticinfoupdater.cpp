/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljssemanticinfoupdater.h"
#include "qmljseditorplugin.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/jsoncheck.h>
#include <qmljs/qmljscontext.h>
#include <qmljs/qmljslink.h>
#include <qmljstools/qmljsmodelmanager.h>

#include <coreplugin/icore.h>

#include <utils/json.h>

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

void SemanticInfoUpdater::update(const QmlJS::Document::Ptr &doc, const QmlJS::Snapshot &snapshot)
{
    QMutexLocker locker(&m_mutex);
    m_sourceDocument = doc;
    m_sourceSnapshot = snapshot;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::reupdate(const QmlJS::Snapshot &snapshot)
{
    QMutexLocker locker(&m_mutex);
    m_sourceDocument = m_lastSemanticInfo.document;
    m_sourceSnapshot = snapshot;
    m_condition.wakeOne();
}

void SemanticInfoUpdater::run()
{
    setPriority(QThread::LowestPriority);

    forever {
        m_mutex.lock();

        while (! (m_wasCancelled || m_sourceDocument))
            m_condition.wait(&m_mutex);

        const bool done = m_wasCancelled;
        QmlJS::Document::Ptr doc = m_sourceDocument;
        QmlJS::Snapshot snapshot = m_sourceSnapshot;
        m_sourceDocument.clear();
        m_sourceSnapshot = QmlJS::Snapshot();

        m_mutex.unlock();

        if (done)
            break;

        const QmlJSTools::SemanticInfo info = makeNewSemanticInfo(doc, snapshot);

        m_mutex.lock();
        const bool cancelledOrNewData = m_wasCancelled || m_sourceDocument;
        m_mutex.unlock();

        if (! cancelledOrNewData) {
            m_lastSemanticInfo = info;
            emit updated(info);
        }
    }
}

QmlJSTools::SemanticInfo SemanticInfoUpdater::makeNewSemanticInfo(const QmlJS::Document::Ptr &doc, const QmlJS::Snapshot &snapshot)
{
    using namespace QmlJS;

    QmlJSTools::SemanticInfo semanticInfo;
    semanticInfo.document = doc;
    semanticInfo.snapshot = snapshot;

    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    Link link(semanticInfo.snapshot, modelManager->defaultVContext(doc->language(), doc), modelManager->builtins(doc));
    semanticInfo.context = link(doc, &semanticInfo.semanticMessages);

    ScopeChain *scopeChain = new ScopeChain(doc, semanticInfo.context);
    semanticInfo.setRootScopeChain(QSharedPointer<const ScopeChain>(scopeChain));

    if (doc->language() == Dialect::Json) {
        Utils::JsonSchema *schema =
                QmlJSEditorPlugin::instance()->jsonManager()->schemaForFile(doc->fileName());
        if (schema) {
            JsonCheck jsonChecker(doc);
            semanticInfo.staticAnalysisMessages = jsonChecker(schema);
        }
    } else {
        Check checker(doc, semanticInfo.context);
        semanticInfo.staticAnalysisMessages = checker();
    }

    return semanticInfo;
}

} // namespace Internal
} // namespace QmlJSEditor
