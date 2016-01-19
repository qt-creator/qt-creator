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

#ifndef DESIGNERSETTINGS_H
#define DESIGNERSETTINGS_H

#include <QtGlobal>
#include <QHash>
#include <QVariant>
#include <QByteArray>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace DesignerSettingsKey {
const char ITEMSPACING[] = "ItemSpacing";
const char CONTAINERPADDING[] = "ContainerPadding";
const char CANVASWIDTH[] = "CanvasWidth";
const char CANVASHEIGHT[] = "CanvasHeight";
const char WARNING_FOR_FEATURES_IN_DESIGNER[] = "WarnAboutQtQuickFeaturesInDesigner";
const char WARNING_FOR_DESIGNER_FEATURES_IN_EDITOR[] = "WarnAboutQtQuickDesignerFeaturesInCodeEditor";
const char SHOW_DEBUGVIEW[] = "ShowQtQuickDesignerDebugView";
const char ENABLE_DEBUGVIEW[] = "EnableQtQuickDesignerDebugView";
const char ALWAYS_SAFE_IN_CRUMBLEBAR[] = "AlwaysSafeInCrumbleBar";
const char USE_ONLY_FALLBACK_PUPPET[] = "UseOnlyFallbackPuppet";
const char PUPPET_TOPLEVEL_BUILD_DIRECTORY[] = "PuppetToplevelBuildDirectory";
const char PUPPET_FALLBACK_DIRECTORY[] = "PuppetFallbackDirectory";
const char CONTROLS_STYLE[] = "ControlsStyle";
const char USE_QSTR_FUNCTION[] = "UseQsTrFunction";
const char SHOW_PROPERTYEDITOR_WARNINGS[] = "ShowPropertyEditorWarnings";
const char ENABLE_MODEL_EXCEPTION_OUTPUT[] = "WarnException";
const char PUPPET_KILL_TIMEOUT[] = "PuppetKillTimeout";
const char DEBUG_PUPPET[] = "DebugPuppet";
const char FORWARD_PUPPET_OUTPUT[] = "ForwardPuppetOutput";

}

class DesignerSettings : public QHash<QByteArray, QVariant>
{
public:
    DesignerSettings();

    void fromSettings(QSettings *);
    void toSettings(QSettings *) const;
private:
    void restoreValue(QSettings *settings, const QByteArray &key,
        const QVariant &defaultValue = QVariant());
    void storeValue(QSettings *settings, const QByteArray &key, const QVariant &value) const;
};

} // namespace QmlDesigner

#endif // DESIGNERSETTINGS_H
