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

#include <debugger/analyzer/analyzericons.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QFileDialog>
#include <QListView>
#include <QPushButton>
#include <QStandardItemModel>

#include <functional>

using namespace Utils;

namespace Valgrind {
namespace Internal {

class ValgrindBaseSettings;

class ValgrindConfigWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Valgrind::Internal::ValgrindConfigWidget)

public:
    explicit ValgrindConfigWidget(ValgrindBaseSettings *settings);

    void apply() final
    {
        ValgrindGlobalSettings::instance()->group.apply();
        ValgrindGlobalSettings::instance()->writeSettings();
    }

    void setSuppressions(const QStringList &files);
    QStringList suppressions() const;

    void slotAddSuppression();
    void slotRemoveSuppression();
    void slotSuppressionsRemoved(const QStringList &files);
    void slotSuppressionsAdded(const QStringList &files);
    void slotSuppressionSelectionChanged();

private:
    void updateUi();

    ValgrindBaseSettings *m_settings;

    QPushButton *addSuppression;
    QPushButton *removeSuppression;
    QListView *suppressionList;

    QStandardItemModel m_model;
};

ValgrindConfigWidget::ValgrindConfigWidget(ValgrindBaseSettings *settings)
    : m_settings(settings)
{
    addSuppression = new QPushButton(tr("Add..."));
    removeSuppression = new QPushButton(tr("Remove"));

    suppressionList = new QListView;
    suppressionList->setModel(&m_model);
    suppressionList->setSelectionMode(QAbstractItemView::MultiSelection);

    using namespace Layouting;
    const Break nl;
    ValgrindBaseSettings &s = *settings;

    Grid generic {
        s.valgrindExecutable, nl,
        s.valgrindArguments, nl,
        s.selfModifyingCodeDetection, nl
    };

    Grid memcheck {
        s.memcheckArguments, nl,
        s.trackOrigins, nl,
        s.showReachable, nl,
        s.leakCheckOnFinish, nl,
        s.numCallers, nl,
        s.filterExternalIssues, nl,
        Item {
            Group {
                Title(tr("Suppression files:")),
                Row {
                    suppressionList,
                    Column { addSuppression, removeSuppression, Stretch() }
                }
            },
            2 // Span.
        }
    };

    Grid callgrind {
        s.callgrindArguments, nl,
        s.kcachegrindExecutable, nl,
        s.minimumInclusiveCostRatio, nl,
        s.visualizationMinimumInclusiveCostRatio, nl,
        s.enableEventToolTips, nl,
        Item {
            Group {
                s.enableCacheSim,
                s.enableBranchSim,
                s.collectSystime,
                s.collectBusEvents,
            },
            2 // Span.
        }
    };

    Column {
        Group { Title(tr("Valgrind Generic Settings")), generic },
        Group { Title(tr("MemCheck Memory Analysis Options")), memcheck },
        Group { Title(tr("CallGrind Profiling Options")), callgrind },
        Stretch(),
    }.attachTo(this);


    updateUi();
    connect(m_settings, &ValgrindBaseSettings::changed, this, &ValgrindConfigWidget::updateUi);

    connect(addSuppression, &QPushButton::clicked,
            this, &ValgrindConfigWidget::slotAddSuppression);
    connect(removeSuppression, &QPushButton::clicked,
            this, &ValgrindConfigWidget::slotRemoveSuppression);

    connect(&s, &ValgrindBaseSettings::suppressionFilesRemoved,
            this, &ValgrindConfigWidget::slotSuppressionsRemoved);
    connect(&s, &ValgrindBaseSettings::suppressionFilesAdded,
            this, &ValgrindConfigWidget::slotSuppressionsAdded);

    connect(suppressionList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ValgrindConfigWidget::slotSuppressionSelectionChanged);

    slotSuppressionSelectionChanged();
}

void ValgrindConfigWidget::updateUi()
{
    m_model.clear();
    foreach (const QString &file, m_settings->suppressionFiles())
        m_model.appendRow(new QStandardItem(file));
}

void ValgrindConfigWidget::slotAddSuppression()
{
    ValgrindGlobalSettings *conf = ValgrindGlobalSettings::instance();
    QTC_ASSERT(conf, return);
    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("Valgrind Suppression Files"),
        conf->lastSuppressionDirectory.value(),
        tr("Valgrind Suppression File (*.supp);;All Files (*)"));
    //dialog.setHistory(conf->lastSuppressionDialogHistory());
    if (!files.isEmpty()) {
        foreach (const QString &file, files)
            m_model.appendRow(new QStandardItem(file));
        m_settings->addSuppressionFiles(files);
        conf->lastSuppressionDirectory.setValue(QFileInfo(files.at(0)).absolutePath());
        //conf->setLastSuppressionDialogHistory(dialog.history());
    }
}

void ValgrindConfigWidget::slotSuppressionsAdded(const QStringList &files)
{
    QStringList filesToAdd = files;
    for (int i = 0, c = m_model.rowCount(); i < c; ++i)
        filesToAdd.removeAll(m_model.item(i)->text());

    foreach (const QString &file, filesToAdd)
        m_model.appendRow(new QStandardItem(file));
}

void ValgrindConfigWidget::slotRemoveSuppression()
{
    // remove from end so no rows get invalidated
    QList<int> rows;

    QStringList removed;
    foreach (const QModelIndex &index, suppressionList->selectionModel()->selectedIndexes()) {
        rows << index.row();
        removed << index.data().toString();
    }

    Utils::sort(rows, std::greater<int>());

    foreach (int row, rows)
        m_model.removeRow(row);

    m_settings->removeSuppressionFiles(removed);
}

void ValgrindConfigWidget::slotSuppressionsRemoved(const QStringList &files)
{
    for (int i = 0; i < m_model.rowCount(); ++i) {
        if (files.contains(m_model.item(i)->text())) {
            m_model.removeRow(i);
            --i;
        }
    }
}

void ValgrindConfigWidget::setSuppressions(const QStringList &files)
{
    m_model.clear();
    foreach (const QString &file, files)
        m_model.appendRow(new QStandardItem(file));
}

QStringList ValgrindConfigWidget::suppressions() const
{
    QStringList ret;

    for (int i = 0; i < m_model.rowCount(); ++i)
        ret << m_model.item(i)->text();

    return ret;
}

void ValgrindConfigWidget::slotSuppressionSelectionChanged()
{
    removeSuppression->setEnabled(suppressionList->selectionModel()->hasSelection());
}

// ValgrindOptionsPage

ValgrindOptionsPage::ValgrindOptionsPage()
{
    setId(ANALYZER_VALGRIND_SETTINGS);
    setDisplayName(ValgrindConfigWidget::tr("Valgrind"));
    setCategory("T.Analyzer");
    setDisplayCategory(QCoreApplication::translate("Analyzer", "Analyzer"));
    setCategoryIconPath(Analyzer::Icons::SETTINGSCATEGORY_ANALYZER);
    setWidgetCreator([] { return new ValgrindConfigWidget(ValgrindGlobalSettings::instance()); });
}

QWidget *ValgrindOptionsPage::createSettingsWidget(ValgrindBaseSettings *settings)
{
    return new ValgrindConfigWidget(settings);
}

} // namespace Internal
} // namespace Valgrind
