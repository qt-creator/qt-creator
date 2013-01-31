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

#include "cpptoolssettings.h"
#include "cpptoolsconstants.h"
#include "cppcodestylepreferences.h"
#include "cppcodestylepreferencesfactory.h"
#include "commentssettings.h"
#include "completionsettingspage.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestylepool.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/settingsutils.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <QSettings>

static const char idKey[] = "CppGlobal";

using namespace CppTools;
using namespace CppTools::Internal;
using TextEditor::TabSettings;

namespace CppTools {
namespace Internal {

class CppToolsSettingsPrivate
{
public:
    CppToolsSettingsPrivate()
        : m_globalCodeStyle(0)
        , m_completionSettingsPage(0)
    {}

    CppCodeStylePreferences *m_globalCodeStyle;
    CompletionSettingsPage *m_completionSettingsPage;
};

} // namespace Internal
} // namespace CppTools

CppToolsSettings *CppToolsSettings::m_instance = 0;

CppToolsSettings::CppToolsSettings(QObject *parent)
    : QObject(parent)
    , d(new Internal::CppToolsSettingsPrivate)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    qRegisterMetaType<CppTools::CppCodeStyleSettings>("CppTools::CppCodeStyleSettings");

    d->m_completionSettingsPage = new CompletionSettingsPage(this);
    ExtensionSystem::PluginManager::addObject(d->m_completionSettingsPage);

    connect(d->m_completionSettingsPage,
            SIGNAL(commentsSettingsChanged(CppTools::CommentsSettings)),
            this,
            SIGNAL(commentsSettingsChanged(CppTools::CommentsSettings)));

    TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();

    // code style factory
    TextEditor::ICodeStylePreferencesFactory *factory = new CppTools::CppCodeStylePreferencesFactory();
    textEditorSettings->registerCodeStyleFactory(factory);

    // code style pool
    TextEditor::CodeStylePool *pool = new TextEditor::CodeStylePool(factory, this);
    textEditorSettings->registerCodeStylePool(Constants::CPP_SETTINGS_ID, pool);

    // global code style settings
    d->m_globalCodeStyle = new CppCodeStylePreferences(this);
    d->m_globalCodeStyle->setDelegatingPool(pool);
    d->m_globalCodeStyle->setDisplayName(tr("Global", "Settings"));
    d->m_globalCodeStyle->setId(QLatin1String(idKey));
    pool->addCodeStyle(d->m_globalCodeStyle);
    textEditorSettings->registerCodeStyle(CppTools::Constants::CPP_SETTINGS_ID, d->m_globalCodeStyle);

    /*
    For every language we have exactly 1 pool. The pool contains:
    1) All built-in code styles (Qt/GNU)
    2) All custom code styles (which will be added dynamically)
    3) A global code style

    If the code style gets a pool (setCodeStylePool()) it means it can behave
    like a proxy to one of the code styles from that pool
    (ICodeStylePreferences::setCurrentDelegate()).
    That's why the global code style gets a pool (it can point to any code style
    from the pool), while built-in and custom code styles don't get a pool
    (they can't point to any other code style).

    The instance of the language pool is shared. The same instance of the pool
    is used for all project code style settings and for global one.
    Project code style can point to one of built-in or custom code styles
    or to the global one as well. That's why the global code style is added
    to the pool. The proxy chain can look like:
    ProjectCodeStyle -> GlobalCodeStyle -> BuildInCodeStyle (e.g. Qt).

    With the global pool there is an exception - it gets a pool
    in which it exists itself. The case in which a code style point to itself
    is disallowed and is handled in ICodeStylePreferences::setCurrentDelegate().
    */

    // built-in settings
    // Qt style
    CppCodeStylePreferences *qtCodeStyle = new CppCodeStylePreferences();
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

    // GNU style
    CppCodeStylePreferences *gnuCodeStyle = new CppCodeStylePreferences();
    gnuCodeStyle->setId(QLatin1String("gnu"));
    gnuCodeStyle->setDisplayName(tr("GNU"));
    gnuCodeStyle->setReadOnly(true);
    TabSettings gnuTabSettings;
    gnuTabSettings.m_tabPolicy = TabSettings::MixedTabPolicy;
    gnuTabSettings.m_tabSize = 8;
    gnuTabSettings.m_indentSize = 2;
    gnuTabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;
    gnuCodeStyle->setTabSettings(gnuTabSettings);
    CppCodeStyleSettings gnuCodeStyleSettings;
    gnuCodeStyleSettings.indentNamespaceBody = true;
    gnuCodeStyleSettings.indentBlockBraces = true;
    gnuCodeStyleSettings.indentSwitchLabels = true;
    gnuCodeStyleSettings.indentBlocksRelativeToSwitchLabels = true;
    gnuCodeStyle->setCodeStyleSettings(gnuCodeStyleSettings);
    pool->addCodeStyle(gnuCodeStyle);

    // default delegate for global preferences
    d->m_globalCodeStyle->setCurrentDelegate(qtCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    QSettings *s = Core::ICore::settings();
    d->m_globalCodeStyle->fromSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), Core::ICore::settings());

    // legacy handling start (Qt Creator Version < 2.4)
    const bool legacyTransformed =
                s->value(QLatin1String("CppCodeStyleSettings/LegacyTransformed"), false).toBool();

    if (!legacyTransformed) {
        // creator 2.4 didn't mark yet the transformation (first run of creator 2.4)

        // we need to transform the settings only if at least one from
        // below settings was already written - otherwise we use
        // defaults like it would be the first run of creator 2.4 without stored settings
        const QStringList groups = s->childGroups();
        const bool needTransform = groups.contains(QLatin1String("textTabPreferences")) ||
                                   groups.contains(QLatin1String("CppTabPreferences")) ||
                                   groups.contains(QLatin1String("CppCodeStyleSettings"));
        if (needTransform) {
            CppCodeStyleSettings legacyCodeStyleSettings;
            if (groups.contains(QLatin1String("CppCodeStyleSettings"))) {
                Utils::fromSettings(QLatin1String("CppCodeStyleSettings"),
                                    QString(), s, &legacyCodeStyleSettings);
            }

            const QString currentFallback = s->value(QLatin1String("CppTabPreferences/CurrentFallback")).toString();
            TabSettings legacyTabSettings;
            if (currentFallback == QLatin1String("CppGlobal")) {
                // no delegate, global overwritten
                Utils::fromSettings(QLatin1String("CppTabPreferences"),
                                    QString(), s, &legacyTabSettings);
            } else {
                // delegating to global
                legacyTabSettings = textEditorSettings->codeStyle()->currentTabSettings();
            }

            // create custom code style out of old settings
            QVariant v;
            v.setValue(legacyCodeStyleSettings);
            TextEditor::ICodeStylePreferences *oldCreator = pool->createCodeStyle(
                     QLatin1String("legacy"), legacyTabSettings,
                     v, tr("Old Creator"));

            // change the current delegate and save
            d->m_globalCodeStyle->setCurrentDelegate(oldCreator);
            d->m_globalCodeStyle->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
        // mark old settings as transformed
        s->setValue(QLatin1String("CppCodeStyleSettings/LegacyTransformed"), true);
        // legacy handling stop
    }


    // mimetypes to be handled
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::C_SOURCE_MIMETYPE),
                Constants::CPP_SETTINGS_ID);
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::C_HEADER_MIMETYPE),
                Constants::CPP_SETTINGS_ID);
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::CPP_SOURCE_MIMETYPE),
                Constants::CPP_SETTINGS_ID);
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::CPP_HEADER_MIMETYPE),
                Constants::CPP_SETTINGS_ID);
}

CppToolsSettings::~CppToolsSettings()
{
    ExtensionSystem::PluginManager::removeObject(d->m_completionSettingsPage);

    delete d;

    m_instance = 0;
}

CppToolsSettings *CppToolsSettings::instance()
{
    return m_instance;
}

CppCodeStylePreferences *CppToolsSettings::cppCodeStyle() const
{
    return d->m_globalCodeStyle;
}

const CommentsSettings &CppToolsSettings::commentsSettings() const
{
    return d->m_completionSettingsPage->commentsSettings();
}
