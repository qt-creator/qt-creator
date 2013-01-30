/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QBSPROJECTMANAGERCONSTANTS_H
#define QBSPROJECTMANAGERCONSTANTS_H

#include <QtGlobal>

namespace QbsProjectManager {
namespace Constants {

// Contexts
const char PROJECT_ID[] = "Qbs.QbsProject";

// MIME types:
const char MIME_TYPE[] = "application/vnd.qtproject.qbsproject";

// Progress reports:
const char QBS_EVALUATE[] = "Qbs.QbsEvaluate";

// Actions:
const char ACTION_REPARSE_QBS[] = "Qbs.Reparse";
const char ACTION_REPARSE_QBS_CONTEXT[] = "Qbs.ReparseCtx";
const char ACTION_BUILD_FILE_QBS_CONTEXT[] = "Qbs.BuildFileCtx";

// Ids:
const char QBS_BUILDSTEP_ID[] = "Qbs.BuildStep";
const char QBS_CLEANSTEP_ID[] = "Qbs.CleanStep";

// QBS strings:
static const char QBS_VARIANT_DEBUG[] = "debug";
static const char QBS_VARIANT_RELEASE[] = "release";

static const char QBS_CONFIG_VARIANT_KEY[] = "qbs.buildVariant";
static const char QBS_CONFIG_PROFILE_KEY[] = "qbs.profile";

} // namespace Constants
} // namespace QbsProjectManager

#endif // QBSPROJECTMANAGERCONSTANTS_H

