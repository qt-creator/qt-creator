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

#include "codepastersettings.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QSettings>
#include <QCoreApplication>
#include <QLineEdit>
#include <QFileDialog>
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QDebug>
#include <QVariant>

static const char settingsGroupC[] = "CodePasterSettings";
static const char serverKeyC[] = "Server";

namespace CodePaster {

CodePasterSettingsPage::CodePasterSettingsPage()
{
    setId("C.CodePaster");
    setDisplayName(tr("CodePaster"));
    setCategory(Constants::CPASTER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("CodePaster",
        Constants::CPASTER_SETTINGS_TR_CATEGORY));

    m_settings = Core::ICore::settings();
    if (m_settings) {
        const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
        m_host = m_settings->value(keyRoot + QLatin1String(serverKeyC), QString()).toString();
    }
}

QWidget *CodePasterSettingsPage::createPage(QWidget *parent)
{
    QWidget *outerWidget = new QWidget(parent);
    QVBoxLayout *outerLayout = new QVBoxLayout(outerWidget);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *lineEdit = new QLineEdit(m_host);
    connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(serverChanged(QString)));
    formLayout->addRow(tr("Server:"), lineEdit);
    outerLayout->addLayout(formLayout);
    outerLayout->addSpacerItem(new QSpacerItem(0, 3, QSizePolicy::Ignored, QSizePolicy::Fixed));

    QLabel *noteLabel = new QLabel(tr("<i>Note: Specify the host name for the CodePaster service "
                                      "without any protocol prepended (e.g. codepaster.mycompany.com).</i>"));
    noteLabel->setWordWrap(true);
    outerLayout->addWidget(noteLabel);

    outerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    return outerWidget;
}

void CodePasterSettingsPage::apply()
{
    if (!m_settings)
        return;

    m_settings->beginGroup(QLatin1String(settingsGroupC));
    m_settings->setValue(QLatin1String(serverKeyC), m_host);
    m_settings->endGroup();
}

void CodePasterSettingsPage::serverChanged(const QString &host)
{
    m_host = host.trimmed();
}

QString CodePasterSettingsPage::hostName() const
{
    return m_host;
}
} // namespace CodePaster
