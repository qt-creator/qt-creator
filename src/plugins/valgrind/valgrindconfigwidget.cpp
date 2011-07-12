/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "valgrindconfigwidget.h"
#include "valgrindsettings.h"

#include "ui_valgrindconfigwidget.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>

#include <QtGui/QStandardItemModel>
#include <QtGui/QFileDialog>

namespace Valgrind {
namespace Internal {

ValgrindConfigWidget::ValgrindConfigWidget(ValgrindBaseSettings *settings,
        QWidget *parent, bool global)
    : QWidget(parent),
      m_settings(settings),
      m_ui(new Ui::ValgrindConfigWidget)
{
    m_ui->setupUi(this);

    m_ui->valgrindExeChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_ui->valgrindExeChooser->setPromptDialogTitle(tr("Valgrind Command"));

    m_ui->valgrindExeChooser->setPath(m_settings->valgrindExecutable());
    connect(m_ui->valgrindExeChooser, SIGNAL(changed(QString)),
            m_settings, SLOT(setValgrindExecutable(QString)));
    connect(m_settings, SIGNAL(valgrindExecutableChanged(QString)),
            m_ui->valgrindExeChooser, SLOT(setPath(QString)));

    //
    // Callgrind
    //
    m_ui->enableCacheSim->setChecked(m_settings->enableCacheSim());
    connect(m_ui->enableCacheSim, SIGNAL(toggled(bool)),
            m_settings, SLOT(setEnableCacheSim(bool)));
    connect(m_settings, SIGNAL(enableCacheSimChanged(bool)),
            m_ui->enableCacheSim, SLOT(setChecked(bool)));

    m_ui->enableBranchSim->setChecked(m_settings->enableBranchSim());
    connect(m_ui->enableBranchSim, SIGNAL(toggled(bool)),
            m_settings, SLOT(setEnableBranchSim(bool)));
    connect(m_settings, SIGNAL(enableBranchSimChanged(bool)),
            m_ui->enableBranchSim, SLOT(setChecked(bool)));

    m_ui->collectSystime->setChecked(m_settings->collectSystime());
    connect(m_ui->collectSystime, SIGNAL(toggled(bool)),
            m_settings, SLOT(setCollectSystime(bool)));
    connect(m_settings, SIGNAL(collectSystimeChanged(bool)),
            m_ui->collectSystime, SLOT(setChecked(bool)));

    m_ui->collectBusEvents->setChecked(m_settings->collectBusEvents());
    connect(m_ui->collectBusEvents, SIGNAL(toggled(bool)),
            m_settings, SLOT(setCollectBusEvents(bool)));
    connect(m_settings, SIGNAL(collectBusEventsChanged(bool)),
            m_ui->collectBusEvents, SLOT(setChecked(bool)));

    m_ui->enableEventToolTips->setChecked(m_settings->enableEventToolTips());
    connect(m_ui->enableEventToolTips, SIGNAL(toggled(bool)),
            m_settings, SLOT(setEnableEventToolTips(bool)));
    connect(m_settings, SIGNAL(enableEventToolTipsChanged(bool)),
            m_ui->enableEventToolTips, SLOT(setChecked(bool)));

    m_ui->minimumInclusiveCostRatio->setValue(m_settings->minimumInclusiveCostRatio());
    connect(m_ui->minimumInclusiveCostRatio, SIGNAL(valueChanged(double)),
            m_settings, SLOT(setMinimumInclusiveCostRatio(double)));
    connect(m_settings, SIGNAL(minimumInclusiveCostRatioChanged(double)),
            m_ui->minimumInclusiveCostRatio, SLOT(setValue(double)));

    m_ui->visualisationMinimumInclusiveCostRatio->setValue(m_settings->visualisationMinimumInclusiveCostRatio());
    connect(m_ui->visualisationMinimumInclusiveCostRatio, SIGNAL(valueChanged(double)),
            m_settings, SLOT(setVisualisationMinimumInclusiveCostRatio(double)));
    connect(m_settings, SIGNAL(visualisationMinimumInclusiveCostRatioChanged(double)),
            m_ui->visualisationMinimumInclusiveCostRatio, SLOT(setValue(double)));

    //
    // Memcheck
    //
    m_model = new QStandardItemModel(this);

    m_ui->suppressionList->setModel(m_model);
    m_ui->suppressionList->setSelectionMode(QAbstractItemView::MultiSelection);

    connect(m_ui->addSuppression, SIGNAL(clicked()),
            this, SLOT(slotAddSuppression()));
    connect(m_ui->removeSuppression, SIGNAL(clicked()),
            this, SLOT(slotRemoveSuppression()));

    m_ui->numCallers->setValue(m_settings->numCallers());
    connect(m_ui->numCallers, SIGNAL(valueChanged(int)), m_settings, SLOT(setNumCallers(int)));
    connect(m_settings, SIGNAL(numCallersChanged(int)), m_ui->numCallers, SLOT(setValue(int)));

    m_ui->trackOrigins->setChecked(m_settings->trackOrigins());
    connect(m_ui->trackOrigins, SIGNAL(toggled(bool)),
            m_settings, SLOT(setTrackOrigins(bool)));
    connect(m_settings, SIGNAL(trackOriginsChanged(bool)),
            m_ui->trackOrigins, SLOT(setChecked(bool)));

    connect(m_settings, SIGNAL(suppressionFilesRemoved(QStringList)),
            this, SLOT(slotSuppressionsRemoved(QStringList)));
    connect(m_settings, SIGNAL(suppressionFilesAdded(QStringList)),
            this, SLOT(slotSuppressionsAdded(QStringList)));

    m_model->clear();
    foreach (const QString &file, m_settings->suppressionFiles())
        m_model->appendRow(new QStandardItem(file));

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

void ValgrindConfigWidget::slotAddSuppression()
{
    QFileDialog dialog;
    dialog.setNameFilter(tr("Valgrind Suppression File (*.supp);;All Files (*)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    ValgrindGlobalSettings *conf = Analyzer::AnalyzerGlobalSettings::instance()->subConfig<ValgrindGlobalSettings>();
    QTC_ASSERT(conf, return);
    dialog.setDirectory(conf->lastSuppressionDialogDirectory());
    dialog.setHistory(conf->lastSuppressionDialogHistory());

    if (dialog.exec() == QDialog::Accepted) {
        foreach (const QString &file, dialog.selectedFiles())
            m_model->appendRow(new QStandardItem(file));

        m_settings->addSuppressionFiles(dialog.selectedFiles());
    }

    conf->setLastSuppressionDialogDirectory(dialog.directory().absolutePath());
    conf->setLastSuppressionDialogHistory(dialog.history());
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
