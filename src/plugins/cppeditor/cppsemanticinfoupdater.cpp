// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppsemanticinfoupdater.h"

#include "cppmodelmanager.h"

#include <cplusplus/Control.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/TranslationUnit.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/async.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>

#include <QLoggingCategory>

enum { debug = 0 };

using namespace CPlusPlus;
using namespace Utils;

static Q_LOGGING_CATEGORY(log, "qtc.cppeditor.semanticinfoupdater", QtWarningMsg)

namespace CppEditor {

class SemanticInfoUpdaterPrivate
{
public:
    ~SemanticInfoUpdaterPrivate() { cancelFuture(); }

    void cancelFuture();

    SemanticInfo m_semanticInfo;
    std::unique_ptr<QFutureWatcher<SemanticInfo>> m_watcher;
};

void SemanticInfoUpdaterPrivate::cancelFuture()
{
    if (!m_watcher)
        return;

    m_watcher->cancel();
    m_watcher.reset();
}

class FuturizedTopLevelDeclarationProcessor: public TopLevelDeclarationProcessor
{
public:
    explicit FuturizedTopLevelDeclarationProcessor(const QFuture<void> &future): m_future(future) {}
    bool processDeclaration(DeclarationAST *) override { return !m_future.isCanceled(); }
private:
    QFuture<void> m_future;
};

static void doUpdate(QPromise<SemanticInfo> &promise, const SemanticInfo::Source &source)
{
    SemanticInfo newSemanticInfo;
    newSemanticInfo.revision = source.revision;
    newSemanticInfo.snapshot = source.snapshot;

    Document::Ptr doc = newSemanticInfo.snapshot.preprocessedDocument(
        source.code, FilePath::fromString(source.fileName));

    FuturizedTopLevelDeclarationProcessor processor(QFuture<void>(promise.future()));
    doc->control()->setTopLevelDeclarationProcessor(&processor);
    doc->check();
    if (promise.isCanceled())
        newSemanticInfo.complete = false;
    newSemanticInfo.doc = doc;

    qCDebug(log) << "update() for source revision:" << source.revision
                 << "canceled:" << !newSemanticInfo.complete;

    promise.addResult(newSemanticInfo);
}

static std::optional<SemanticInfo> canReuseSemanticInfo(
    const SemanticInfo &currentSemanticInfo, const SemanticInfo::Source &source)
{
    if (!source.force
            && currentSemanticInfo.complete
            && currentSemanticInfo.revision == source.revision
            && currentSemanticInfo.doc
            && currentSemanticInfo.doc->translationUnit()->ast()
            && currentSemanticInfo.doc->filePath().toString() == source.fileName
            && !currentSemanticInfo.snapshot.isEmpty()
            && currentSemanticInfo.snapshot == source.snapshot) {
        SemanticInfo newSemanticInfo;
        newSemanticInfo.revision = source.revision;
        newSemanticInfo.snapshot = source.snapshot;
        newSemanticInfo.doc = currentSemanticInfo.doc;
        qCDebug(log) << "re-using current semantic info, source revision:" << source.revision;
        return newSemanticInfo;
    }
    return {};
}

SemanticInfoUpdater::SemanticInfoUpdater()
    : d(new SemanticInfoUpdaterPrivate)
{}

SemanticInfoUpdater::~SemanticInfoUpdater() = default;

SemanticInfo SemanticInfoUpdater::update(const SemanticInfo::Source &source)
{
    qCDebug(log) << "update() - synchronous";
    d->cancelFuture();

    const auto info = canReuseSemanticInfo(d->m_semanticInfo, source);
    if (info) {
        d->m_semanticInfo = *info;
        return d->m_semanticInfo;
    }

    QPromise<SemanticInfo> dummy;
    dummy.start();
    doUpdate(dummy, source);
    const SemanticInfo result = dummy.future().result();
    d->m_semanticInfo = result;
    return result;
}

void SemanticInfoUpdater::updateDetached(const SemanticInfo::Source &source)
{
    qCDebug(log) << "updateDetached() - asynchronous";
    d->cancelFuture();

    const auto info = canReuseSemanticInfo(d->m_semanticInfo, source);
    if (info) {
        d->m_semanticInfo = *info;
        emit updated(d->m_semanticInfo);
        return;
    }

    d->m_watcher.reset(new QFutureWatcher<SemanticInfo>);
    connect(d->m_watcher.get(), &QFutureWatcherBase::finished, this, [this] {
        d->m_semanticInfo = d->m_watcher->result();
        emit updated(d->m_semanticInfo);
        d->m_watcher.release()->deleteLater();
    });
    const auto future = Utils::asyncRun(CppModelManager::sharedThreadPool(), doUpdate, source);
    d->m_watcher->setFuture(future);
    ExtensionSystem::PluginManager::futureSynchronizer()->addFuture(future);
}

SemanticInfo SemanticInfoUpdater::semanticInfo() const
{
    return d->m_semanticInfo;
}

} // namespace CppEditor
