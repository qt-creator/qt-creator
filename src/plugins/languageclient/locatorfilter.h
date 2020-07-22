/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include "client.h"

#include <coreplugin/locator/ilocatorfilter.h>
#include <languageserverprotocol/lsptypes.h>
#include <languageserverprotocol/languagefeatures.h>
#include <languageserverprotocol/workspace.h>

#include <QVector>

namespace Core { class IEditor; }

namespace LanguageClient {

class DocumentLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT
public:
    DocumentLocatorFilter();

    void updateCurrentClient();
    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(Core::LocatorFilterEntry selection,
                QString *newText,
                int *selectionStart,
                int *selectionLength) const override;
    void refresh(QFutureInterface<void> &future) override;

signals:
    void symbolsUpToDate(QPrivateSignal);

protected:
    QPointer<DocumentSymbolCache> m_symbolCache;
    LanguageServerProtocol::DocumentUri m_currentUri;

private:
    void updateSymbols(const LanguageServerProtocol::DocumentUri &uri,
                       const LanguageServerProtocol::DocumentSymbolsResult &symbols);
    void resetSymbols();

    template<class T>
    QList<Core::LocatorFilterEntry> generateEntries(const QList<T> &list, const QString &filter);

    QMutex m_mutex;
    QMetaObject::Connection m_updateSymbolsConnection;
    QMetaObject::Connection m_resetSymbolsConnection;
    Utils::optional<LanguageServerProtocol::DocumentSymbolsResult> m_currentSymbols;
};

class WorkspaceLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT
public:
    WorkspaceLocatorFilter();

    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(Core::LocatorFilterEntry selection,
                QString *newText,
                int *selectionStart,
                int *selectionLength) const override;
    void refresh(QFutureInterface<void> &future) override;

signals:
    void allRequestsFinished(QPrivateSignal);

protected:
    explicit WorkspaceLocatorFilter(const QVector<LanguageServerProtocol::SymbolKind> &filter);

private:
    void handleResponse(Client *client,
                        const LanguageServerProtocol::WorkspaceSymbolRequest::Response &response);

    QMutex m_mutex;
    QMap<Client *, LanguageServerProtocol::MessageId> m_pendingRequests;
    QVector<LanguageServerProtocol::SymbolInformation> m_results;
    QVector<LanguageServerProtocol::SymbolKind> m_filterKinds;
};

class WorkspaceClassLocatorFilter : public WorkspaceLocatorFilter
{
public:
    WorkspaceClassLocatorFilter();
};

class WorkspaceMethodLocatorFilter : public WorkspaceLocatorFilter
{
public:
    WorkspaceMethodLocatorFilter();
};

} // namespace LanguageClient
