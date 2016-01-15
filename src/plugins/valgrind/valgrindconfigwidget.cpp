/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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

#include "valgrindconfigwidget.h"
#include "valgrindsettings.h"
#include "valgrindplugin.h"

#include "ui_valgrindconfigwidget.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>

#include <QStandardItemModel>
#include <QFileDialog>

#include <functional>

namespace Valgrind {
namespace Internal {

ValgrindConfigWidget::ValgrindConfigWidget(ValgrindBaseSettings *settings,
        QWidget *parent, bool global)
    : QWidget(parent),
      m_settings(settings),
      m_ui(new Ui::ValgrindConfigWidget)
{
    m_ui->setupUi(this);
    m_model = new QStandardItemModel(this);

    m_ui->valgrindExeChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->valgrindExeChooser->setHistoryCompleter(QLatin1String("Valgrind.Command.History"));
    m_ui->valgrindExeChooser->setPromptDialogTitle(tr("Valgrind Command"));

    updateUi();
    connect(m_settings, &ValgrindBaseSettings::changed, this, &ValgrindConfigWidget::updateUi);

    connect(m_ui->valgrindExeChooser, &Utils::PathChooser::rawPathChanged,
            m_settings, &ValgrindBaseSettings::setValgrindExecutable);
    connect(m_settings, &ValgrindBaseSettings::valgrindExecutableChanged,
            m_ui->valgrindExeChooser, &Utils::PathChooser::setPath);
    connect(m_ui->smcDetectionComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            m_settings, &ValgrindBaseSettings::setSelfModifyingCodeDetection);

    if (Utils::HostOsInfo::isWindowsHost()) {
        // FIXME: On Window we know that we don't have a local valgrind
        // executable, so having the "Browse" button in the path chooser
        // (which is needed for the remote executable) is confusing.
        m_ui->valgrindExeChooser->buttonAtIndex(0)->hide();
    }

    //
    // Callgrind
    //
    connect(m_ui->enableCacheSim, &QCheckBox::toggled,
            m_settings, &ValgrindBaseSettings::setEnableCacheSim);
    connect(m_settings, &ValgrindBaseSettings::enableCacheSimChanged,
            m_ui->enableCacheSim, &QAbstractButton::setChecked);

    connect(m_ui->enableBranchSim, &QCheckBox::toggled,
            m_settings, &ValgrindBaseSettings::setEnableBranchSim);
    connect(m_settings, &ValgrindBaseSettings::enableBranchSimChanged,
            m_ui->enableBranchSim, &QAbstractButton::setChecked);

    connect(m_ui->collectSystime, &QCheckBox::toggled,
            m_settings, &ValgrindBaseSettings::setCollectSystime);
    connect(m_settings, &ValgrindBaseSettings::collectSystimeChanged,
            m_ui->collectSystime, &QAbstractButton::setChecked);

    connect(m_ui->collectBusEvents, &QCheckBox::toggled,
            m_settings, &ValgrindBaseSettings::setCollectBusEvents);
    connect(m_settings, &ValgrindBaseSettings::collectBusEventsChanged,
            m_ui->collectBusEvents, &QAbstractButton::setChecked);

    connect(m_ui->enableEventToolTips, &QGroupBox::toggled,
            m_settings, &ValgrindBaseSettings::setEnableEventToolTips);
    connect(m_settings, &ValgrindBaseSettings::enableEventToolTipsChanged,
            m_ui->enableEventToolTips, &QGroupBox::setChecked);

    connect(m_ui->minimumInclusiveCostRatio, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            m_settings, &ValgrindBaseSettings::setMinimumInclusiveCostRatio);
    connect(m_settings, &ValgrindBaseSettings::minimumInclusiveCostRatioChanged,
            m_ui->minimumInclusiveCostRatio, &QDoubleSpinBox::setValue);

    connect(m_ui->visualisationMinimumInclusiveCostRatio, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            m_settings, &ValgrindBaseSettings::setVisualisationMinimumInclusiveCostRatio);
    connect(m_settings, &ValgrindBaseSettings::visualisationMinimumInclusiveCostRatioChanged,
            m_ui->visualisationMinimumInclusiveCostRatio, &QDoubleSpinBox::setValue);

    //
    // Memcheck
    //
    m_ui->suppressionList->setModel(m_model);
    m_ui->suppressionList->setSelectionMode(QAbstractItemView::MultiSelection);

    connect(m_ui->addSuppression, &QPushButton::clicked, this, &ValgrindConfigWidget::slotAddSuppression);
    connect(m_ui->removeSuppression, &QPushButton::clicked, this, &ValgrindConfigWidget::slotRemoveSuppression);

    connect(m_ui->numCallers, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            m_settings, &ValgrindBaseSettings::setNumCallers);
    connect(m_settings, &ValgrindBaseSettings::numCallersChanged,
            m_ui->numCallers, &QSpinBox::setValue);

    connect(m_ui->leakCheckOnFinish, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            m_settings, &ValgrindBaseSettings::setLeakCheckOnFinish);
    connect(m_settings, &ValgrindBaseSettings::leakCheckOnFinishChanged,
            m_ui->leakCheckOnFinish, &QComboBox::setCurrentIndex);

    connect(m_ui->showReachable, &QCheckBox::toggled,
            m_settings, &ValgrindBaseSettings::setShowReachable);
    connect(m_settings, &ValgrindBaseSettings::showReachableChanged,
            m_ui->showReachable, &QAbstractButton::setChecked);

    connect(m_ui->trackOrigins, &QCheckBox::toggled, m_settings, &ValgrindBaseSettings::setTrackOrigins);
    connect(m_settings, &ValgrindBaseSettings::trackOriginsChanged,
            m_ui->trackOrigins, &QAbstractButton::setChecked);

    connect(m_settings, &ValgrindBaseSettings::suppressionFilesRemoved,
            this, &ValgrindConfigWidget::slotSuppressionsRemoved);
    connect(m_settings, &ValgrindBaseSettings::suppressionFilesAdded,
            this, &ValgrindConfigWidget::slotSuppressionsAdded);

    connect(m_ui->suppressionList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ValgrindConfigWidget::slotSuppressionSelectionChanged);
    slotSuppressionSelectionChanged();

    if (!global) {
        // In project settings we want a flat vertical list.
        QVBoxLayout *l = new QVBoxLayout;
        while (layout()->count()) {
            QLayoutItem *item = layout()->takeAt(0);
            if (QWidget *w = item->widget())
                l->addWidget(w);
            delete item;
        }
        delete layout();
        setLayout(l);
    }
}

ValgrindConfigWidget::~ValgrindConfigWidget()
{
    delete m_ui;
}

void ValgrindConfigWidget::updateUi()
{
    m_ui->valgrindExeChooser->setPath(m_settings->valgrindExecutable());
    m_ui->smcDetectionComboBox->setCurrentIndex(m_settings->selfModifyingCodeDetection());
    m_ui->enableCacheSim->setChecked(m_settings->enableCacheSim());
    m_ui->enableBranchSim->setChecked(m_settings->enableBranchSim());
    m_ui->collectSystime->setChecked(m_settings->collectSystime());
    m_ui->collectBusEvents->setChecked(m_settings->collectBusEvents());
    m_ui->enableEventToolTips->setChecked(m_settings->enableEventToolTips());
    m_ui->minimumInclusiveCostRatio->setValue(m_settings->minimumInclusiveCostRatio());
    m_ui->visualisationMinimumInclusiveCostRatio->setValue(m_settings->visualisationMinimumInclusiveCostRatio());
    m_ui->numCallers->setValue(m_settings->numCallers());
    m_ui->leakCheckOnFinish->setCurrentIndex(m_settings->leakCheckOnFinish());
    m_ui->showReachable->setChecked(m_settings->showReachable());
    m_ui->trackOrigins->setChecked(m_settings->trackOrigins());
    m_model->clear();
    foreach (const QString &file, m_settings->suppressionFiles())
        m_model->appendRow(new QStandardItem(file));
}

void ValgrindConfigWidget::slotAddSuppression()
{
    ValgrindGlobalSettings *conf = ValgrindPlugin::globalSettings();
    QTC_ASSERT(conf, return);
    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("Valgrind Suppression Files"),
        conf->lastSuppressionDialogDirectory(),
        tr("Valgrind Suppression File (*.supp);;All Files (*)"));
    //dialog.setHistory(conf->lastSuppressionDialogHistory());
    if (!files.isEmpty()) {
        foreach (const QString &file, files)
            m_model->appendRow(new QStandardItem(file));
        m_settings->addSuppressionFiles(files);
        conf->setLastSuppressionDialogDirectory(QFileInfo(files.at(0)).absolutePath());
        //conf->setLastSuppressionDialogHistory(dialog.history());
    }
}

void ValgrindConfigWidget::slotSuppressionsAdded(const QStringList &files)
{
    QStringList filesToAdd = files;
    for (int i = 0, c = m_model->rowCount(); i < c; ++i)
        filesToAdd.removeAll(m_model->item(i)->text());

    foreach (const QString &file, filesToAdd)
        m_model->appendRow(new QStandardItem(file));
}

void ValgrindConfigWidget::slotRemoveSuppression()
{
    // remove from end so no rows get invalidated
    QList<int> rows;

    QStringList removed;
    foreach (const QModelIndex &index, m_ui->suppressionList->selectionModel()->selectedIndexes()) {
        rows << index.row();
        removed << index.data().toString();
    }

    Utils::sort(rows, std::greater<int>());

    foreach (int row, rows)
        m_model->removeRow(row);

    m_settings->removeSuppressionFiles(removed);
}

void ValgrindConfigWidget::slotSuppressionsRemoved(const QStringList &files)
{
    for (int i = 0; i < m_model->rowCount(); ++i) {
        if (files.contains(m_model->item(i)->text())) {
            m_model->removeRow(i);
            --i;
        }
    }
}

void ValgrindConfigWidget::setSuppressions(const QStringList &files)
{
    m_model->clear();
    foreach (const QString &file, files)
        m_model->appendRow(new QStandardItem(file));
}

QStringList ValgrindConfigWidget::suppressions() const
{
    QStringList ret;

    for (int i = 0; i < m_model->rowCount(); ++i)
        ret << m_model->item(i)->text();

    return ret;
}

void ValgrindConfigWidget::slotSuppressionSelectionChanged()
{
    m_ui->removeSuppression->setEnabled(m_ui->suppressionList->selectionModel()->hasSelection());
}

} // namespace Internal
} // namespace Valgrind
