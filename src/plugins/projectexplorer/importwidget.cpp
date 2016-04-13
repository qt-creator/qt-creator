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

#include "importwidget.h"

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QPushButton>
#include <QVBoxLayout>

namespace ProjectExplorer {
namespace Internal {

ImportWidget::ImportWidget(QWidget *parent) :
    QWidget(parent),
    m_pathChooser(new Utils::PathChooser)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto vboxLayout = new QVBoxLayout();
    setLayout(vboxLayout);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    auto detailsWidget = new Utils::DetailsWidget(this);
    detailsWidget->setUseCheckBox(false);
    detailsWidget->setSummaryText(tr("Import Build From..."));
    detailsWidget->setSummaryFontBold(true);
    // m_detailsWidget->setIcon(); // FIXME: Set icon!
    vboxLayout->addWidget(detailsWidget);

    auto widget = new QWidget;
    auto layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pathChooser);

    m_pathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_pathChooser->setHistoryCompleter(QLatin1String("Import.SourceDir.History"));
    auto importButton = new QPushButton(tr("Import"), widget);
    layout->addWidget(importButton);

    connect(importButton, &QAbstractButton::clicked, this, &ImportWidget::handleImportRequest);

    detailsWidget->setWidget(widget);
}

void ImportWidget::setCurrentDirectory(const Utils::FileName &dir)
{
    m_pathChooser->setBaseFileName(dir);
    m_pathChooser->setFileName(dir);
}

void ImportWidget::handleImportRequest()
{
    Utils::FileName dir = m_pathChooser->fileName();
    emit importFrom(dir);

    m_pathChooser->setFileName(m_pathChooser->baseFileName());
}

} // namespace Internal
} // namespace ProjectExplorer
