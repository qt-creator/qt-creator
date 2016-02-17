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

#include "cppsemanticinfoupdater.h"

#include "cpplocalsymbols.h"
#include "cppmodelmanager.h"

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <cplusplus/Control.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/TranslationUnit.h>

#include <QLoggingCategory>

enum { debug = 0 };

using namespace CPlusPlus;
using namespace CppTools;

static Q_LOGGING_CATEGORY(log, "qtc.cpptools.semanticinfoupdater")

namespace CppTools {

class SemanticInfoUpdaterPrivate
{
public:
    class FuturizedTopLevelDeclarationProcessor: public TopLevelDeclarationProcessor
    {
    public:
        FuturizedTopLevelDeclarationProcessor(QFutureInterface<void> &future): m_future(future) {}
        bool processDeclaration(DeclarationAST *) { return !isCanceled(); }
        bool isCanceled() { return m_future.isCanceled(); }
    private:
        QFutureInterface<void> m_future;
    };

public:
    SemanticInfoUpdaterPrivate(SemanticInfoUpdater *q);
    ~SemanticInfoUpdaterPrivate();

    SemanticInfo semanticInfo() const;
    void setSemanticInfo(const SemanticInfo &semanticInfo, bool emitSignal);

    SemanticInfo update(const SemanticInfo::Source &source,
                        bool emitSignalWhenFinished,
                        FuturizedTopLevelDeclarationProcessor *processor);

    bool reuseCurrentSemanticInfo(const SemanticInfo::Source &source, bool emitSignalWhenFinished);

    void update_helper(QFutureInterface<void> &future, const SemanticInfo::Source source);

public:
    SemanticInfoUpdater *q;
    mutable QMutex m_lock;
    SemanticInfo m_semanticInfo;
    QFuture<void> m_future;
};

SemanticInfoUpdaterPrivate::SemanticInfoUpdaterPrivate(SemanticInfoUpdater *q)
    : q(q)
{
}

SemanticInfoUpdaterPrivate::~SemanticInfoUpdaterPrivate()
{
    m_future.cancel();
    m_future.waitForFinished();
}

SemanticInfo SemanticInfoUpdaterPrivate::semanticInfo() const
{
    QMutexLocker locker(&m_lock);
    return m_semanticInfo;
}

void SemanticInfoUpdaterPrivate::setSemanticInfo(const SemanticInfo &semanticInfo, bool emitSignal)
{
    {
        QMutexLocker locker(&m_lock);
        m_semanticInfo = semanticInfo;
    }
    if (emitSignal) {
        qCDebug(log) << "emiting new info";
        emit q->updated(semanticInfo);
    }
}

SemanticInfo SemanticInfoUpdaterPrivate::update(const SemanticInfo::Source &source,
                                                bool emitSignalWhenFinished,
                                                FuturizedTopLevelDeclarationProcessor *processor)
{
    SemanticInfo newSemanticInfo;
    newSemanticInfo.revision = source.revision;
    newSemanticInfo.snapshot = source.snapshot;

    Document::Ptr doc = newSemanticInfo.snapshot.preprocessedDocument(source.code, source.fileName);
    if (processor)
        doc->control()->setTopLevelDeclarationProcessor(processor);
    doc->check();
    if (processor && processor->isCanceled())
        newSemanticInfo.complete = false;
    newSemanticInfo.doc = doc;

    qCDebug(log) << "update() for source revision:" << source.revision
                 << "canceled:" << !newSemanticInfo.complete;

    setSemanticInfo(newSemanticInfo, emitSignalWhenFinished);
    return newSemanticInfo;
}

bool SemanticInfoUpdaterPrivate::reuseCurrentSemanticInfo(const SemanticInfo::Source &source,
                                                          bool emitSignalWhenFinished)
{
    const SemanticInfo currentSemanticInfo = semanticInfo();

    if (!source.force
            && currentSemanticInfo.complete
            && currentSemanticInfo.revision == source.revision
            && currentSemanticInfo.doc
            && currentSemanticInfo.doc->translationUnit()->ast()
            && currentSemanticInfo.doc->fileName() == source.fileName
            && !currentSemanticInfo.snapshot.isEmpty()
            && currentSemanticInfo.snapshot == source.snapshot) {
        SemanticInfo newSemanticInfo;
        newSemanticInfo.revision = source.revision;
        newSemanticInfo.snapshot = source.snapshot;
        newSemanticInfo.doc = currentSemanticInfo.doc;
        setSemanticInfo(newSemanticInfo, emitSignalWhenFinished);
        qCDebug(log) << "re-using current semantic info, source revision:" << source.revision;
        return true;
    }

    return false;
}

void SemanticInfoUpdaterPrivate::update_helper(QFutureInterface<void> &future,
                                               const SemanticInfo::Source source)
{
    FuturizedTopLevelDeclarationProcessor processor(future);
    update(source, true, &processor);
}

SemanticInfoUpdater::SemanticInfoUpdater()
    : d(new SemanticInfoUpdaterPrivate(this))
{
}

SemanticInfoUpdater::~SemanticInfoUpdater()
{
    d->m_future.cancel();
    d->m_future.waitForFinished();
}

SemanticInfo SemanticInfoUpdater::update(const SemanticInfo::Source &source)
{
    qCDebug(log) << "update() - synchronous";
    d->m_future.cancel();

    const bool emitSignalWhenFinished = false;
    if (d->reuseCurrentSemanticInfo(source, emitSignalWhenFinished)) {
        d->m_future = QFuture<void>();
        return semanticInfo();
    }

    return d->update(source, emitSignalWhenFinished, 0);
}

void SemanticInfoUpdater::updateDetached(const SemanticInfo::Source source)
{
    qCDebug(log) << "updateDetached() - asynchronous";
    d->m_future.cancel();

    const bool emitSignalWhenFinished = true;
    if (d->reuseCurrentSemanticInfo(source, emitSignalWhenFinished)) {
        d->m_future = QFuture<void>();
        return;
    }

    d->m_future = Utils::runAsync(CppModelManager::instance()->sharedThreadPool(),
                                  &SemanticInfoUpdaterPrivate::update_helper, d.data(), source);
}

SemanticInfo SemanticInfoUpdater::semanticInfo() const
{
    return d->semanticInfo();
}

} // namespace CppTools
