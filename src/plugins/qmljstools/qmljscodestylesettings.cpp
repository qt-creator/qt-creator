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

#include "qmljscodestylesettings.h"

#include "qmljscodestylepreferences.h"
#include "qmljstoolssettings.h"

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/tabsettings.h>

#include <cplusplus/Overview.h>

#include <utils/qtcassert.h>
#include <utils/settingsutils.h>

static const char lineLengthKey[] = "LineLength";

namespace QmlJSTools {

// ------------------ QmlJSCodeStyleSettingsWidget

QmlJSCodeStyleSettings::QmlJSCodeStyleSettings() = default;

QVariantMap QmlJSCodeStyleSettings::toMap() const
{
    return {
        {lineLengthKey, lineLength}
    };
}

void QmlJSCodeStyleSettings::fromMap(const QVariantMap &map)
{
    lineLength = map.value(lineLengthKey, lineLength).toInt();
}

bool QmlJSCodeStyleSettings::equals(const QmlJSCodeStyleSettings &rhs) const
{
    return lineLength == rhs.lineLength;
}

QmlJSCodeStyleSettings QmlJSCodeStyleSettings::currentGlobalCodeStyle()
{
    QmlJSCodeStylePreferences *QmlJSCodeStylePreferences = QmlJSToolsSettings::globalCodeStyle();
    QTC_ASSERT(QmlJSCodeStylePreferences, return QmlJSCodeStyleSettings());

    return QmlJSCodeStylePreferences->currentCodeStyleSettings();
}

TextEditor::TabSettings QmlJSCodeStyleSettings::currentGlobalTabSettings()
{
    QmlJSCodeStylePreferences *QmlJSCodeStylePreferences = QmlJSToolsSettings::globalCodeStyle();
    QTC_ASSERT(QmlJSCodeStylePreferences, return TextEditor::TabSettings());

    return QmlJSCodeStylePreferences->currentTabSettings();
}

} // namespace QmlJSTools
