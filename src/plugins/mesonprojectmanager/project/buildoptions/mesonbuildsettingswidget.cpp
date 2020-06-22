/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/buildaspects.h>
#include <projectexplorer/projectconfiguration.h>
#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>

#include "../mesonbuildconfiguration.h"
#include "../mesonbuildsystem.h"
#include "mesonbuildsettingswidget.h"
#include "ui_mesonbuildsettingswidget.h"

namespace MesonProjectManager {
namespace Internal {
MesonBuildSettingsWidget::MesonBuildSettingsWidget(MesonBuildConfiguration *buildCfg)
    : ProjectExplorer::NamedWidget{tr("Meson")}
    , ui{new Ui::MesonBuildSettingsWidget}
    , m_progressIndicator(Utils::ProgressIndicatorSize::Large)
{
    ui->setupUi(this);
    ui->container->setState(Utils::DetailsWidget::NoSummary);
    ui->container->setWidget(ui->details);
    ProjectExplorer::LayoutBuilder buildDirWBuilder{ui->buildDirWidget};
    auto buildDirAspect = buildCfg->buildDirectoryAspect();
    buildDirAspect->addToLayout(buildDirWBuilder);

    ui->optionsFilterLineEdit->setFiltering(true);

    ui->optionsTreeView->sortByColumn(0, Qt::AscendingOrder);

    QFrame *findWrapper
        = Core::ItemViewFind::createSearchableWrapper(ui->optionsTreeView,
                                                      Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);
    m_progressIndicator.attachToWidget(findWrapper);
    m_progressIndicator.raise();
    m_progressIndicator.hide();
    ui->details->layout()->addWidget(findWrapper);

    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { m_progressIndicator.show(); });
    connect(&m_optionsModel, &BuidOptionsModel::configurationChanged, this, [this]() {
        ui->configureButton->setEnabled(true);
    });
    m_optionsFilter.setSourceModel(&m_optionsModel);
    m_optionsFilter.setSortRole(Qt::DisplayRole);
    m_optionsFilter.setFilterKeyColumn(-1);

    ui->optionsTreeView->setModel(&m_optionsFilter);

    ui->optionsTreeView->setItemDelegate(new BuildOptionDelegate{ui->optionsTreeView});
    MesonBuildSystem *bs = static_cast<MesonBuildSystem *>(buildCfg->buildSystem());
    connect(buildCfg->target(),
            &ProjectExplorer::Target::parsingFinished,
            this,
            [this, bs](bool success) {
                if (success) {
                    m_optionsModel.setConfiguration(bs->buildOptions());
                } else {
                    m_optionsModel.clear();
                }
                ui->optionsTreeView->expandAll();
                ui->optionsTreeView->resizeColumnToContents(0);
                ui->optionsTreeView->setEnabled(true);
                m_showProgressTimer.stop();
                m_progressIndicator.hide();
            });

    connect(bs, &MesonBuildSystem::parsingStarted, this, [this]() {
        if (!m_showProgressTimer.isActive()) {
            ui->optionsTreeView->setEnabled(false);
            m_showProgressTimer.start();
        }
    });

    connect(&m_optionsModel, &BuidOptionsModel::dataChanged, this, [bs, this]() {
        bs->setMesonConfigArgs(this->m_optionsModel.changesAsMesonArgs());
    });

    connect(&m_optionsFilter, &QAbstractItemModel::modelReset, this, [this]() {
        ui->optionsTreeView->expandAll();
        ui->optionsTreeView->resizeColumnToContents(0);
    });
    connect(ui->optionsFilterLineEdit,
            &QLineEdit::textChanged,
            &m_optionsFilter,
            [this](const QString &txt) {
                m_optionsFilter.setFilterRegularExpression(
                    QRegularExpression(QRegularExpression::escape(txt),
                                       QRegularExpression::CaseInsensitiveOption));
            });
    connect(ui->optionsTreeView,
            &Utils::TreeView::activated,
            ui->optionsTreeView,
            [tree = ui->optionsTreeView](const QModelIndex &idx) { tree->edit(idx); });
    connect(ui->configureButton, &QPushButton::clicked, [bs, this]() {
        ui->optionsTreeView->setEnabled(false);
        ui->configureButton->setEnabled(false);
        m_showProgressTimer.start();
        bs->configure();
    });
    connect(ui->wipeButton, &QPushButton::clicked, [bs, this]() {
        ui->optionsTreeView->setEnabled(false);
        ui->configureButton->setEnabled(false);
        m_showProgressTimer.start();
        bs->wipe();
    });
    bs->triggerParsing();
}

MesonBuildSettingsWidget::~MesonBuildSettingsWidget()
{
    delete ui;
}

} // namespace Internal
} // namespace MesonProjectManager
