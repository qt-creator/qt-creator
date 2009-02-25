/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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


