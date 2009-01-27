#!/bin/sh

#  **************************************************************************
#
#  This file is part of Qt Creator
#
#  Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
#
#  Contact:  Qt Software Information (qt-info@nokia.com)
#
#
#  Non-Open Source Usage
#
#  Licensees may use this file in accordance with the Qt Beta Version
#  License Agreement, Agreement version 2.2 provided with the Software or,
#  alternatively, in accordance with the terms contained in a written
#  agreement between you and Nokia.
#
#  GNU General Public License Usage
#
#  Alternatively, this file may be used under the terms of the GNU General
#  Public License versions 2.0 or 3.0 as published by the Free Software
#  Foundation and appearing in the file LICENSE.GPL included in the packaging
#  of this file.  Please review the following information to ensure GNU
#  General Public Licensing requirements will be met:
#
#  http://www.fsf.org/licensing/licenses/info/GPLv2.html and
#  http://www.gnu.org/copyleft/gpl.html.
#
#  In addition, as a special exception, Nokia gives you certain additional
#  rights. These rights are described in the Nokia Qt GPL Exception
#  version 1.3, included in the file GPL_EXCEPTION.txt in this package.
#
# ***************************************************************************/

# Internal utility script that synchronizes the Qt Designer private headers
# used by the Qt Designer plugin (located in the qt_private) directory
# with the Qt source tree pointed to by the environment variable QTDIR.

REQUIRED_HEADERS="pluginmanager_p.h iconloader_p.h qdesigner_formwindowmanager_p.h formwindowbase_p.h
abstractnewformwidget_p.h qtresourcemodel_p.h abstractoptionspage_p.h
shared_global_p.h abstractsettings_p.h qdesigner_integration_p.h"

echo Using $QTDIR

syncHeader()
{
  HDR=$1
  # Locate the Designer header: look in lib/shared or SDK
  QTHDR=$QTDIR/tools/designer/src/lib/shared/$HDR
  if [ ! -f $QTHDR ]
  then
    QTHDR=$QTDIR/tools/designer/src/lib/sdk/$HDR
  fi
  echo Syncing $QTHDR

  [ -f $QTHDR ] || { echo "$HDR does not exist" ; exit 1 ; }

  TARGET=qt_private/$HDR

  # Exchange license header
  head -n 32 formwindowfile.h > $TARGET || exit 1
  tail -n +11 $QTHDR >> $TARGET || exit 1
}

for H in $REQUIRED_HEADERS
do
  syncHeader $H
done
