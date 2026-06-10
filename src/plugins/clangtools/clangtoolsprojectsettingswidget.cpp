// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsprojectsettingswidget.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolssettings.h"
#include "clangtoolstr.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>

#include <projectexplorer/projectpanelfactory.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QAbstractTableModel>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTreeView>

using namespace ProjectExplorer;

namespace ClangTools::Internal {

class SuppressedDiagnosticsModel : public QAbstractTableModel
{
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

class ClangToolsProjectSettingsWidget : public QWidget
{
public:
    explicit ClangToolsProjectSettingsWidget(Project *project);

private:
    void onGlobalCustomChanged(bool useGlobal);

    void updateButtonStates();
    void updateButtonStateRemoveSelected();
    void updateButtonStateRemoveAll();
    void removeSelected();

    QPushButton m_restoreGlobal{Tr::tr("Restore Global Settings")};
    QTreeView m_diagnosticsView;
    QPushButton m_removeSelectedButton{Tr::tr("Remove Selected")};
    QPushButton m_removeAllButton{Tr::tr("Remove All")};

    std::shared_ptr<ClangToolsProjectSettings> const m_projectSettings;
};

ClangToolsProjectSettingsWidget::ClangToolsProjectSettingsWidget(Project *project)
    : m_projectSettings(ClangToolsProjectSettings::getSettings(project))
{
    const auto gotoClangTidyModeLabel
            = new QLabel("<a href=\"target\">" + Tr::tr("Go to Clang-Tidy") + "</a>");
    const auto gotoClazyModeLabel
            = new QLabel("<a href=\"target\">" + Tr::tr("Go to Clazy") + "</a>");

    m_diagnosticsView.setSelectionMode(QAbstractItemView::SingleSelection);

    m_projectSettings->useGlobalSettings.addOnChanged(this, [this] {
        onGlobalCustomChanged(m_projectSettings->useGlobalSettings());
    });

    using namespace Layouting;
    Column {
        Row {
            m_projectSettings->useGlobalSettings,
            createUseGlobalSettingsLabel(ClangTools::Constants::SETTINGS_PAGE_ID), st
        },
        createHr(),
        Row {
            &m_restoreGlobal, st, gotoClangTidyModeLabel, gotoClazyModeLabel
        },

        *m_projectSettings,

        Group {
            title(Tr::tr("Suppressed diagnostics")),
            Row {
                &m_diagnosticsView,
                Column {
                    &m_removeSelectedButton,
                    &m_removeAllButton,
                    st
                }
            }
        },
        noMargin,
        st
    }.attachTo(this);

    onGlobalCustomChanged(m_projectSettings->useGlobalSettings());

    // Global settings
    connect(ClangToolsSettings::instance(), &ClangToolsSettings::changed, this,
            [this] { onGlobalCustomChanged(m_projectSettings->useGlobalSettings()); });
    connect(&m_restoreGlobal, &QPushButton::clicked, this, [this] {
        const ClangToolsSettings *global = ClangToolsSettings::instance();
        m_projectSettings->diagnosticConfigId.setValue(global->safeDiagnosticConfigId());
        m_projectSettings->parallelJobs.setValue(global->parallelJobs());
        m_projectSettings->preferConfigFile.setValue(global->preferConfigFile());
        m_projectSettings->buildBeforeAnalysis.setValue(global->buildBeforeAnalysis());
        m_projectSettings->analyzeOpenFiles.setValue(global->analyzeOpenFiles());
    });

    connect(gotoClangTidyModeLabel, &QLabel::linkActivated, [](const QString &) {
        clangTidyTool()->selectPerspective();
    });
    connect(gotoClazyModeLabel, &QLabel::linkActivated, [](const QString &) {
        clazyTool()->selectPerspective();
    });

    // The run settings aspects (including diagnosticConfigId) auto-apply since
    // ClangToolsProjectSettings::runSettings has autoApply=true.
    // Save custom diagnostic configs to global settings when the aspect changes.
    connect(&m_projectSettings->diagnosticConfigId,
            &Utils::BaseAspect::changed, this, [this] {
                ClangToolsSettings::instance()->diagnosticConfigId.setCustomConfigs(
                    m_projectSettings->diagnosticConfigId.customConfigs());
                ClangToolsSettings::instance()->writeSettings();
            });

    // Suppressed diagnostics
    auto * const model = new SuppressedDiagnosticsModel(this);
    model->setDiagnostics(m_projectSettings->suppressedDiagnostics());
    connect(m_projectSettings.get(), &ClangToolsProjectSettings::suppressedDiagnosticsChanged, this,
            [model, this] {
                    model->setDiagnostics(m_projectSettings->suppressedDiagnostics());
                    updateButtonStates();
            });
    m_diagnosticsView.setModel(model);
    updateButtonStates();
    connect(m_diagnosticsView.selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) {
                updateButtonStateRemoveSelected();
            });
    connect(&m_removeSelectedButton, &QAbstractButton::clicked,
            this, [this](bool) { removeSelected(); });
    connect(&m_removeAllButton, &QAbstractButton::clicked,
            this, [this](bool) { m_projectSettings->removeAllSuppressedDiagnostics();});
}

void ClangToolsProjectSettingsWidget::onGlobalCustomChanged(bool useGlobal)
{
    m_projectSettings->setRunSettingsEnabled(!useGlobal);
    m_restoreGlobal.setEnabled(!useGlobal);
}

void ClangToolsProjectSettingsWidget::updateButtonStates()
{
    updateButtonStateRemoveSelected();
    updateButtonStateRemoveAll();
}

void ClangToolsProjectSettingsWidget::updateButtonStateRemoveSelected()
{
    const auto selectedRows = m_diagnosticsView.selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() <= 1, return);
    m_removeSelectedButton.setEnabled(!selectedRows.isEmpty());
}

void ClangToolsProjectSettingsWidget::updateButtonStateRemoveAll()
{
    m_removeAllButton.setEnabled(m_diagnosticsView.model()->rowCount() > 0);
}

void ClangToolsProjectSettingsWidget::removeSelected()
{
    const auto selectedRows = m_diagnosticsView.selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() == 1, return);
    const auto * const model
            = static_cast<SuppressedDiagnosticsModel *>(m_diagnosticsView.model());
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
            return Tr::tr("File");
        if (section == ColumnDescription)
            return Tr::tr("Diagnostic");
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

class ClangToolsProjectPanelFactory final : public ProjectPanelFactory
{
public:
    ClangToolsProjectPanelFactory()
    {
        setPriority(100);
        setId(Constants::PROJECT_PANEL_ID);
        setDisplayName(Tr::tr("Clang Tools"));
        setCreateWidgetFunction([](Project *project) {
            return new ClangToolsProjectSettingsWidget(project);
        });
    }
};

void setupClangToolsProjectPanel()
{
    static ClangToolsProjectPanelFactory theClangToolsProjectPanelFactory;
}

} // namespace ClangTools::Internal
