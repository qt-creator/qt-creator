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

#ifndef ANALYZERICONS_H
#define ANALYZERICONS_H

#include <utils/icon.h>

namespace Debugger {
namespace Icons {

const Utils::Icon ANALYZER_CONTROL_START({
        {QLatin1String(":/core/images/run_small.png"), Utils::Theme::IconsRunToolBarColor},
        {QLatin1String(":/images/analyzer_overlay_small.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_ANALYZE_CLASSIC(
        QLatin1String(":/images/mode_analyze.png"));
const Utils::Icon MODE_ANALYZE_FLAT({
        {QLatin1String(":/images/mode_analyze_mask.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon MODE_ANALYZE_FLAT_ACTIVE({
        {QLatin1String(":/images/mode_analyze_mask.png"), Utils::Theme::IconsModeAnalyzeActiveColor}});

} // namespace Icons
} // namespace Debugger

#endif // ANALYZERICONS_H
