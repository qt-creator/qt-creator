/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "pastebindotcomsettings.h"
#include "cpasterconstants.h"
#include "ui_pastebindotcomsettings.h"

#include <coreplugin/icore.h>
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>

static const char groupC[] = "PasteBinDotComSettings";
static const char prefixKeyC[] = "Prefix";

namespace CodePaster {
PasteBinDotComSettings::PasteBinDotComSettings()
{
    m_settings = Core::ICore::instance()->settings();
    if (m_settings) {
        const QString rootKey = QLatin1String(groupC) + QLatin1Char('/');
        m_hostPrefix = m_settings->value(rootKey + QLatin1String(prefixKeyC), QString()).toString();
    }
}

QString PasteBinDotComSettings::id() const
{
    return QLatin1String("B.Pastebin.com");
}

QString PasteBinDotComSettings::displayName() const
{
    return tr("Pastebin.com");
}

QString PasteBinDotComSettings::category() const
{
    return QLatin1String(CodePaster::Constants::CPASTER_SETTINGS_CATEGORY);
}

QString PasteBinDotComSettings::displayCategory() const
{
    return QCoreApplication::translate("CodePaster", CodePaster::Constants::CPASTER_SETTINGS_TR_CATEGORY);
}

QIcon PasteBinDotComSettings::categoryIcon() const
{
    return QIcon(); // TODO: Icon for CodePaster
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

    m_settings->beginGroup(QLatin1String(groupC));
    m_settings->setValue(QLatin1String(prefixKeyC), m_hostPrefix);
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
} //namespace CodePaster
