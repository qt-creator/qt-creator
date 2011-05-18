/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "analyzerrunconfigwidget.h"

#include "analyzersettings.h"

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtGui/QGroupBox>
#include <QtGui/QVBoxLayout>

namespace Analyzer {

AnalyzerRunConfigWidget::AnalyzerRunConfigWidget()
    : m_detailsWidget(new Utils::DetailsWidget(this))
{
    QWidget *mainWidget = new QWidget(this);
    new QVBoxLayout(mainWidget);
    m_detailsWidget->setWidget(mainWidget);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_detailsWidget);
}

QString AnalyzerRunConfigWidget::displayName() const
{
    return tr("Analyzer Settings");
}

void AnalyzerRunConfigWidget::setRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    QTC_ASSERT(rc, return);

    AnalyzerProjectSettings *settings = rc->extraAspect<AnalyzerProjectSettings>();
    QTC_ASSERT(settings, return);

    // update summary text
    QStringList tools;
    foreach (AbstractAnalyzerSubConfig *config, settings->subConfigs()) {
        tools << QString("<strong>%1</strong>").arg(config->displayName());
    }
    m_detailsWidget->setSummaryText(tr("Available settings: %1").arg(tools.join(", ")));

    // add group boxes for each sub config
    QLayout *layout = m_detailsWidget->widget()->layout();
    foreach (AbstractAnalyzerSubConfig *config, settings->subConfigs()) {
        (void) new QGroupBox(config->displayName());
        QWidget *widget = config->createConfigWidget(this);
        layout->addWidget(widget);
    }
}

} // namespace Analyzer
