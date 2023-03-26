// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageclient/client.h>

namespace Coco {

class CocoLanguageClient : public LanguageClient::Client
{
public:
    CocoLanguageClient(const Utils::FilePath &coco, const Utils::FilePath &csmes);
    ~CocoLanguageClient() override;

    LanguageClient::BaseClientInterface *clientInterface(const Utils::FilePath &coco,
                                                         const Utils::FilePath &csmes);

protected:
    LanguageClient::DiagnosticManager *createDiagnosticManager() override;
    void handleDiagnostics(const LanguageServerProtocol::PublishDiagnosticsParams &params) override;

private:
    void initClientCapabilities();
    void onDocumentOpened(Core::IDocument *document);
    void onDocumentClosed(Core::IDocument *document);
    void handleEditorOpened(Core::IEditor *editor);
};

} // namespace Coco
