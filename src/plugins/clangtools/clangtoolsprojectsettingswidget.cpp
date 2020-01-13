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

#include "clangtoolsprojectsettingswidget.h"
#include "ui_clangtoolsprojectsettingswidget.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"

#include <coreplugin/icore.h>
#include <cpptools/clangdiagnosticconfigsselectionwidget.h>

#include <utils/qtcassert.h>

#include <QAbstractTableModel>

#include <cpptools/clangdiagnosticconfigsmodel.h>

namespace ClangTools {
namespace Internal {

class SuppressedDiagnosticsModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    SuppressedDiagnosticsModel(QObject *parent = nullptr) : QAbstractTableModel(parent) { }

    void setDiagnostics(const SuppressedDiagnosticsList &diagnostics);
    SuppressedDiagnostic diagnosticAt(int i) const;

private:
    enum Columns { ColumnFile, ColumnDescription, ColumnLast = ColumnDescription };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override { return ColumnLast + 1; }
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    SuppressedDiagnosticsList m_diagnostics;
};

enum { UseGlobalSettings, UseCustomSettings }; // Values in sync with m_ui->globalCustomComboBox

ProjectSettingsWidget::ProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ProjectSettingsWidget)
  , m_projectSettings(ClangToolsProjectSettings::getSettings(project))
{
    m_ui->setupUi(this);

    // Use global/custom settings combo box
    const int globalOrCustomIndex = m_projectSettings->useGlobalSettings() ? UseGlobalSettings
                                                                           : UseCustomSettings;
    m_ui->globalCustomComboBox->setCurrentIndex(globalOrCustomIndex);
    onGlobalCustomChanged(globalOrCustomIndex);
    connect(m_ui->globalCustomComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            QOverload<int>::of(&ProjectSettingsWidget::onGlobalCustomChanged));

    // Global settings
    connect(ClangToolsSettings::instance(),
            &ClangToolsSettings::changed,
            this,
            QOverload<>::of(&ProjectSettingsWidget::onGlobalCustomChanged));
    connect(m_ui->restoreGlobal, &QPushButton::clicked, this, [this]() {
        m_ui->runSettingsWidget->fromSettings(ClangToolsSettings::instance()->runSettings());
    });

    // Links
    connect(m_ui->gotoGlobalSettingsLabel, &QLabel::linkActivated, [](const QString &) {
        Core::ICore::showOptionsDialog(ClangTools::Constants::SETTINGS_PAGE_ID);
    });

    connect(m_ui->gotoAnalyzerModeLabel, &QLabel::linkActivated, [](const QString &) {
        ClangTool::instance()->selectPerspective();
    });

    // Run options
    connect(m_ui->runSettingsWidget, &RunSettingsWidget::changed, [this]() {
        // Save project run settings
        m_projectSettings->setRunSettings(m_ui->runSettingsWidget->toSettings());

        // Save global custom configs
        const CppTools::ClangDiagnosticConfigs configs
            = m_ui->runSettingsWidget->diagnosticSelectionWidget()->customConfigs();
        ClangToolsSettings::instance()->setDiagnosticConfigs(configs);
        ClangToolsSettings::instance()->writeSettings();
    });

    // Suppressed diagnostics
    auto * const model = new SuppressedDiagnosticsModel(this);
    model->setDiagnostics(m_projectSettings->suppressedDiagnostics());
    connect(m_projectSettings.data(), &ClangToolsProjectSettings::suppressedDiagnosticsChanged,
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

void ProjectSettingsWidget::onGlobalCustomChanged()
{
    onGlobalCustomChanged(m_ui->globalCustomComboBox->currentIndex());
}

void ProjectSettingsWidget::onGlobalCustomChanged(int index)
{
    const bool useGlobal = index == UseGlobalSettings;
    const RunSettings runSettings = useGlobal ? ClangToolsSettings::instance()->runSettings()
                                              : m_projectSettings->runSettings();
    m_ui->runSettingsWidget->fromSettings(runSettings);
    m_ui->runSettingsWidget->setEnabled(!useGlobal);
    m_ui->restoreGlobal->setEnabled(!useGlobal);

    m_projectSettings->setUseGlobalSettings(useGlobal);
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
    if (index.column() == ColumnDescription)
        return diag.description;
    return QVariant();
}

} // namespace Internal
} // namespace ClangTools

#include "clangtoolsprojectsettingswidget.moc"
