/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QtGlobal>

namespace ClearCase {
namespace Constants {

const char VCS_ID_CLEARCASE[] = "E.ClearCase";
const char CLEARCASE_SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.clearcase.submit";
const char CLEARCASECHECKINEDITOR_ID[] = "ClearCase Check In Editor";
const char CLEARCASECHECKINEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("VCS", "ClearCase Check In Editor");
const char CHECKIN_SELECTED[] = "ClearCase.CheckInSelected";
const char DIFF_SELECTED[] = "ClearCase.DiffSelected";
const char TASK_INDEX[] = "ClearCase.Task.Index";
const char KEEP_ACTIVITY[] = "__KEEP__";
enum { debug = 0 };

} // namespace Constants
} // namespace ClearCase
