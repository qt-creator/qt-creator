// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllsclient.h"

#include "qmljseditortr.h"

#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <QLoggingCategory>

using namespace LanguageClient;
using namespace Utils;

namespace {
Q_LOGGING_CATEGORY(qmllsLog, "qtc.qmlls.client", QtWarningMsg);
}

namespace QmlJSEditor {

static QHash<FilePath, QmllsClient *> &qmllsClients()
{
    static QHash<FilePath, QmllsClient *> clients;
    return clients;
}

QmllsClient *QmllsClient::clientForQmlls(const FilePath &qmlls)
{
    if (auto client = qmllsClients()[qmlls]) {
        switch (client->state()) {
        case Client::State::Uninitialized:
        case Client::State::InitializeRequested:
        case Client::State::Initialized:
            return client;
        case Client::State::FailedToInitialize:
        case Client::State::ShutdownRequested:
        case Client::State::Shutdown:
        case Client::State::Error:
            qCDebug(qmllsLog) << "client was stopping or failed, restarting";
            break;
        }
    }
    auto interface = new StdIOClientInterface;
    interface->setCommandLine(CommandLine(qmlls));
    auto client = new QmllsClient(interface);
    client->setName(Tr::tr("Qmlls (%1)").arg(qmlls.toUserOutput()));
    client->setActivateDocumentAutomatically(true);
    LanguageFilter filter;
    using namespace QmlJSTools::Constants;
    filter.mimeTypes = QStringList()
                       << QML_MIMETYPE << QMLUI_MIMETYPE << QBS_MIMETYPE << QMLPROJECT_MIMETYPE
                       << QMLTYPES_MIMETYPE << JS_MIMETYPE << JSON_MIMETYPE;
    client->setSupportedLanguage(filter);
    client->start();
    qmllsClients()[qmlls] = client;
    return client;
}

QmllsClient::QmllsClient(StdIOClientInterface *interface)
    : Client(interface)
{
}

QmllsClient::~QmllsClient()
{
    qmllsClients().remove(qmllsClients().key(this));
}

} // namespace QmlJSEditor
