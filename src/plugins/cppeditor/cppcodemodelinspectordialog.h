/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPCODEMODELINSPECTORDIALOG_H
#define CPPCODEMODELINSPECTORDIALOG_H

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

private slots:
    void onRefreshRequested();

    void onSnapshotFilterChanged(const QString &pattern);
    void onSnapshotSelected(int row);
    void onDocumentSelected(const QModelIndex &current, const QModelIndex &);
    void onSymbolsViewExpandedOrCollapsed(const QModelIndex &);

    void onProjectPartFilterChanged(const QString &pattern);
    void onProjectPartSelected(const QModelIndex &current, const QModelIndex &);

    void onWorkingCopyFilterChanged(const QString &pattern);
    void onWorkingCopyDocumentSelected(const QModelIndex &current, const QModelIndex &);

private:
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

    // Working Copy
    FilterableView *m_workingCopyView;
    WorkingCopyModel *m_workingCopyModel;
    QSortFilterProxyModel *m_proxyWorkingCopyModel;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPCODEMODELINSPECTORDIALOG_H
