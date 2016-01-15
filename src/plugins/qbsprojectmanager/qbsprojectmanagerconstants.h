/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QBSPROJECTMANAGERCONSTANTS_H
#define QBSPROJECTMANAGERCONSTANTS_H

#include <QtGlobal>

namespace QbsProjectManager {
namespace Constants {

// Contexts
const char PROJECT_ID[] = "Qbs.QbsProject";

// MIME types:
const char MIME_TYPE[] = "application/x-qt.qbs+qml";

// Actions:
const char ACTION_REPARSE_QBS[] = "Qbs.Reparse";
const char ACTION_REPARSE_QBS_CONTEXT[] = "Qbs.ReparseCtx";
const char ACTION_BUILD_FILE_CONTEXT[] = "Qbs.BuildFileCtx";
const char ACTION_BUILD_FILE[] = "Qbs.BuildFile";
const char ACTION_BUILD_PRODUCT_CONTEXT[] = "Qbs.BuildProductCtx";
const char ACTION_BUILD_PRODUCT[] = "Qbs.BuildProduct";
const char ACTION_BUILD_SUBPROJECT_CONTEXT[] = "Qbs.BuildSubprojectCtx";
const char ACTION_BUILD_SUBPROJECT[] = "Qbs.BuildSubproduct";

// Ids:
const char QBS_BUILDSTEP_ID[] = "Qbs.BuildStep";
const char QBS_CLEANSTEP_ID[] = "Qbs.CleanStep";
const char QBS_INSTALLSTEP_ID[] = "Qbs.InstallStep";

// QBS strings:
static const char QBS_VARIANT_DEBUG[] = "debug";
static const char QBS_VARIANT_RELEASE[] = "release";

static const char QBS_CONFIG_VARIANT_KEY[] = "qbs.buildVariant";
static const char QBS_CONFIG_PROFILE_KEY[] = "qbs.profile";
static const char QBS_CONFIG_DECLARATIVE_DEBUG_KEY[] = "Qt.declarative.qmlDebugging";
static const char QBS_CONFIG_QUICK_DEBUG_KEY[] = "Qt.quick.qmlDebugging";

// Icons:
static const char QBS_GROUP_ICON[] = ":/qbsprojectmanager/images/groups.png";
static const char QBS_PRODUCT_OVERLAY_ICON[] = ":/qbsprojectmanager/images/productgear.png";

} // namespace Constants
} // namespace QbsProjectManager

#endif // QBSPROJECTMANAGERCONSTANTS_H

