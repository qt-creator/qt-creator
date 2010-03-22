/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef COMMUNITYWELCOMEPAGE_H
#define COMMUNITYWELCOMEPAGE_H

#include "welcome_global.h"

#include <utils/iwelcomepage.h>

namespace Welcome {
namespace Internal {

class CommunityWelcomePageWidget;

class CommunityWelcomePage : public Utils::IWelcomePage
{
    Q_OBJECT
public:
    CommunityWelcomePage();

    QWidget *page();
    QString title() const { return tr("Community"); }
    int priority() const { return 30; }

private:
    CommunityWelcomePageWidget *m_page;
};

} // namespace Internal
} // namespace Welcome

#endif // COMMUNITYWELCOMEPAGE_H
