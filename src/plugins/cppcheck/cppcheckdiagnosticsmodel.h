/****************************************************************************
**
** Copyright (C) 2019 Sergey Morozov
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

#include <cppcheck/cppcheckdiagnostic.h>
#include <cppcheck/cppcheckdiagnosticmanager.h>

#include <debugger/analyzer/detailederrorview.h>

#include <utils/treemodel.h>

namespace Cppcheck {
namespace Internal {

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

} // namespace Internal
} // namespace Cppcheck
