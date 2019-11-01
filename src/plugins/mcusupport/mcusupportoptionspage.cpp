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
    void showBoardPackages(int boardIndex);

private:
    QString m_armGccPath;
    const McuSupportOptions *m_options;
    int m_currentBoardIndex = 0;
    QMap <PackageOptions*, QWidget*> m_packageWidgets;
    QMap <BoardOptions*, QWidget*> m_boardPacketWidgets;
    QFormLayout *m_packagesLayout = nullptr;
    QLabel *m_statusLabel = nullptr;
};

McuSupportOptionsWidget::McuSupportOptionsWidget(const McuSupportOptions *options, QWidget *parent)
    : QWidget(parent)
    , m_options(options)
{
    auto mainLayout = new QVBoxLayout(this);

    auto boardChooserlayout = new QHBoxLayout;
    auto boardChooserLabel = new QLabel(McuSupportOptionsPage::tr("Target:"));
    boardChooserlayout->addWidget(boardChooserLabel);
    auto boardComboBox = new QComboBox;
    boardChooserLabel->setBuddy(boardComboBox);
    boardChooserLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    boardComboBox->addItems(Utils::transform<QStringList>(m_options->boards, [](BoardOptions *b){
                                 return b->model();}));
    boardChooserlayout->addWidget(boardComboBox);
    mainLayout->addLayout(boardChooserlayout);

    auto m_packagesGroupBox = new QGroupBox(McuSupportOptionsPage::tr("Packages"));
    mainLayout->addWidget(m_packagesGroupBox);
    m_packagesLayout = new QFormLayout;
    m_packagesGroupBox->setLayout(m_packagesLayout);

    m_statusLabel = new QLabel;
    mainLayout->addWidget(m_statusLabel);
    m_statusLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    connect(options, &McuSupportOptions::changed, this, &McuSupportOptionsWidget::updateStatus);
    connect(boardComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &McuSupportOptionsWidget::showBoardPackages);

    showBoardPackages(m_currentBoardIndex);
}

static QString ulOfBoardModels(const QVector<BoardOptions*> &validBoards)
{
    return "<ul><li>"
        + Utils::transform<QStringList>(validBoards,[](BoardOptions* board)
                                                    {return board->model();}).join("</li><li>")
        + "</li></ul>";
}

void McuSupportOptionsWidget::updateStatus()
{
    const QVector<BoardOptions*> validBoards = m_options->validBoards();
    m_statusLabel->setText(validBoards.isEmpty()
                           ? McuSupportOptionsPage::tr("No devices and kits can currently be generated. "
                                                       "Select a board and provide the package paths. "
                                                       "Afterwards, press Apply to generate device and kit for "
                                                       "your board.")
                           : McuSupportOptionsPage::tr("Devices and kits for the following boards can be generated: "
                                                       "%1 "
                                                       "Press Apply to generate device and kit for "
                                                       "your board.").arg(ulOfBoardModels(validBoards)));
}

void McuSupportOptionsWidget::showBoardPackages(int boardIndex)
{
    while (m_packagesLayout->rowCount() > 0) {
        QFormLayout::TakeRowResult row = m_packagesLayout->takeRow(0);
        row.labelItem->widget()->hide();
        row.fieldItem->widget()->hide();
    }

    const BoardOptions *currentBoard = m_options->boards.at(boardIndex);

    for (auto package : m_options->packages) {
        QWidget *packageWidget = package->widget();
        if (!currentBoard->packages().contains(package))
            continue;
        m_packagesLayout->addRow(package->label(), packageWidget);
        packageWidget->show();
    }
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

    const QVector<BoardOptions*> validBoards = m_options->validBoards();

    using namespace ProjectExplorer;

    for (auto board : validBoards)
        m_options->kit(board);
}

void McuSupportOptionsPage::finish()
{
    delete m_options;
    m_options = nullptr;
    delete m_widget;
}

} // Internal
} // McuSupport
