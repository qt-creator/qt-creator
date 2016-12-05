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

#include <cpptools/cppmodelmanager.h>

#include <cplusplus/CppDocument.h>

#include <QDialog>
#include <QList>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QModelIndex;
namespace Ui { class CppCodeModelInspectorDialog; }
QT_END_NAMESPACE

namespace CppEditor {
namespace Internal {

class FilterableView;
class SnapshotInfo;

class DiagnosticMessagesModel;
class IncludesModel;
class KeyValueModel;
class MacrosModel;
class ProjectPartsModel;
class ProjectFilesModel;
class ProjectHeaderPathsModel;
class SnapshotModel;
class SymbolsModel;
class TokensModel;
class WorkingCopyModel;

//
// This dialog is for DEBUGGING PURPOSES and thus NOT TRANSLATED.
//

class CppCodeModelInspectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CppCodeModelInspectorDialog(QWidget *parent = 0);
    ~CppCodeModelInspectorDialog();

private:
    void onRefreshRequested();

    void onSnapshotFilterChanged(const QString &pattern);
    void onSnapshotSelected(int row);
    void onDocumentSelected(const QModelIndex &current, const QModelIndex &);
    void onSymbolsViewExpandedOrCollapsed(const QModelIndex &);

    void onProjectPartFilterChanged(const QString &pattern);
    void onProjectPartSelected(const QModelIndex &current, const QModelIndex &);

    void onWorkingCopyFilterChanged(const QString &pattern);
    void onWorkingCopyDocumentSelected(const QModelIndex &current, const QModelIndex &);

    void refresh();

    void clearDocumentData();
    void updateDocumentData(const CPlusPlus::Document::Ptr &document);

    void clearProjectPartData();
    void updateProjectPartData(const CppTools::ProjectPart::Ptr &part);

    bool event(QEvent *e);

private:
    Ui::CppCodeModelInspectorDialog *m_ui;

    // Snapshots and Documents
    QList<SnapshotInfo> *m_snapshotInfos;
    FilterableView *m_snapshotView;
    SnapshotModel *m_snapshotModel;
    QSortFilterProxyModel *m_proxySnapshotModel;
    KeyValueModel *m_docGenericInfoModel;
    IncludesModel *m_docIncludesModel;
    DiagnosticMessagesModel *m_docDiagnosticMessagesModel;
    MacrosModel *m_docMacrosModel;
    SymbolsModel *m_docSymbolsModel;
    TokensModel *m_docTokensModel;

    // Project Parts
    FilterableView *m_projectPartsView;
    ProjectPartsModel *m_projectPartsModel;
    QSortFilterProxyModel *m_proxyProjectPartsModel;
    KeyValueModel *m_partGenericInfoModel;
    ProjectFilesModel *m_projectFilesModel;
    ProjectHeaderPathsModel *m_projectHeaderPathsModel;

    // Working Copy
    FilterableView *m_workingCopyView;
    WorkingCopyModel *m_workingCopyModel;
    QSortFilterProxyModel *m_proxyWorkingCopyModel;
};

} // namespace Internal
} // namespace CppEditor
