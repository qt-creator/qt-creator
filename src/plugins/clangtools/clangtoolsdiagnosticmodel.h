/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangfixitsrefactoringchanges.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsprojectsettings.h"

#include <debugger/analyzer/detailederrorview.h>
#include <utils/fileutils.h>
#include <utils/treemodel.h>

#include <QFileSystemWatcher>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QVector>

#include <functional>
#include <map>
#include <memory>

namespace ProjectExplorer { class Project; }

namespace ClangTools {
namespace Internal {

enum class FixitStatus {
    NotAvailable,
    NotScheduled,
    Scheduled,
    Applied,
    FailedToApply,
    Invalidated,
};

class ClangToolsDiagnosticModel;

class FilePathItem : public Utils::TreeItem
{
public:
    FilePathItem(const QString &filePath);
    QVariant data(int column, int role) const override;

private:
    const QString m_filePath;
};

class DiagnosticItem : public Utils::TreeItem
{
public:
    using OnFixitStatusChanged = std::function<void(FixitStatus newStatus)>;
    DiagnosticItem(const Diagnostic &diag, const OnFixitStatusChanged &onFixitStatusChanged,
                   ClangToolsDiagnosticModel *parent);
    ~DiagnosticItem() override;

    const Diagnostic &diagnostic() const { return m_diagnostic; }

    FixitStatus fixItStatus() const { return m_fixitStatus; }
    void setFixItStatus(const FixitStatus &status);

    bool hasNewFixIts() const;
    ReplacementOperations &fixitOperations() { return m_fixitOperations; }
    void setFixitOperations(const ReplacementOperations &replacements);

    bool setData(int column, const QVariant &data, int role) override;

private:
    Qt::ItemFlags flags(int column) const override;
    QVariant data(int column, int role) const override;

private:
    const Diagnostic m_diagnostic;
    OnFixitStatusChanged m_onFixitStatusChanged;

    ReplacementOperations  m_fixitOperations;
    FixitStatus m_fixitStatus = FixitStatus::NotAvailable;
    ClangToolsDiagnosticModel *m_parentModel = nullptr;
};

class ExplainingStepItem;

using ClangToolsDiagnosticModelBase
    = Utils::TreeModel<Utils::TreeItem, FilePathItem, DiagnosticItem, ExplainingStepItem>;
class ClangToolsDiagnosticModel : public ClangToolsDiagnosticModelBase
{
    Q_OBJECT

    friend class DiagnosticItem;

public:
    ClangToolsDiagnosticModel(QObject *parent = nullptr);

    void addDiagnostics(const QList<Diagnostic> &diagnostics);
    QSet<Diagnostic> diagnostics() const;

    enum ItemRole {
        DiagnosticRole = Debugger::DetailedErrorView::FullTextRole + 1,
        TextRole,
        CheckBoxEnabledRole
    };

    void clear();
    void removeWatchedPath(const QString &path);
    void addWatchedPath(const QString &path);

signals:
    void fixItsToApplyCountChanged(int count);

private:
    void connectFileWatcher();
    void updateItems(const DiagnosticItem *changedItem);
    void onFileChanged(const QString &path);
    void clearAndSetupCache();

private:
    QHash<QString, FilePathItem *> m_filePathToItem;
    QSet<Diagnostic> m_diagnostics;
    std::map<QVector<ExplainingStep>, QVector<DiagnosticItem *>> stepsToItemsCache;
    std::unique_ptr<QFileSystemWatcher> m_filesWatcher;
    int m_fixItsToApplyCount = 0;
};

class DiagnosticFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    DiagnosticFilterModel(QObject *parent = nullptr);

    void setProject(ProjectExplorer::Project *project);
    void addSuppressedDiagnostic(const SuppressedDiagnostic &diag);
    ProjectExplorer::Project *project() const { return m_project; }

    void invalidateFilter();

private:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &l, const QModelIndex &r) const override;
    void handleSuppressedDiagnosticsChanged();

    QPointer<ProjectExplorer::Project> m_project;
    Utils::FileName m_lastProjectDirectory;
    SuppressedDiagnosticsList m_suppressedDiagnostics;
};

} // namespace Internal
} // namespace ClangTools
