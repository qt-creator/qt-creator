// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljseditor/qmljsfindreferences.h>

namespace QmlJSEditor {
namespace Internal {

class FileComponentRenameHandler : public QObject
{
    Q_OBJECT
public:
    static FileComponentRenameHandler *instance();

    void handleFilesRenamed(const Utils::FilePairs &renames);

signals:
    void usageResultsReady(const QList<QmlJSEditor::FindReferences::Usage>& usages,
                           const QString oldBaseName,
                           const QString &newBaseName);

private:
    explicit FileComponentRenameHandler(QObject *parent = nullptr);

    void renameFileComponentUsages(const Utils::FilePath &oldPath, const Utils::FilePath &newPath);
    void onUsageResultsReady(const QList<QmlJSEditor::FindReferences::Usage>& usages,
                             const QString oldBaseName,
                             const QString &newBaseName);
};

} // namespace Internal
} // namespace QmlJSEditor
