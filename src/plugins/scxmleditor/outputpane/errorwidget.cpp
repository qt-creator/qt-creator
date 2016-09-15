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

#include "errorwidget.h"
#include "scxmleditorconstants.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QTextStream>

#include <coreplugin/icore.h>
#include <utils/utilsicons.h>

using namespace ScxmlEditor::OutputPane;

ErrorWidget::ErrorWidget(QWidget *parent)
    : OutputPane(parent)
    , m_warningModel(new WarningModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    m_ui.setupUi(this);

    m_ui.m_clean->setIcon(Utils::Icons::CLEAN.icon());
    m_ui.m_exportWarnings->setIcon(Utils::Icons::SAVEFILE.icon());
    m_ui.m_showErrors->setIcon(Utils::Icons::ERROR.icon());
    m_ui.m_showWarnings->setIcon(Utils::Icons::WARNING.icon());
    m_ui.m_showInfos->setIcon(Utils::Icons::INFO.icon());

    m_proxyModel->setFilterRole(WarningModel::FilterRole);
    m_proxyModel->setSourceModel(m_warningModel);
    m_proxyModel->setFilterFixedString(Constants::C_WARNINGMODEL_FILTER_ACTIVE);

    m_ui.m_errorsTable->setModel(m_proxyModel);
    m_ui.m_errorsTable->horizontalHeader()->setSectionsMovable(true);

    connect(m_ui.m_errorsTable, &TableView::entered, [this](const QModelIndex &ind) {
        if (ind.isValid())
            emit warningEntered(m_warningModel->getWarning(m_proxyModel->mapToSource(ind)));
    });

    connect(m_ui.m_errorsTable, &TableView::pressed, [this](const QModelIndex &ind) {
        if (ind.isValid())
            emit warningSelected(m_warningModel->getWarning(m_proxyModel->mapToSource(ind)));
    });

    connect(m_ui.m_errorsTable, &TableView::doubleClicked, [this](const QModelIndex &ind) {
        if (ind.isValid())
            emit warningDoubleClicked(m_warningModel->getWarning(m_proxyModel->mapToSource(ind)));
    });

    connect(m_ui.m_errorsTable, &TableView::mouseExited, this, [this](){
        emit mouseExited();
    });

    connect(m_ui.m_showErrors, &QToolButton::toggled, [this](bool show) {
        m_warningModel->setShowWarnings(Warning::ErrorType, show);
    });

    connect(m_ui.m_showWarnings, &QToolButton::toggled, [this](bool show) {
        m_warningModel->setShowWarnings(Warning::WarningType, show);
    });

    connect(m_ui.m_showInfos, &QToolButton::toggled, [this](bool show) {
        m_warningModel->setShowWarnings(Warning::InfoType, show);
    });

    connect(m_ui.m_clean, &QToolButton::clicked, m_warningModel, &WarningModel::clear);
    connect(m_ui.m_exportWarnings, &QToolButton::clicked, this, &ErrorWidget::exportWarnings);
    connect(m_warningModel, &WarningModel::warningsChanged, this, &ErrorWidget::updateWarnings);
    connect(m_warningModel, &WarningModel::countChanged, this, &ErrorWidget::warningCountChanged);

    const QSettings *s = Core::ICore::settings();
    m_ui.m_errorsTable->restoreGeometry(s->value(Constants::C_SETTINGS_ERRORPANE_GEOMETRY).toByteArray());
    m_ui.m_showErrors->setChecked(s->value(Constants::C_SETTINGS_ERRORPANE_SHOWERRORS, true).toBool());
    m_ui.m_showWarnings->setChecked(s->value(Constants::C_SETTINGS_ERRORPANE_SHOWWARNINGS, true).toBool());
    m_ui.m_showInfos->setChecked(s->value(Constants::C_SETTINGS_ERRORPANE_SHOWINFOS, true).toBool());

    updateWarnings();
}

ErrorWidget::~ErrorWidget()
{
    QSettings *s = Core::ICore::settings();
    s->setValue(Constants::C_SETTINGS_ERRORPANE_GEOMETRY, m_ui.m_errorsTable->saveGeometry());
    s->setValue(Constants::C_SETTINGS_ERRORPANE_SHOWERRORS, m_ui.m_showErrors->isChecked());
    s->setValue(Constants::C_SETTINGS_ERRORPANE_SHOWWARNINGS, m_ui.m_showWarnings->isChecked());
    s->setValue(Constants::C_SETTINGS_ERRORPANE_SHOWINFOS, m_ui.m_showInfos->isChecked());
}

void ErrorWidget::setPaneFocus()
{
}

void ErrorWidget::updateWarnings()
{
    int errorCount = m_warningModel->count(Warning::ErrorType);
    int warningCount = m_warningModel->count(Warning::WarningType);
    int infoCount = m_warningModel->count(Warning::InfoType);

    m_title = tr("Errors(%1) / Warnings(%2) / Info(%3)").arg(errorCount).arg(warningCount).arg(infoCount);
    if (errorCount > 0)
        m_icon = m_ui.m_showInfos->icon();
    else if (warningCount > 0)
        m_icon = m_ui.m_showWarnings->icon();
    else if (infoCount > 0)
        m_icon = m_ui.m_showErrors->icon();
    else
        m_icon = QIcon();

    emit titleChanged();
    emit iconChanged();
}

QColor ErrorWidget::alertColor() const
{
    if (m_warningModel->count(Warning::ErrorType) > 0)
        return QColor(0xff, 0x77, 0x77);
    else if (m_warningModel->count(Warning::WarningType))
        return QColor(0xfd, 0x88, 0x21);
    else
        return QColor(0x29, 0xb6, 0xff);
}

void ErrorWidget::warningCountChanged(int c)
{
    if (c > 0)
        emit dataChanged();
}

QString ErrorWidget::modifyExportedValue(const QString &val)
{
    QString value(val);

    if (value.contains(",") || value.startsWith(" ") || value.endsWith(" "))
        value = QString::fromLatin1("\"%1\"").arg(value);

    return value;
}

void ErrorWidget::exportWarnings()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export To File"), QString(), tr("CSV files (*.csv)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Cannot open file %.").arg(fileName));
        file.close();
        return;
    }

    QTextStream out(&file);

    // Headerdata
    QStringList values;
    for (int c = 0; c < m_proxyModel->columnCount(); ++c)
        values << modifyExportedValue(m_proxyModel->headerData(m_ui.m_errorsTable->horizontalHeader()->visualIndex(c), Qt::Horizontal, Qt::DisplayRole).toString());
    out << values.join(",") << "\n";

    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        values.clear();
        for (int c = 0; c < m_proxyModel->columnCount(); ++c)
            values << modifyExportedValue(m_proxyModel->data(m_proxyModel->index(r, m_ui.m_errorsTable->horizontalHeader()->visualIndex(c)), Qt::DisplayRole).toString());
        out << values.join(",") << "\n";
    }
}
