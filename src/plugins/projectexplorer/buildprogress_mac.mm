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

#include "buildprogress_mac.h"

#include <AppKit/NSDockTile.h>
#include <AppKit/NSApplication.h>
#include <Foundation/NSString.h>


void ProjectExplorer::Internal::qtcShowDockTileBadgeLabel(const QString &text)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    const char *utf8String = text.toUtf8().constData();
    NSString *cocoaString = [[NSString alloc] initWithUTF8String:utf8String];
    [[NSApp dockTile] setBadgeLabel:cocoaString];
    [cocoaString release];
#else
    Q_UNUSED(text)
#endif
}
