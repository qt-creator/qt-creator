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

#include "qmljstoolssettings.h"
#include "qmljstoolsconstants.h"
#include "qmljscodestylepreferencesfactory.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestylepool.h>

#include <utils/settingsutils.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QSettings>

using namespace TextEditor;

namespace QmlJSTools {

const char idKey[] = "QmlJSGlobal";

static SimpleCodeStylePreferences *m_globalCodeStyle = 0;

QmlJSToolsSettings::QmlJSToolsSettings(QObject *parent)
    : QObject(parent)
{
    QTC_ASSERT(!m_globalCodeStyle, return);

    // code style factory
    ICodeStylePreferencesFactory *factory = new QmlJSCodeStylePreferencesFactory();
    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    CodeStylePool *pool = new CodeStylePool(factory, this);
    TextEditorSettings::registerCodeStylePool(Constants::QML_JS_SETTINGS_ID, pool);

    // global code style settings
    m_globalCodeStyle = new SimpleCodeStylePreferences(this);
    m_globalCodeStyle->setDelegatingPool(pool);
    m_globalCodeStyle->setDisplayName(tr("Global", "Settings"));
    m_globalCodeStyle->setId(idKey);
    pool->addCodeStyle(m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID, m_globalCodeStyle);

    // built-in settings
    // Qt style
    SimpleCodeStylePreferences *qtCodeStyle = new SimpleCodeStylePreferences();
    qtCodeStyle->setId("qt");
    qtCodeStyle->setDisplayName(tr("Qt"));
    qtCodeStyle->setReadOnly(true);
    TabSettings qtTabSettings;
    qtTabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
    qtTabSettings.m_tabSize = 4;
    qtTabSettings.m_indentSize = 4;
    qtTabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;
    qtCodeStyle->setTabSettings(qtTabSettings);
    pool->addCodeStyle(qtCodeStyle);

    // default delegate for global preferences
    m_globalCodeStyle->setCurrentDelegate(qtCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    QSettings *s = Core::ICore::settings();
    m_globalCodeStyle->fromSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);

    // legacy handling start (Qt Creator Version < 2.4)
    const bool legacyTransformed =
                s->value(QLatin1String("QmlJSTabPreferences/LegacyTransformed"), false).toBool();

    if (!legacyTransformed) {
        // creator 2.4 didn't mark yet the transformation (first run of creator 2.4)

        // we need to transform the settings only if at least one from
        // below settings was already written - otherwise we use
        // defaults like it would be the first run of creator 2.4 without stored settings
        const QStringList groups = s->childGroups();
        const bool needTransform = groups.contains(QLatin1String("textTabPreferences")) ||
                                   groups.contains(QLatin1String("QmlJSTabPreferences"));

        if (needTransform) {
            const QString currentFallback = s->value(QLatin1String("QmlJSTabPreferences/CurrentFallback")).toString();
            TabSettings legacyTabSettings;
            if (currentFallback == QLatin1String("QmlJSGlobal")) {
                // no delegate, global overwritten
                Utils::fromSettings(QLatin1String("QmlJSTabPreferences"),
                                    QString(), s, &legacyTabSettings);
            } else {
                // delegating to global
                legacyTabSettings = TextEditorSettings::codeStyle()->currentTabSettings();
            }

            // create custom code style out of old settings
            ICodeStylePreferences *oldCreator = pool->createCodeStyle(
                     "legacy", legacyTabSettings, QVariant(), tr("Old Creator"));

            // change the current delegate and save
            m_globalCodeStyle->setCurrentDelegate(oldCreator);
            m_globalCodeStyle->toSettings(QLatin1String(QmlJSTools::Constants::QML_JS_SETTINGS_ID), s);
        }
        // mark old settings as transformed
        s->setValue(QLatin1String("QmlJSTabPreferences/LegacyTransformed"), true);
        // legacy handling stop
    }

    // mimetypes to be handled
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::QML_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::QMLUI_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::QBS_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::QMLPROJECT_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::QMLTYPES_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::JS_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::JSON_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
}

QmlJSToolsSettings::~QmlJSToolsSettings()
{
    TextEditorSettings::unregisterCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::unregisterCodeStylePool(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::unregisterCodeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID);

    delete m_globalCodeStyle;
    m_globalCodeStyle = 0;
}

SimpleCodeStylePreferences *QmlJSToolsSettings::globalCodeStyle()
{
    return m_globalCodeStyle;
}

} // namespace QmlJSTools
