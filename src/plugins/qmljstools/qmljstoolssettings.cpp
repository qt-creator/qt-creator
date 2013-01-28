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

static const char *idKey = "QmlJSGlobal";

using namespace QmlJSTools;
using TextEditor::TabSettings;

namespace QmlJSTools {
namespace Internal {

class QmlJSToolsSettingsPrivate
{
public:
    TextEditor::SimpleCodeStylePreferences *m_globalCodeStyle;
};

} // namespace Internal
} // namespace QmlJSTools

QmlJSToolsSettings *QmlJSToolsSettings::m_instance = 0;

QmlJSToolsSettings::QmlJSToolsSettings(QObject *parent)
    : QObject(parent)
    , d(new Internal::QmlJSToolsSettingsPrivate)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();

    // code style factory
    TextEditor::ICodeStylePreferencesFactory *factory = new QmlJSTools::QmlJSCodeStylePreferencesFactory();
    textEditorSettings->registerCodeStyleFactory(factory);

    // code style pool
    TextEditor::CodeStylePool *pool = new TextEditor::CodeStylePool(factory, this);
    textEditorSettings->registerCodeStylePool(Constants::QML_JS_SETTINGS_ID, pool);

    // global code style settings
    d->m_globalCodeStyle = new TextEditor::SimpleCodeStylePreferences(this);
    d->m_globalCodeStyle->setDelegatingPool(pool);
    d->m_globalCodeStyle->setDisplayName(tr("Global", "Settings"));
    d->m_globalCodeStyle->setId(idKey);
    pool->addCodeStyle(d->m_globalCodeStyle);
    textEditorSettings->registerCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID, d->m_globalCodeStyle);

    // built-in settings
    // Qt style
    TextEditor::SimpleCodeStylePreferences *qtCodeStyle = new TextEditor::SimpleCodeStylePreferences();
    qtCodeStyle->setId(QLatin1String("qt"));
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
    d->m_globalCodeStyle->setCurrentDelegate(qtCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    QSettings *s = Core::ICore::settings();
    d->m_globalCodeStyle->fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);

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
                legacyTabSettings = textEditorSettings->codeStyle()->currentTabSettings();
            }

            // create custom code style out of old settings
            TextEditor::ICodeStylePreferences *oldCreator = pool->createCodeStyle(
                     QLatin1String("legacy"), legacyTabSettings,
                     QVariant(), tr("Old Creator"));

            // change the current delegate and save
            d->m_globalCodeStyle->setCurrentDelegate(oldCreator);
            d->m_globalCodeStyle->toSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
        }
        // mark old settings as transformed
        s->setValue(QLatin1String("QmlJSTabPreferences/LegacyTransformed"), true);
        // legacy handling stop
    }

    // mimetypes to be handled
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::QML_MIMETYPE),
                Constants::QML_JS_SETTINGS_ID);
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::JS_MIMETYPE),
                Constants::QML_JS_SETTINGS_ID);
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::JSON_MIMETYPE),
                Constants::QML_JS_SETTINGS_ID);
}

QmlJSToolsSettings::~QmlJSToolsSettings()
{
    delete d;

    m_instance = 0;
}

QmlJSToolsSettings *QmlJSToolsSettings::instance()
{
    return m_instance;
}

TextEditor::SimpleCodeStylePreferences *QmlJSToolsSettings::qmlJSCodeStyle() const
{
    return d->m_globalCodeStyle;
}


