/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "mcusupportconstants.h"
#include "mcusupportoptionspage.h"
#include "mcusupportoptions.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

namespace McuSupport {
namespace Internal {

class McuSupportOptionsWidget : public QWidget
{
public:
    McuSupportOptionsWidget(const McuSupportOptions *options, QWidget *parent = nullptr);

    void updateStatus();
    void showBoardPackages();
    BoardOptions *currentBoard() const;

private:
    QString m_armGccPath;
    const McuSupportOptions *m_options;
    QMap <PackageOptions*, QWidget*> m_packageWidgets;
    QMap <BoardOptions*, QWidget*> m_boardPacketWidgets;
    QFormLayout *m_packagesLayout = nullptr;
    QLabel *m_statusLabel = nullptr;
    QComboBox *m_boardComboBox = nullptr;
};

McuSupportOptionsWidget::McuSupportOptionsWidget(const McuSupportOptions *options, QWidget *parent)
    : QWidget(parent)
    , m_options(options)
{
    auto mainLayout = new QVBoxLayout(this);

    auto boardChooserlayout = new QHBoxLayout;
    auto boardChooserLabel = new QLabel(McuSupportOptionsPage::tr("Target:"));
    boardChooserlayout->addWidget(boardChooserLabel);
    m_boardComboBox = new QComboBox;
    boardChooserLabel->setBuddy(m_boardComboBox);
    boardChooserLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_boardComboBox->addItems(
                Utils::transform<QStringList>(m_options->boards, [this](BoardOptions *b){
                    return m_options->kitName(b);
                }));
    boardChooserlayout->addWidget(m_boardComboBox);
    mainLayout->addLayout(boardChooserlayout);

    auto m_packagesGroupBox = new QGroupBox(McuSupportOptionsPage::tr("Packages"));
    mainLayout->addWidget(m_packagesGroupBox);
    m_packagesLayout = new QFormLayout;
    m_packagesGroupBox->setLayout(m_packagesLayout);

    m_statusLabel = new QLabel;
    mainLayout->addWidget(m_statusLabel, 2);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);

    connect(options, &McuSupportOptions::changed, this, &McuSupportOptionsWidget::updateStatus);
    connect(m_boardComboBox, &QComboBox::currentTextChanged,
            this, &McuSupportOptionsWidget::showBoardPackages);

    showBoardPackages();
    updateStatus();
}

void McuSupportOptionsWidget::updateStatus()
{
    const BoardOptions *board = currentBoard();
    if (!board)
        return;

    m_statusLabel->setText(board->isValid()
                ? QString::fromLatin1("A kit <b>%1</b> for the selected target can be generated. "
                                      "Press Apply to generate it.").arg(m_options->kitName(board))
                : QString::fromLatin1("Provide the package paths in order to create a kit for "
                                      "your target."));
}

void McuSupportOptionsWidget::showBoardPackages()
{
    const BoardOptions *board = currentBoard();
    if (!board)
        return;

    while (m_packagesLayout->rowCount() > 0) {
        QFormLayout::TakeRowResult row = m_packagesLayout->takeRow(0);
        row.labelItem->widget()->hide();
        row.fieldItem->widget()->hide();
    }

    for (auto package : m_options->packages) {
        QWidget *packageWidget = package->widget();
        if (!board->packages().contains(package))
            continue;
        m_packagesLayout->addRow(package->label(), packageWidget);
        packageWidget->show();
    }

    updateStatus();
}

BoardOptions *McuSupportOptionsWidget::currentBoard() const
{
    const int boardIndex = m_boardComboBox->currentIndex();
    return m_options->boards.isEmpty() ? nullptr : m_options->boards.at(boardIndex);
}

McuSupportOptionsPage::McuSupportOptionsPage(QObject* parent)
    : Core::IOptionsPage(parent)
{
    setId(Core::Id(Constants::SETTINGS_ID));
    setDisplayName(tr("MCU"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
}

QWidget *McuSupportOptionsPage::widget()
{
    if (!m_options)
        m_options = new McuSupportOptions(this);
    if (!m_widget)
        m_widget = new McuSupportOptionsWidget(m_options);
    return m_widget;
}

void McuSupportOptionsPage::apply()
{
    for (auto package : m_options->packages)
        package->writeToSettings();

    QTC_ASSERT(m_options->toolchainPackage, return);
    QTC_ASSERT(m_options->qulSdkPackage, return);

    const BoardOptions *board = m_widget->currentBoard();
    if (!board)
        return;

    using namespace ProjectExplorer;

    for (auto existingKit : m_options->existingKits(board))
        ProjectExplorer::KitManager::deregisterKit(existingKit);
    m_options->newKit(board);
}

void McuSupportOptionsPage::finish()
{
    delete m_options;
    m_options = nullptr;
    delete m_widget;
}

} // Internal
} // McuSupport
