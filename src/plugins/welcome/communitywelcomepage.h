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
    QString title() const { return tr("News && Support"); }
    int priority() const { return 30; }

private:
    CommunityWelcomePageWidget *m_page;
};

} // namespace Internal
} // namespace Welcome

#endif // COMMUNITYWELCOMEPAGE_H
