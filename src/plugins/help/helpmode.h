/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef HELPMODE_H
#define HELPMODE_H

#include "helpmode.h"
#include "helpconstants.h"

#include <coreplugin/imode.h>

#include <QtCore/QString>
#include <QtGui/QIcon>

namespace Help {
namespace Internal {

class HelpMode : public Core::IMode
{
public:
    HelpMode(QObject *parent = 0);

    QString displayName() const { return tr("Help"); }
    QIcon icon() const { return m_icon; }
    int priority() const { return Constants::P_MODE_HELP; }
    QWidget *widget() { return m_widget; }
    QString id() const { return QLatin1String(Constants::ID_MODE_HELP); }
    QString type() const { return QString(); }
    Core::Context context() const { return Core::Context(Constants::C_MODE_HELP); }
    QString contextHelpId() const { return QString(); }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QWidget *m_widget;
    QIcon m_icon;
};

} // namespace Internal
} // namespace Help

#endif // HELPMODE_H
