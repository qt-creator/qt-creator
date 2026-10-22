// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "languageclient_global.h"

#include <languageserverprotocol/lsptypes.h>

#include <utils/filepath.h>
#include <utils/result.h>
#include <utils/searchresultitem.h>

#include <QHash>

#include <functional>

namespace LanguageClient {

class Client;

LANGUAGECLIENT_EXPORT bool hasChangeAnnotations(const LanguageServerProtocol::WorkspaceEdit &edit);

LANGUAGECLIENT_EXPORT void applyWorkspaceEditWithPreview(
    Client *client,
    const LanguageServerProtocol::WorkspaceEdit &edit,
    const QString &title,
    const std::function<void(bool applied)> &done = {});

void groupByAnnotation(
    Utils::SearchResultItem &item,
    const std::optional<LanguageServerProtocol::ChangeAnnotation> &annotation);

Utils::SearchResultItems workspaceEditPreviewItems(
    const LanguageServerProtocol::WorkspaceEdit &edit,
    const std::function<Utils::FilePath(const QString &uri)> &filePathFor);

LanguageServerProtocol::WorkspaceEdit selectedWorkspaceEdit(
    const LanguageServerProtocol::WorkspaceEdit &edit, const Utils::SearchResultItems &checkedItems);

QHash<Utils::FilePath, size_t> documentSnapshots(
    const LanguageServerProtocol::WorkspaceEdit &edit,
    const std::function<Utils::FilePath(const QString &uri)> &filePathFor,
    const std::function<QStringList(const Utils::FilePath &filePath)> &linesFor);

Utils::Result<> canApplySelectedEdit(
    const LanguageServerProtocol::WorkspaceEdit &selected,
    const QHash<Utils::FilePath, size_t> &previewedSnapshots,
    const std::function<Utils::FilePath(const QString &uri)> &filePathFor,
    const std::function<QStringList(const Utils::FilePath &filePath)> &linesFor);

#ifdef WITH_TESTS
QObject *createWorkspaceEditPreviewTest();
#endif

} // namespace LanguageClient
