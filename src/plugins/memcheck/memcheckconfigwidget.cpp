/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "memcheckconfigwidget.h"

#include "ui_memcheckconfigwidget.h"

#include "memchecksettings.h"

#include <utils/qtcassert.h>

#include <QtGui/QStandardItemModel>
#include <QtGui/QFileDialog>
#include <QtCore/QDebug>

using namespace Analyzer::Internal;

MemcheckConfigWidget::MemcheckConfigWidget(AbstractMemcheckSettings *settings, QWidget *parent)
    : QWidget(parent),
      m_settings(settings),
      m_model(new QStandardItemModel(this)),
      m_ui(new Ui::MemcheckConfigWidget)
{
    m_ui->setupUi(this);

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
    connect(m_ui->trackOrigins, SIGNAL(toggled(bool)), m_settings, SLOT(setTrackOrigins(bool)));
    connect(m_settings, SIGNAL(trackOriginsChanged(bool)), m_ui->trackOrigins, SLOT(setChecked(bool)));

    connect(m_settings, SIGNAL(suppressionFilesRemoved(QStringList)),
            this, SLOT(slotSuppressionsRemoved(QStringList)));
    connect(m_settings, SIGNAL(suppressionFilesAdded(QStringList)),
            this, SLOT(slotSuppressionsAdded(QStringList)));

    m_model->clear();
    foreach(const QString &file, m_settings->suppressionFiles())
        m_model->appendRow(new QStandardItem(file));

    connect(m_ui->suppressionList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(slotSuppressionSelectionChanged()));
    slotSuppressionSelectionChanged();
}

MemcheckConfigWidget::~MemcheckConfigWidget()
{
    delete m_ui;
}

void MemcheckConfigWidget::slotAddSuppression()
{
    QFileDialog dialog;
    dialog.setNameFilter(tr("Valgrind Suppression File (*.supp);;All Files (*)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    MemcheckGlobalSettings *conf = Analyzer::AnalyzerGlobalSettings::instance()->subConfig<MemcheckGlobalSettings>();
    QTC_ASSERT(conf, return);
    dialog.setDirectory(conf->lastSuppressionDialogDirectory());
    dialog.setHistory(conf->lastSuppressionDialogHistory());

    if (dialog.exec() == QDialog::Accepted) {
        foreach(const QString &file, dialog.selectedFiles())
            m_model->appendRow(new QStandardItem(file));

        m_settings->addSuppressionFiles(dialog.selectedFiles());
    }

    conf->setLastSuppressionDialogDirectory(dialog.directory().absolutePath());
    conf->setLastSuppressionDialogHistory(dialog.history());
}

void MemcheckConfigWidget::slotSuppressionsAdded(const QStringList &files)
{
    QStringList filesToAdd = files;
    for(int i = 0, c = m_model->rowCount(); i < c; ++i)
        filesToAdd.removeAll(m_model->item(i)->text());

    foreach(const QString &file, filesToAdd)
        m_model->appendRow(new QStandardItem(file));
}

bool sortReverse(int l, int r)
{
    return l > r;
}

void MemcheckConfigWidget::slotRemoveSuppression()
{
    // remove from end so no rows get invalidated
    QList<int> rows;

    QStringList removed;
    foreach(const QModelIndex &index, m_ui->suppressionList->selectionModel()->selectedIndexes()) {
        rows << index.row();
        removed << index.data().toString();
    }

    qSort(rows.begin(), rows.end(), sortReverse);

    foreach(int row, rows)
        m_model->removeRow(row);

    m_settings->removeSuppressionFiles(removed);
}

void MemcheckConfigWidget::slotSuppressionsRemoved(const QStringList &files)
{
    for(int i = 0; i < m_model->rowCount(); ++i) {
        if (files.contains(m_model->item(i)->text())) {
            m_model->removeRow(i);
            --i;
        }
    }
}

void MemcheckConfigWidget::setSuppressions(const QStringList &files)
{
    m_model->clear();
    foreach(const QString &file, files)
        m_model->appendRow(new QStandardItem(file));
}

QStringList MemcheckConfigWidget::suppressions() const
{
    QStringList ret;

    for(int i = 0; i < m_model->rowCount(); ++i)
        ret << m_model->item(i)->text();

    return ret;
}

void MemcheckConfigWidget::slotSuppressionSelectionChanged()
{
    m_ui->removeSuppression->setEnabled(m_ui->suppressionList->selectionModel()->hasSelection());
}
