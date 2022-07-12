/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "opensquishsuitesdialog.h"

#include "squishutils.h"

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

static QString previousPath;

OpenSquishSuitesDialog::OpenSquishSuitesDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Open Squish Test Suites"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setModal(true);

    m_directoryLineEdit = new Utils::PathChooser;
    m_directoryLineEdit->setPath(previousPath);

    m_suitesListWidget = new QListWidget;

    auto selectAllPushButton = new QPushButton(tr("Select All"));

    auto deselectAllPushButton = new QPushButton(tr("Deselect All"));

    m_buttonBox = new QDialogButtonBox;
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Open);
    m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);

    using namespace Utils::Layouting;

    Column {
        new QLabel(tr("Base directory:")),
        m_directoryLineEdit,
        new QLabel(tr("Test suites:")),
        Row {
            m_suitesListWidget,
            Column {
                selectAllPushButton,
                deselectAllPushButton,
                Stretch()
            }
        },
        m_buttonBox
    }.attachTo(this);

    connect(m_directoryLineEdit,
            &Utils::PathChooser::pathChanged,
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
}

OpenSquishSuitesDialog::~OpenSquishSuitesDialog() = default;

void OpenSquishSuitesDialog::onDirectoryChanged()
{
    m_suitesListWidget->clear();
    m_buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
    QDir baseDir(m_directoryLineEdit->path());
    if (!baseDir.exists()) {
        return;
    }

    const QFileInfoList subDirs = baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDir : subDirs) {
        if (!subDir.baseName().startsWith("suite_"))
            continue;
        if (SquishUtils::validTestCases(subDir.absoluteFilePath()).size()) {
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
    previousPath = m_directoryLineEdit->path();
    const QDir baseDir(previousPath);
    for (int row = 0; row < count; ++row) {
        QListWidgetItem *item = m_suitesListWidget->item(row);
        if (item->checkState() == Qt::Checked)
            m_chosenSuites.append(QFileInfo(baseDir, item->text()).absoluteFilePath());
    }
}

} // namespace Internal
} // namespace Squish
