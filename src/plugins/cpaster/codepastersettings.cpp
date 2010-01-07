/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "codepastersettings.h"
#include "cpasterconstants.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QVBoxLayout>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

using namespace CodePaster;

CodePasterSettingsPage::CodePasterSettingsPage()
{
    m_settings = Core::ICore::instance()->settings();
    if (m_settings) {
        m_settings->beginGroup("CodePasterSettings");
        m_host = m_settings->value("Server", "").toString();
        m_settings->endGroup();
    }
}

QString CodePasterSettingsPage::id() const
{
    return QLatin1String("C.CodePaster");
}

QString CodePasterSettingsPage::displayName() const
{
    return tr("CodePaster");
}

QString CodePasterSettingsPage::category() const
{
    return QLatin1String(Constants::CPASTER_SETTINGS_CATEGORY);
}

QString CodePasterSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("CodePaster", Constants::CPASTER_SETTINGS_TR_CATEGORY);
}

QWidget *CodePasterSettingsPage::createPage(QWidget *parent)
{    
    QGroupBox *groupBox = new QGroupBox();
    QVBoxLayout *groupBoxLayout = new QVBoxLayout(groupBox);
    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *lineedit = new QLineEdit(m_host);
    connect(lineedit, SIGNAL(textChanged(QString)), this, SLOT(serverChanged(QString)));
    formLayout->addRow(tr("Server:"), lineedit);
    groupBoxLayout->addLayout(formLayout);
    groupBoxLayout->addSpacerItem(new QSpacerItem(0, 60, QSizePolicy::Ignored, QSizePolicy::Fixed));

    QLabel *noteLabel = new QLabel(tr("Note: Specify the host name for the CodePaster service "
                                      "without any protocol prepended (e.g. codepaster.mycompany.com)."));
    noteLabel->setWordWrap(true);
    groupBoxLayout->addWidget(noteLabel);

    QWidget *outerWidget = new QWidget(parent);
    QVBoxLayout *outerLayout = new QVBoxLayout(outerWidget);
    outerLayout->addWidget(groupBox);
    outerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    return outerWidget;
}

void CodePasterSettingsPage::apply()
{
    if (!m_settings)
        return;

    m_settings->beginGroup("CodePasterSettings");
    m_settings->setValue("Server", m_host);
    m_settings->endGroup();
}

void CodePasterSettingsPage::serverChanged(const QString &host)
{
    m_host = host;
}

QString CodePasterSettingsPage::hostName() const
{
    return m_host;
}
