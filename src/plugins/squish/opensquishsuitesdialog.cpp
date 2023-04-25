// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "opensquishsuitesdialog.h"

#include "squishtr.h"

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>

#include <QAbstractButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>

namespace Squish {
namespace Internal {

OpenSquishSuitesDialog::OpenSquishSuitesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Tr::tr("Open Squish Test Suites"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setModal(true);

    m_directoryLineEdit = new Utils::PathChooser;
    m_directoryLineEdit->setHistoryCompleter("Squish.SuitesBase", true);

    m_suitesListWidget = new QListWidget;

    auto selectAllPushButton = new QPushButton(Tr::tr("Select All"));

    auto deselectAllPushButton = new QPushButton(Tr::tr("Deselect All"));

    m_buttonBox = new QDialogButtonBox;
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Open);
    m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    using namespace Layouting;

    Column {
        new QLabel(Tr::tr("Base directory:")),
        m_directoryLineEdit,
        new QLabel(Tr::tr("Test suites:")),
        Row {
            m_suitesListWidget,
            Column {
                selectAllPushButton,
                deselectAllPushButton,
                st
            }
        },
        m_buttonBox
    }.attachTo(this);

    connect(m_directoryLineEdit,
            &Utils::PathChooser::textChanged,
            this,
            &OpenSquishSuitesDialog::onDirectoryChanged);
    connect(selectAllPushButton,
            &QPushButton::clicked,
            this,
            &OpenSquishSuitesDialog::selectAll);
    connect(deselectAllPushButton,
            &QPushButton::clicked,
            this,
            &OpenSquishSuitesDialog::deselectAll);
    connect(this, &OpenSquishSuitesDialog::accepted, this, &OpenSquishSuitesDialog::setChosenSuites);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    onDirectoryChanged();
}

OpenSquishSuitesDialog::~OpenSquishSuitesDialog() = default;

void OpenSquishSuitesDialog::onDirectoryChanged()
{
    m_suitesListWidget->clear();
    m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    const Utils::FilePath baseDir = m_directoryLineEdit->filePath();
    if (!baseDir.exists())
        return;

    const Utils::FilePaths subDirs = baseDir.dirEntries(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const Utils::FilePath &subDir : subDirs) {
        if (!subDir.baseName().startsWith("suite_"))
            continue;
        if (subDir.pathAppended("suite.conf").isReadableFile()) {
            QListWidgetItem *item = new QListWidgetItem(subDir.baseName(), m_suitesListWidget);
            item->setCheckState(Qt::Checked);
            connect(m_suitesListWidget,
                    &QListWidget::itemChanged,
                    this,
                    &OpenSquishSuitesDialog::onListItemChanged);
        }
    }
    m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(m_suitesListWidget->count());
}

void OpenSquishSuitesDialog::onListItemChanged(QListWidgetItem *)
{
    const int count = m_suitesListWidget->count();
    for (int row = 0; row < count; ++row) {
        if (m_suitesListWidget->item(row)->checkState() == Qt::Checked) {
            m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
            return;
        }
    }
    m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
}

void OpenSquishSuitesDialog::selectAll()
{
    const int count = m_suitesListWidget->count();
    for (int row = 0; row < count; ++row)
        m_suitesListWidget->item(row)->setCheckState(Qt::Checked);
}

void OpenSquishSuitesDialog::deselectAll()
{
    const int count = m_suitesListWidget->count();
    for (int row = 0; row < count; ++row)
        m_suitesListWidget->item(row)->setCheckState(Qt::Unchecked);
}

void OpenSquishSuitesDialog::setChosenSuites()
{
    const int count = m_suitesListWidget->count();
    const Utils::FilePath baseDir = m_directoryLineEdit->filePath();
    for (int row = 0; row < count; ++row) {
        QListWidgetItem *item = m_suitesListWidget->item(row);
        if (item->checkState() == Qt::Checked)
            m_chosenSuites.append(baseDir.pathAppended(item->text()));
    }
}

} // namespace Internal
} // namespace Squish
