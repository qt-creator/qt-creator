/**************************************************************************
**
** Copyright (c) 2013 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CLEARCASE_CONSTANTS_H
#define CLEARCASE_CONSTANTS_H

#include <QtGlobal>

namespace ClearCase {
namespace Constants {

const char VCS_ID_CLEARCASE[] = "E.ClearCase";
const char CLEARCASE_ROOT_FILE[] = "view.dat";
const char CLEARCASE_SUBMIT_MIMETYPE[] = "application/vnd.audc.text.clearcase.submit";
const char CLEARCASECHECKINEDITOR[] = "ClearCase Check In Editor";
const char CLEARCASECHECKINEDITOR_ID[] = "ClearCase Check In Editor";
const char CLEARCASECHECKINEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "ClearCase Check In Editor");
const char CHECKIN_SELECTED[] = "ClearCase.CheckInSelected";
const char DIFF_SELECTED[] = "ClearCase.DiffSelected";
const char TASK_INDEX[] = "ClearCase.Task.Index";
const char KEEP_ACTIVITY[] = "__KEEP__";
enum { debug = 0 };

} // namespace Constants
} // namespace ClearCase

#endif // CLEARCASE_CONSTANTS_H
