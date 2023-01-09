// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppcheck/cppcheckdiagnostic.h>
#include <cppcheck/cppcheckdiagnosticmanager.h>

#include <debugger/analyzer/detailederrorview.h>

#include <utils/treemodel.h>

namespace Cppcheck::Internal {

class DiagnosticsModel;

class FilePathItem : public Utils::TreeItem
{
public:
    explicit FilePathItem(const QString &filePath);
    QVariant data(int column, int role) const override;

private:
    const QString m_filePath;
};

class DiagnosticItem : public Utils::TreeItem
{
public:
    explicit DiagnosticItem(const Diagnostic &diagnostic);
    QVariant data(int column, int role) const override;

private:
    const Diagnostic m_diagnostic;
};

using BaseModel = Utils::TreeModel<Utils::TreeItem, FilePathItem, DiagnosticItem>;

class DiagnosticsModel : public BaseModel, public CppcheckDiagnosticManager
{
    Q_OBJECT
public:
    enum Column {DiagnosticColumn};

    explicit DiagnosticsModel(QObject *parent = nullptr);
    void clear();
    void add(const Diagnostic &diagnostic) override;

signals:
    void hasDataChanged(bool hasData);

private:
    QHash<QString, FilePathItem *> m_filePathToItem;
    QSet<Diagnostic> m_diagnostics;
};

} // Cppcheck::Internal
