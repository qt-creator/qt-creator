// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingspage.h"

#include "designersettings.h"
#include "qmldesignertr.h"

#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

namespace QmlDesigner::Internal {

SettingsPage::SettingsPage()
{
    setId("B.QmlDesigner");
    setDisplayName(Tr::tr("Qt Quick Designer"));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setSettingsProvider([] { return &designerSettings(); });
}

} // QmlDesigner::Internal
