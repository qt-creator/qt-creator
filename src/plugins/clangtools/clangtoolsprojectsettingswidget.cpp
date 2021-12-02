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

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "runsettingswidget.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/clangdiagnosticconfigsselectionwidget.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QAbstractTableModel>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>

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

enum { UseGlobalSettings, UseCustomSettings }; // Values in sync with m_globalCustomComboBox

ProjectSettingsWidget::ProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent) :
    QWidget(parent),
    m_projectSettings(ClangToolsProjectSettings::getSettings(project))
{
    m_globalCustomComboBox = new QComboBox;
    m_globalCustomComboBox->addItem(tr("Use Global Settings"));
    m_globalCustomComboBox->addItem(tr("Use Customized Settings"));

    m_restoreGlobal = new QPushButton(tr("Restore Global Settings"));

    auto gotoGlobalSettingsLabel =
            new QLabel("<a href=\"target\">" + tr("Open Global Settings") + "</a>");

    auto gotoAnalyzerModeLabel =
            new QLabel("<a href=\"target\">" + tr("Go to Analyzer") + "</a>");

    m_runSettingsWidget = new ClangTools::Internal::RunSettingsWidget(this);

    m_diagnosticsView = new QTreeView;
    m_diagnosticsView->setSelectionMode(QAbstractItemView::SingleSelection);

    m_removeSelectedButton = new QPushButton(tr("Remove Selected"), this);
    m_removeAllButton = new QPushButton(tr("Remove All"));

    using namespace Utils::Layouting;

    Column {
        Row {
            m_globalCustomComboBox,
            m_restoreGlobal,
            gotoGlobalSettingsLabel,
            Stretch(),
            gotoAnalyzerModeLabel
        },

        m_runSettingsWidget,

        Group {
            Title(tr("Suppressed diagnostics")),
            Row {
                m_diagnosticsView,
                Column {
                    m_removeSelectedButton,
                    m_removeAllButton,
                    Stretch()
                }
            }
        }
    }.attachTo(this, false);

    // Use global/custom settings combo box
    const int globalOrCustomIndex = m_projectSettings->useGlobalSettings() ? UseGlobalSettings
                                                                           : UseCustomSettings;
    m_globalCustomComboBox->setCurrentIndex(globalOrCustomIndex);
    onGlobalCustomChanged(globalOrCustomIndex);
    connect(m_globalCustomComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            QOverload<int>::of(&ProjectSettingsWidget::onGlobalCustomChanged));

    // Global settings
    connect(ClangToolsSettings::instance(),
            &ClangToolsSettings::changed,
            this,
            QOverload<>::of(&ProjectSettingsWidget::onGlobalCustomChanged));
    connect(m_restoreGlobal, &QPushButton::clicked, this, [this]() {
        m_runSettingsWidget->fromSettings(ClangToolsSettings::instance()->runSettings());
    });

    // Links
    connect(gotoGlobalSettingsLabel, &QLabel::linkActivated, [](const QString &) {
        Core::ICore::showOptionsDialog(ClangTools::Constants::SETTINGS_PAGE_ID);
    });

    connect(gotoAnalyzerModeLabel, &QLabel::linkActivated, [](const QString &) {
        ClangTool::instance()->selectPerspective();
    });

    // Run options
    connect(m_runSettingsWidget, &RunSettingsWidget::changed, [this]() {
        // Save project run settings
        m_projectSettings->setRunSettings(m_runSettingsWidget->toSettings());

        // Save global custom configs
        const CppEditor::ClangDiagnosticConfigs configs
            = m_runSettingsWidget->diagnosticSelectionWidget()->customConfigs();
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
    m_diagnosticsView->setModel(model);
    updateButtonStates();
    connect(m_diagnosticsView->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this](const QItemSelection &, const QItemSelection &) {
                updateButtonStateRemoveSelected();
            });
    connect(m_removeSelectedButton, &QAbstractButton::clicked,
            [this](bool) { removeSelected(); });
    connect(m_removeAllButton, &QAbstractButton::clicked,
            [this](bool) { m_projectSettings->removeAllSuppressedDiagnostics();});
}

void ProjectSettingsWidget::onGlobalCustomChanged()
{
    onGlobalCustomChanged(m_globalCustomComboBox->currentIndex());
}

void ProjectSettingsWidget::onGlobalCustomChanged(int index)
{
    const bool useGlobal = index == UseGlobalSettings;
    const RunSettings runSettings = useGlobal ? ClangToolsSettings::instance()->runSettings()
                                              : m_projectSettings->runSettings();
    m_runSettingsWidget->fromSettings(runSettings);
    m_runSettingsWidget->setEnabled(!useGlobal);
    m_restoreGlobal->setEnabled(!useGlobal);

    m_projectSettings->setUseGlobalSettings(useGlobal);
}

void ProjectSettingsWidget::updateButtonStates()
{
    updateButtonStateRemoveSelected();
    updateButtonStateRemoveAll();
}

void ProjectSettingsWidget::updateButtonStateRemoveSelected()
{
    const auto selectedRows = m_diagnosticsView->selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() <= 1, return);
    m_removeSelectedButton->setEnabled(!selectedRows.isEmpty());
}

void ProjectSettingsWidget::updateButtonStateRemoveAll()
{
    m_removeAllButton->setEnabled(m_diagnosticsView->model()->rowCount() > 0);
}

void ProjectSettingsWidget::removeSelected()
{
    const auto selectedRows = m_diagnosticsView->selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() == 1, return);
    const auto * const model
            = static_cast<SuppressedDiagnosticsModel *>(m_diagnosticsView->model());
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
