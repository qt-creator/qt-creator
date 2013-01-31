/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "valgrindconfigwidget.h"
#include "valgrindsettings.h"

#include "ui_valgrindconfigwidget.h"

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QDebug>

#include <QStandardItemModel>
#include <QFileDialog>

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
    m_ui->valgrindExeChooser->setPromptDialogTitle(tr("Valgrind Command"));

    updateUi();
    connect(m_settings, SIGNAL(changed()), this, SLOT(updateUi()));

    connect(m_ui->valgrindExeChooser, SIGNAL(changed(QString)),
            m_settings, SLOT(setValgrindExecutable(QString)));
    connect(m_settings, SIGNAL(valgrindExecutableChanged(QString)),
            m_ui->valgrindExeChooser, SLOT(setPath(QString)));

    if (Utils::HostOsInfo::isWindowsHost()) {
        // FIXME: On Window we know that we don't have a local valgrind
        // executable, so having the "Browse" button in the path chooser
        // (which is needed for the remote executable) is confusing.
        m_ui->valgrindExeChooser->buttonAtIndex(0)->hide();
    }

    //
    // Callgrind
    //
    connect(m_ui->enableCacheSim, SIGNAL(toggled(bool)),
            m_settings, SLOT(setEnableCacheSim(bool)));
    connect(m_settings, SIGNAL(enableCacheSimChanged(bool)),
            m_ui->enableCacheSim, SLOT(setChecked(bool)));

    connect(m_ui->enableBranchSim, SIGNAL(toggled(bool)),
            m_settings, SLOT(setEnableBranchSim(bool)));
    connect(m_settings, SIGNAL(enableBranchSimChanged(bool)),
            m_ui->enableBranchSim, SLOT(setChecked(bool)));

    connect(m_ui->collectSystime, SIGNAL(toggled(bool)),
            m_settings, SLOT(setCollectSystime(bool)));
    connect(m_settings, SIGNAL(collectSystimeChanged(bool)),
            m_ui->collectSystime, SLOT(setChecked(bool)));

    connect(m_ui->collectBusEvents, SIGNAL(toggled(bool)),
            m_settings, SLOT(setCollectBusEvents(bool)));
    connect(m_settings, SIGNAL(collectBusEventsChanged(bool)),
            m_ui->collectBusEvents, SLOT(setChecked(bool)));

    connect(m_ui->enableEventToolTips, SIGNAL(toggled(bool)),
            m_settings, SLOT(setEnableEventToolTips(bool)));
    connect(m_settings, SIGNAL(enableEventToolTipsChanged(bool)),
            m_ui->enableEventToolTips, SLOT(setChecked(bool)));

    connect(m_ui->minimumInclusiveCostRatio, SIGNAL(valueChanged(double)),
            m_settings, SLOT(setMinimumInclusiveCostRatio(double)));
    connect(m_settings, SIGNAL(minimumInclusiveCostRatioChanged(double)),
            m_ui->minimumInclusiveCostRatio, SLOT(setValue(double)));

    connect(m_ui->visualisationMinimumInclusiveCostRatio, SIGNAL(valueChanged(double)),
            m_settings, SLOT(setVisualisationMinimumInclusiveCostRatio(double)));
    connect(m_settings, SIGNAL(visualisationMinimumInclusiveCostRatioChanged(double)),
            m_ui->visualisationMinimumInclusiveCostRatio, SLOT(setValue(double)));

    //
    // Memcheck
    //
    m_ui->suppressionList->setModel(m_model);
    m_ui->suppressionList->setSelectionMode(QAbstractItemView::MultiSelection);

    connect(m_ui->addSuppression, SIGNAL(clicked()),
            this, SLOT(slotAddSuppression()));
    connect(m_ui->removeSuppression, SIGNAL(clicked()),
            this, SLOT(slotRemoveSuppression()));

    connect(m_ui->numCallers, SIGNAL(valueChanged(int)), m_settings, SLOT(setNumCallers(int)));
    connect(m_settings, SIGNAL(numCallersChanged(int)), m_ui->numCallers, SLOT(setValue(int)));

    connect(m_ui->trackOrigins, SIGNAL(toggled(bool)),
            m_settings, SLOT(setTrackOrigins(bool)));
    connect(m_settings, SIGNAL(trackOriginsChanged(bool)),
            m_ui->trackOrigins, SLOT(setChecked(bool)));

    connect(m_settings, SIGNAL(suppressionFilesRemoved(QStringList)),
            this, SLOT(slotSuppressionsRemoved(QStringList)));
    connect(m_settings, SIGNAL(suppressionFilesAdded(QStringList)),
            this, SLOT(slotSuppressionsAdded(QStringList)));

    connect(m_ui->suppressionList->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotSuppressionSelectionChanged()));
    slotSuppressionSelectionChanged();

    if (!global) {
        // In project settings we want a flat vertical list.
        QVBoxLayout *l = new QVBoxLayout;
        while (layout()->count())
            if (QWidget *w = layout()->takeAt(0)->widget())
                l->addWidget(w);
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
    m_ui->enableCacheSim->setChecked(m_settings->enableCacheSim());
    m_ui->enableBranchSim->setChecked(m_settings->enableBranchSim());
    m_ui->collectSystime->setChecked(m_settings->collectSystime());
    m_ui->collectBusEvents->setChecked(m_settings->collectBusEvents());
    m_ui->enableEventToolTips->setChecked(m_settings->enableEventToolTips());
    m_ui->minimumInclusiveCostRatio->setValue(m_settings->minimumInclusiveCostRatio());
    m_ui->visualisationMinimumInclusiveCostRatio->setValue(m_settings->visualisationMinimumInclusiveCostRatio());
    m_ui->numCallers->setValue(m_settings->numCallers());
    m_ui->trackOrigins->setChecked(m_settings->trackOrigins());
    m_model->clear();
    foreach (const QString &file, m_settings->suppressionFiles())
        m_model->appendRow(new QStandardItem(file));
}

void ValgrindConfigWidget::slotAddSuppression()
{
    ValgrindGlobalSettings *conf = Analyzer::AnalyzerGlobalSettings::instance()->subConfig<ValgrindGlobalSettings>();
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

bool sortReverse(int l, int r)
{
    return l > r;
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

    qSort(rows.begin(), rows.end(), sortReverse);

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
