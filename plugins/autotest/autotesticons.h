/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef AUTOTESTICONS_H
#define AUTOTESTICONS_H

#include <utils/icon.h>

namespace Autotest {
namespace Icons {

const Utils::Icon EXPAND({
        {QLatin1String(":/images/expand.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon COLLAPSE({
        {QLatin1String(":/images/collapse.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon SORT_ALPHABETICALLY({
        {QLatin1String(":/images/sort.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon SORT_NATURALLY({
        {QLatin1String(":/images/leafsort.png"), Utils::Theme::IconsBaseColor}});
const Utils::Icon RUN_SELECTED_OVERLAY({
        {QLatin1String(":/images/runselected_boxes.png"), Utils::Theme::BackgroundColorDark},
        {QLatin1String(":/images/runselected_tickmarks.png"), Utils::Theme::IconsBaseColor}});

} // namespace Icons
} // namespace Autotest

#endif // AUTOTESTICONS_H
