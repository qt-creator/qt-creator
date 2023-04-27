// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonbuildsettingswidget.h"

#include "mesonbuildconfiguration.h"
#include "mesonbuildsystem.h"
#include "mesonprojectmanagertr.h"

#include <coreplugin/find/itemviewfind.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/projectconfiguration.h>

#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>

#include <QLayout>
#include <QPushButton>

using namespace Utils;

namespace MesonProjectManager::Internal {

MesonBuildSettingsWidget::MesonBuildSettingsWidget(MesonBuildConfiguration *buildCfg)
    : ProjectExplorer::NamedWidget(Tr::tr("Meson"))
    , m_progressIndicator(ProgressIndicatorSize::Large)
{
    auto configureButton = new QPushButton(Tr::tr("Apply Configuration Changes"));
    configureButton->setEnabled(false);
    configureButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    auto wipeButton = new QPushButton(Tr::tr("Wipe Project"));
    wipeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    wipeButton->setIcon(Utils::Icons::WARNING.icon());
    wipeButton->setToolTip(Tr::tr("Wipes build directory and reconfigures using previous command "
                                  "line options.\nUseful if build directory is corrupted or when "
                                  "rebuilding with a newer version of Meson."));

    auto container = new DetailsWidget;

    auto details = new QWidget;

    container->setState(DetailsWidget::NoSummary);
    container->setWidget(details);

    auto parametersLineEdit = new QLineEdit;

    auto buildDirWidget = new QWidget;

    auto optionsFilterLineEdit = new FancyLineEdit;

    auto optionsTreeView = new TreeView;
    optionsTreeView->setMinimumHeight(300);
    optionsTreeView->setFrameShape(QFrame::NoFrame);
    optionsTreeView->setSelectionBehavior(QAbstractItemView::SelectItems);
    optionsTreeView->setUniformRowHeights(true);
    optionsTreeView->setSortingEnabled(true);

    using namespace Layouting;

    Column {
        Form { Tr::tr("Parameters"), parametersLineEdit, br, },
        buildDirWidget,
        optionsFilterLineEdit,
        optionsTreeView,
    }.attachTo(details, WithoutMargins);

    Column {
        container,
        Row { configureButton, wipeButton, }
    }.attachTo(this, WithoutMargins);

    Form {
        buildCfg->buildDirectoryAspect(),
    }.attachTo(buildDirWidget, WithoutMargins);

    parametersLineEdit->setText(buildCfg->parameters());
    optionsFilterLineEdit->setFiltering(true);

    optionsTreeView->sortByColumn(0, Qt::AscendingOrder);

    QFrame *findWrapper
        = Core::ItemViewFind::createSearchableWrapper(optionsTreeView,
                                                      Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);
    m_progressIndicator.attachToWidget(findWrapper);
    m_progressIndicator.raise();
    m_progressIndicator.hide();
    details->layout()->addWidget(findWrapper);

    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { m_progressIndicator.show(); });
    connect(&m_optionsModel, &BuidOptionsModel::configurationChanged, this, [configureButton] {
        configureButton->setEnabled(true);
    });
    m_optionsFilter.setSourceModel(&m_optionsModel);
    m_optionsFilter.setSortRole(Qt::DisplayRole);
    m_optionsFilter.setFilterKeyColumn(-1);

    optionsTreeView->setModel(&m_optionsFilter);
    optionsTreeView->setItemDelegate(new BuildOptionDelegate{optionsTreeView});

    MesonBuildSystem *bs = static_cast<MesonBuildSystem *>(buildCfg->buildSystem());
    connect(buildCfg->target(), &ProjectExplorer::Target::parsingFinished,
            this, [this, bs, optionsTreeView](bool success) {
                if (success) {
                    m_optionsModel.setConfiguration(bs->buildOptions());
                } else {
                    m_optionsModel.clear();
                }
                optionsTreeView->expandAll();
                optionsTreeView->resizeColumnToContents(0);
                optionsTreeView->setEnabled(true);
                m_showProgressTimer.stop();
                m_progressIndicator.hide();
            });

    connect(bs, &MesonBuildSystem::parsingStarted, this, [this, optionsTreeView] {
        if (!m_showProgressTimer.isActive()) {
            optionsTreeView->setEnabled(false);
            m_showProgressTimer.start();
        }
    });

    connect(&m_optionsModel, &BuidOptionsModel::dataChanged, this, [bs, this] {
        bs->setMesonConfigArgs(this->m_optionsModel.changesAsMesonArgs());
    });

    connect(&m_optionsFilter, &QAbstractItemModel::modelReset, this, [optionsTreeView] {
        optionsTreeView->expandAll();
        optionsTreeView->resizeColumnToContents(0);
    });

    connect(optionsFilterLineEdit, &QLineEdit::textChanged, &m_optionsFilter, [this](const QString &txt) {
        m_optionsFilter.setFilterRegularExpression(
            QRegularExpression(QRegularExpression::escape(txt),
                               QRegularExpression::CaseInsensitiveOption));
    });

    connect(optionsTreeView,
            &Utils::TreeView::activated,
            optionsTreeView,
            [tree = optionsTreeView](const QModelIndex &idx) { tree->edit(idx); });

    connect(configureButton, &QPushButton::clicked, [this, bs, configureButton, optionsTreeView] {
        optionsTreeView->setEnabled(false);
        configureButton->setEnabled(false);
        m_showProgressTimer.start();
        bs->configure();
    });
    connect(wipeButton, &QPushButton::clicked, [this, bs, configureButton, optionsTreeView] {
        optionsTreeView->setEnabled(false);
        configureButton->setEnabled(false);
        m_showProgressTimer.start();
        bs->wipe();
    });
    connect(parametersLineEdit, &QLineEdit::editingFinished, this, [ buildCfg, parametersLineEdit] {
        buildCfg->setParameters(parametersLineEdit->text());
    });
    bs->triggerParsing();
}

MesonBuildSettingsWidget::~MesonBuildSettingsWidget() = default;

} // MesonProjectManager::Internal
