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

#include "pastebindotcomsettings.h"
#include "cpasterconstants.h"
#include "ui_pastebindotcomsettings.h"

#include <coreplugin/icore.h>
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>

PasteBinDotComSettings::PasteBinDotComSettings()
{
    m_settings = Core::ICore::instance()->settings();
    if (m_settings) {
        m_settings->beginGroup("PasteBinDotComSettings");
        m_hostPrefix = m_settings->value("Prefix", "").toString();
        m_settings->endGroup();
    }
}

QString PasteBinDotComSettings::id() const
{
    return QLatin1String("B.Pastebin.com");
}

QString PasteBinDotComSettings::trName() const
{
    return tr("Pastebin.com");
}

QString PasteBinDotComSettings::category() const
{
    return QLatin1String(CodePaster::Constants::CPASTER_SETTINGS_CATEGORY);
}

QString PasteBinDotComSettings::trCategory() const
{
    return QCoreApplication::translate("CodePaster", CodePaster::Constants::CPASTER_SETTINGS_TR_CATEGORY);
}

QWidget *PasteBinDotComSettings::createPage(QWidget *parent)
{
    Ui_PasteBinComSettingsWidget ui;
    QWidget *w = new QWidget(parent);
    ui.setupUi(w);
    ui.lineEdit->setText(hostPrefix());
    connect(ui.lineEdit, SIGNAL(textChanged(QString)), this, SLOT(serverChanged(QString)));
    return w;
}

void PasteBinDotComSettings::apply()
{
    if (!m_settings)
        return;

    m_settings->beginGroup("PasteBinDotComSettings");
    m_settings->setValue("Prefix", m_hostPrefix);
    m_settings->endGroup();
}

void PasteBinDotComSettings::serverChanged(const QString &prefix)
{
    m_hostPrefix = prefix;
}

QString PasteBinDotComSettings::hostPrefix() const
{
    return m_hostPrefix;
}
