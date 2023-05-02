// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsprojectsettingswidget.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolstr.h"
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

ClangToolsProjectSettingsWidget::ClangToolsProjectSettingsWidget(ProjectExplorer::Project *project, QWidget *parent) :
    ProjectExplorer::ProjectSettingsWidget(parent),
    m_projectSettings(ClangToolsProjectSettings::getSettings(project))
{
    setGlobalSettingsId(ClangTools::Constants::SETTINGS_PAGE_ID);
    m_restoreGlobal = new QPushButton(Tr::tr("Restore Global Settings"));

    const auto gotoClangTidyModeLabel
            = new QLabel("<a href=\"target\">" + Tr::tr("Go to Clang-Tidy") + "</a>");
    const auto gotoClazyModeLabel
            = new QLabel("<a href=\"target\">" + Tr::tr("Go to Clazy") + "</a>");

    m_runSettingsWidget = new ClangTools::Internal::RunSettingsWidget(this);

    m_diagnosticsView = new QTreeView;
    m_diagnosticsView->setSelectionMode(QAbstractItemView::SingleSelection);

    m_removeSelectedButton = new QPushButton(Tr::tr("Remove Selected"), this);
    m_removeAllButton = new QPushButton(Tr::tr("Remove All"));

    using namespace Layouting;
    Column {
        Row { m_restoreGlobal, st, gotoClangTidyModeLabel, gotoClazyModeLabel },

        m_runSettingsWidget,

        Group {
            title(Tr::tr("Suppressed diagnostics")),
            Row {
                m_diagnosticsView,
                Column {
                    m_removeSelectedButton,
                    m_removeAllButton,
                    st
                }
            }
        },
        noMargin
    }.attachTo(this);

    setUseGlobalSettings(m_projectSettings->useGlobalSettings());
    onGlobalCustomChanged(useGlobalSettings());
    connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged,
            this, &ClangToolsProjectSettingsWidget::onGlobalCustomChanged);

    // Global settings
    connect(ClangToolsSettings::instance(), &ClangToolsSettings::changed,
            this, [this] { onGlobalCustomChanged(useGlobalSettings()); });
    connect(m_restoreGlobal, &QPushButton::clicked, this, [this] {
        m_runSettingsWidget->fromSettings(ClangToolsSettings::instance()->runSettings());
    });

    connect(gotoClangTidyModeLabel, &QLabel::linkActivated, [](const QString &) {
        ClangTidyTool::instance()->selectPerspective();
    });
    connect(gotoClazyModeLabel, &QLabel::linkActivated, [](const QString &) {
        ClazyTool::instance()->selectPerspective();
    });

    // Run options
    connect(m_runSettingsWidget, &RunSettingsWidget::changed, this, [this] {
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
    connect(m_projectSettings.data(), &ClangToolsProjectSettings::suppressedDiagnosticsChanged, this,
            [model, this] {
                    model->setDiagnostics(m_projectSettings->suppressedDiagnostics());
                    updateButtonStates();
            });
    m_diagnosticsView->setModel(model);
    updateButtonStates();
    connect(m_diagnosticsView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this](const QItemSelection &, const QItemSelection &) {
                updateButtonStateRemoveSelected();
            });
    connect(m_removeSelectedButton, &QAbstractButton::clicked,
            this, [this](bool) { removeSelected(); });
    connect(m_removeAllButton, &QAbstractButton::clicked,
            this, [this](bool) { m_projectSettings->removeAllSuppressedDiagnostics();});
}

void ClangToolsProjectSettingsWidget::onGlobalCustomChanged(bool useGlobal)
{
    const RunSettings runSettings = useGlobal ? ClangToolsSettings::instance()->runSettings()
                                              : m_projectSettings->runSettings();
    m_runSettingsWidget->fromSettings(runSettings);
    m_runSettingsWidget->setEnabled(!useGlobal);
    m_restoreGlobal->setEnabled(!useGlobal);

    m_projectSettings->setUseGlobalSettings(useGlobal);
}

void ClangToolsProjectSettingsWidget::updateButtonStates()
{
    updateButtonStateRemoveSelected();
    updateButtonStateRemoveAll();
}

void ClangToolsProjectSettingsWidget::updateButtonStateRemoveSelected()
{
    const auto selectedRows = m_diagnosticsView->selectionModel()->selectedRows();
    QTC_ASSERT(selectedRows.count() <= 1, return);
    m_removeSelectedButton->setEnabled(!selectedRows.isEmpty());
}

void ClangToolsProjectSettingsWidget::updateButtonStateRemoveAll()
{
    m_removeAllButton->setEnabled(m_diagnosticsView->model()->rowCount() > 0);
}

void ClangToolsProjectSettingsWidget::removeSelected()
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

} // namespace Internal
} // namespace ClangTools

#include "clangtoolsprojectsettingswidget.moc"
