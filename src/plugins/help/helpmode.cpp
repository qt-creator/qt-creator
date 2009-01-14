/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "helpmode.h"
#include "helpplugin.h"

#include <QtCore/QLatin1String>
#include <QtGui/QWidget>
#include <coreplugin/findplaceholder.h>

using namespace Help;
using namespace Help::Internal;

HelpMode::HelpMode(QWidget *widget, QWidget *centralWidget, QObject *parent)
    : BaseMode(parent), m_centralWidget(centralWidget)
{
    setName(tr("Help"));
    setUniqueModeName(Constants::ID_MODE_HELP);
    setIcon(QIcon(QLatin1String(":/fancyactionbar/images/mode_Reference.png")));
    setPriority(Constants::P_MODE_HELP);
    setWidget(widget);
    m_centralWidget->layout()->setSpacing(0);
    m_centralWidget->layout()->addWidget(new Core::FindToolBarPlaceHolder(this));
}


