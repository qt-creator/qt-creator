// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "errorwidget.h"
#include "scxmleditorconstants.h"
#include "scxmleditortr.h"
#include "tableview.h"

#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLayout>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QTextStream>
#include <QToolBar>
#include <QToolButton>

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/utilsicons.h>

using namespace ScxmlEditor::OutputPane;
using namespace Utils;

ErrorWidget::ErrorWidget(QWidget *parent)
    : OutputPane(parent)
    , m_warningModel(new WarningModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
{
    createUi();

    m_proxyModel->setFilterRole(WarningModel::FilterRole);
    m_proxyModel->setSourceModel(m_warningModel);
    m_proxyModel->setFilterFixedString(Constants::C_WARNINGMODEL_FILTER_ACTIVE);

    m_errorsTable->setModel(m_proxyModel);

    connect(m_errorsTable, &TableView::entered, this, [this](const QModelIndex &ind) {
        if (ind.isValid())
            emit warningEntered(m_warningModel->getWarning(m_proxyModel->mapToSource(ind)));
    });

    connect(m_errorsTable, &TableView::pressed, this, [this](const QModelIndex &ind) {
        if (ind.isValid())
            emit warningSelected(m_warningModel->getWarning(m_proxyModel->mapToSource(ind)));
    });

    connect(m_errorsTable, &TableView::doubleClicked, this, [this](const QModelIndex &ind) {
        if (ind.isValid())
            emit warningDoubleClicked(m_warningModel->getWarning(m_proxyModel->mapToSource(ind)));
    });

    connect(m_errorsTable, &TableView::mouseExited, this, [this] {
        emit mouseExited();
    });

    connect(m_showErrors, &QToolButton::toggled, this, [this](bool show) {
        m_warningModel->setShowWarnings(Warning::ErrorType, show);
    });

    connect(m_showWarnings, &QToolButton::toggled, this, [this](bool show) {
        m_warningModel->setShowWarnings(Warning::WarningType, show);
    });

    connect(m_showInfos, &QToolButton::toggled, this, [this](bool show) {
        m_warningModel->setShowWarnings(Warning::InfoType, show);
    });

    connect(m_clean, &QToolButton::clicked, m_warningModel, &WarningModel::clear);
    connect(m_exportWarnings, &QToolButton::clicked, this, &ErrorWidget::exportWarnings);
    connect(m_warningModel, &WarningModel::warningsChanged, this, &ErrorWidget::updateWarnings);
    connect(m_warningModel, &WarningModel::countChanged, this, &ErrorWidget::warningCountChanged);

    const QtcSettings *s = Core::ICore::settings();
    m_errorsTable->restoreGeometry(s->value(Constants::C_SETTINGS_ERRORPANE_GEOMETRY).toByteArray());
    m_showErrors->setChecked(s->value(Constants::C_SETTINGS_ERRORPANE_SHOWERRORS, true).toBool());
    m_showWarnings->setChecked(s->value(Constants::C_SETTINGS_ERRORPANE_SHOWWARNINGS, true).toBool());
    m_showInfos->setChecked(s->value(Constants::C_SETTINGS_ERRORPANE_SHOWINFOS, true).toBool());

    updateWarnings();
}

ErrorWidget::~ErrorWidget()
{
    QtcSettings *s = Core::ICore::settings();
    s->setValue(Constants::C_SETTINGS_ERRORPANE_GEOMETRY, m_errorsTable->saveGeometry());
    s->setValue(Constants::C_SETTINGS_ERRORPANE_SHOWERRORS, m_showErrors->isChecked());
    s->setValue(Constants::C_SETTINGS_ERRORPANE_SHOWWARNINGS, m_showWarnings->isChecked());
    s->setValue(Constants::C_SETTINGS_ERRORPANE_SHOWINFOS, m_showInfos->isChecked());
}

void ErrorWidget::setPaneFocus()
{
}

void ErrorWidget::createUi()
{
    m_clean = new QToolButton;
    m_clean->setIcon(Utils::Icons::CLEAN_TOOLBAR.icon());
    m_exportWarnings = new QToolButton;
    m_exportWarnings->setIcon(Utils::Icons::SAVEFILE_TOOLBAR.icon());
    m_showErrors = new QToolButton;
    m_showErrors->setIcon(Utils::Icons::CRITICAL_TOOLBAR.icon());
    m_showErrors->setCheckable(true);
    m_showWarnings = new QToolButton;
    m_showWarnings->setIcon(Utils::Icons::WARNING_TOOLBAR.icon());
    m_showWarnings->setCheckable(true);
    m_showInfos = new QToolButton;
    m_showInfos->setIcon(Utils::Icons::INFO_TOOLBAR.icon());
    m_showInfos->setCheckable(true);

    auto toolBar = new QToolBar;
    toolBar->addWidget(m_clean);
    toolBar->addWidget(m_exportWarnings);
    toolBar->addWidget(m_showErrors);
    toolBar->addWidget(m_showWarnings);
    toolBar->addWidget(m_showInfos);
    auto stretch = new QWidget;
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    toolBar->addWidget(stretch);

    m_errorsTable = new TableView;
    m_errorsTable->horizontalHeader()->setSectionsMovable(true);
    m_errorsTable->horizontalHeader()->setStretchLastSection(true);
    m_errorsTable->setTextElideMode(Qt::ElideRight);
    m_errorsTable->setSortingEnabled(true);
    m_errorsTable->setAlternatingRowColors(true);
    m_errorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_errorsTable->setFrameShape(QFrame::NoFrame);

    setLayout(new QVBoxLayout);
    layout()->addWidget(toolBar);
    layout()->addWidget(m_errorsTable);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
}

void ErrorWidget::updateWarnings()
{
    int errorCount = m_warningModel->count(Warning::ErrorType);
    int warningCount = m_warningModel->count(Warning::WarningType);
    int infoCount = m_warningModel->count(Warning::InfoType);

    m_title = Tr::tr("Errors(%1) / Warnings(%2) / Info(%3)").arg(errorCount).arg(warningCount).arg(infoCount);
    if (errorCount > 0)
        m_icon = m_showErrors->icon();
    else if (warningCount > 0)
        m_icon = m_showWarnings->icon();
    else if (infoCount > 0)
        m_icon = m_showInfos->icon();
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
    FilePath fileName = FileUtils::getSaveFilePath(this, Tr::tr("Export to File"), {}, Tr::tr("CSV files (*.csv)"));
    if (fileName.isEmpty())
        return;

    QFile file(fileName.toString());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, Tr::tr("Export Failed"), Tr::tr("Cannot open file %1.").arg(fileName.toUserOutput()));
        file.close();
        return;
    }

    QTextStream out(&file);

    // Headerdata
    QStringList values;
    for (int c = 0; c < m_proxyModel->columnCount(); ++c)
        values << modifyExportedValue(m_proxyModel->headerData(m_errorsTable->horizontalHeader()->visualIndex(c), Qt::Horizontal, Qt::DisplayRole).toString());
    out << values.join(",") << "\n";

    for (int r = 0; r < m_proxyModel->rowCount(); ++r) {
        values.clear();
        for (int c = 0; c < m_proxyModel->columnCount(); ++c)
            values << modifyExportedValue(m_proxyModel->data(m_proxyModel->index(r, m_errorsTable->horizontalHeader()->visualIndex(c)), Qt::DisplayRole).toString());
        out << values.join(",") << "\n";
    }
}
