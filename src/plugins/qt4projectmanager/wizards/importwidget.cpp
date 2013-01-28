/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "importwidget.h"

#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QFileInfo>

#include <QPushButton>
#include <QVBoxLayout>

namespace Qt4ProjectManager {
namespace Internal {

ImportWidget::ImportWidget(QWidget *parent) :
    QWidget(parent),
    m_pathChooser(new Utils::PathChooser)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QVBoxLayout *vboxLayout = new QVBoxLayout();
    setLayout(vboxLayout);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    Utils::DetailsWidget *detailsWidget = new Utils::DetailsWidget(this);
    detailsWidget->setUseCheckBox(false);
    detailsWidget->setSummaryText(tr("Import Build from..."));
    detailsWidget->setSummaryFontBold(true);
    // m_detailsWidget->setIcon(); // FIXME: Set icon!
    vboxLayout->addWidget(detailsWidget);

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_pathChooser);

    m_pathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    QPushButton *importButton = new QPushButton(tr("Import"), widget);
    layout->addWidget(importButton);

    connect(importButton, SIGNAL(clicked()), this, SLOT(handleImportRequest()));

    detailsWidget->setWidget(widget);
}

ImportWidget::~ImportWidget()
{ }

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
} // namespace Qt4ProjectManager
