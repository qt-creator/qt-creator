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

#include "clangstaticanalyzerprojectsettingswidget.h"
#include "ui_clangstaticanalyzerprojectsettingswidget.h"

#include "clangstaticanalyzerprojectsettings.h"
#include "clangstaticanalyzerprojectsettingsmanager.h"

#include <utils/qtcassert.h>

#include <QAbstractTableModel>

namespace ClangStaticAnalyzer {
namespace Internal {

class SuppressedDiagnosticsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SuppressedDiagnosticsModel(QObject *parent = 0) : QAbstractTableModel(parent) { }

    void setDiagnostics(const SuppressedDiagnosticsList &diagnostics);
    SuppressedDiagnostic diagnosticAt(int i) const;

private:
    enum Columns { ColumnFile, ColumnContext, ColumnDescription, ColumnLast = ColumnDescription };

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex & = QModelIndex()) const { return ColumnLast + 1; }
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    SuppressedDiagnosticsList m_diagnostics;
};

ProjectSettingsWidget::ProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ProjectSettingsWidget)
  , m_projectSettings(ProjectSettingsManager::getSettings(project))
{
    m_ui->setupUi(this);
    auto * const model = new SuppressedDiagnosticsModel(this);
    model->setDiagnostics(m_projectSettings->suppressedDiagnostics());
    connect(m_projectSettings, &ProjectSettings::suppressedDiagnosticsChanged,
            [model, this] {
                    model->setDiagnostics(m_projectSettings->suppressedDiagnostics());
                    updateButtonStates();
            });
    m_ui->diagnosticsView->setModel(model);
    updateButtonStates();
    connect(m_ui->diagnosticsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this](const QItemSelection &, const QItemSelection &) {
                updateButtonStateRemoveSelected();
            });
    connect(m_ui->removeSelectedButton, &QAbstractButton::clicked,
            [this](bool) { removeSelected(); });
    connect(m_ui->removeAllButton, &QAbstractButton::clicked,
            [this](bool) { m_projectSettings->removeAllSuppressedDiagnostics();});
}

ProjectSettingsWidget::~ProjectSettingsWidget()
{
    delete m_ui;
}

void ProjectSettingsWidget::updateButtonStates()
{
    updateButtonStateRemoveSelected();
    updateButtonStateRemoveAll();
}

void ProjectSettingsWidget::updateButtonStateRemoveSelected()
{
    const auto selectedRows = m_ui->diagnosticsView->selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() <= 1, return);
    m_ui->removeSelectedButton->setEnabled(!selectedRows.isEmpty());
}

void ProjectSettingsWidget::updateButtonStateRemoveAll()
{
    m_ui->removeAllButton->setEnabled(m_ui->diagnosticsView->model()->rowCount() > 0);
}

void ProjectSettingsWidget::removeSelected()
{
    const auto selectedRows = m_ui->diagnosticsView->selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() == 1, return);
    const auto * const model
            = static_cast<SuppressedDiagnosticsModel *>(m_ui->diagnosticsView->model());
    m_projectSettings->removeSuppressedDiagnostic(model->diagnosticAt(selectedRows.first().row()));
}


void SuppressedDiagnosticsModel::setDiagnostics(const SuppressedDiagnosticsList &diagnostics)
{
    beginResetModel();
    m_diagnostics = diagnostics;
    endResetModel();
}

SuppressedDiagnostic SuppressedDiagnosticsModel::diagnosticAt(int i) const
{
    return m_diagnostics.at(i);
}

int SuppressedDiagnosticsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_diagnostics.count();
}

QVariant SuppressedDiagnosticsModel::headerData(int section, Qt::Orientation orientation,
                                                int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == ColumnFile)
            return tr("File");
        if (section == ColumnContext)
            return tr("Context");
        if (section == ColumnDescription)
            return tr("Diagnostic");
    }
    return QVariant();
}

QVariant SuppressedDiagnosticsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || index.row() >= rowCount())
        return QVariant();
    const SuppressedDiagnostic &diag = m_diagnostics.at(index.row());
    if (index.column() == ColumnFile)
        return diag.filePath.toUserOutput();
    if (index.column() == ColumnContext) {
        if (diag.contextKind == QLatin1String("function") && !diag.context.isEmpty())
            return tr("Function \"%1\"").arg(diag.context);
        return QString();
    }
    if (index.column() == ColumnDescription)
        return diag.description;
    return QVariant();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer

#include "clangstaticanalyzerprojectsettingswidget.moc"
