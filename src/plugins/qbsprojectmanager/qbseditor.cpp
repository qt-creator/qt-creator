// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qbseditor.h"

#include "qbslanguageclient.h"
#include "qbsprojectmanagertr.h"

#include <languageclient/languageclientmanager.h>
#include <utils/mimeconstants.h>

#include <QPointer>

using namespace LanguageClient;
using namespace QmlJSEditor;
using namespace Utils;

namespace QbsProjectManager::Internal {

class QbsEditorWidget : public QmlJSEditorWidget
{
private:
    void findLinkAt(const QTextCursor &cursor,
                    const LinkHandler &processLinkCallback,
                    bool resolveTarget = true,
                    bool inNextSplit = false) override;
};

QbsEditorFactory::QbsEditorFactory() : QmlJSEditorFactory("QbsEditor.QbsEditor")
{
    setDisplayName(Tr::tr("Qbs Editor"));
    setMimeTypes({Utils::Constants::QBS_MIMETYPE});
    setEditorWidgetCreator([] { return new QbsEditorWidget; });
}

void QbsEditorWidget::findLinkAt(const QTextCursor &cursor, const LinkHandler &processLinkCallback,
                                 bool resolveTarget, bool inNextSplit)
{
    const LinkHandler extendedCallback = [self = QPointer(this), cursor, processLinkCallback,
                                          resolveTarget](const Link &link) {
        if (link.hasValidTarget())
            return processLinkCallback(link);
        if (!self)
            return;
        const auto doc = self->textDocument();
        if (!doc)
            return;
        const QList<Client *> &candidates = LanguageClientManager::clientsSupportingDocument(doc);
        for (Client * const candidate : candidates) {
            const auto qbsClient = qobject_cast<QbsLanguageClient *>(candidate);
            if (!qbsClient || !qbsClient->isActive() || !qbsClient->documentOpen(doc))
                continue;
            qbsClient->findLinkAt(doc, cursor, processLinkCallback, resolveTarget,
                               LinkTarget::SymbolDef);
        }
    };
    QmlJSEditorWidget::findLinkAt(cursor, extendedCallback, resolveTarget, inNextSplit);
}

} // namespace QbsProjectManager::Internal
