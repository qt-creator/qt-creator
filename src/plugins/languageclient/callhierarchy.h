// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

namespace Core { class IDocument; }

namespace LanguageClient {

class Client;

void setupCallHierarchyFactory();
bool supportsCallHierarchy(Client *client, const Core::IDocument *document);

} // namespace LanguageClient
