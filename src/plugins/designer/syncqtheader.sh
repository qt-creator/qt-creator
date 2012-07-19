#!/bin/sh

#  *************************************************************************
#
#  This file is part of Qt Creator
#
#  Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
#
#  Contact: http://www.qt-project.org/
#
#  GNU Lesser General Public License Usage
#
#  This file may be used under the terms of the GNU Lesser General Public
#  License version 2.1 as published by the Free Software Foundation and
#  appearing in the file LICENSE.LGPL included in the packaging of this file.
#  Please review the following information to ensure the GNU Lesser General
#  Public License version 2.1 requirements will be met:
#  http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
#
#  In addition, as a special exception, Nokia gives you certain additional
#  rights. These rights are described in the Nokia Qt LGPL Exception
#  version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
#
#  Other Usage
#
#  Alternatively, this file may be used in accordance with the terms and
#  conditions contained in a signed written agreement between you and Nokia.
#
#  If you are unsure which license is appropriate for your use, please
#  contact the sales department at http://qt.nokia.com/contact.
#
#  ************************************************************************

# Internal utility script that synchronizes the Qt Designer private headers
# used by the Qt Designer plugin (located in the qt_private) directory
# with the Qt source tree pointed to by the environment variable QTDIR.

REQUIRED_HEADERS="pluginmanager_p.h iconloader_p.h qdesigner_formwindowmanager_p.h formwindowbase_p.h
abstractnewformwidget_p.h qtresourcemodel_p.h abstractoptionspage_p.h
shared_global_p.h abstractsettings_p.h qdesigner_integration_p.h qsimpleresource_p.h shared_enums_p.h"

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
  head -n 28 formwindowfile.h > $TARGET || exit 1
  tail -n +41 $QTHDR >> $TARGET || exit 1
}

for H in $REQUIRED_HEADERS
do
  syncHeader $H
done
