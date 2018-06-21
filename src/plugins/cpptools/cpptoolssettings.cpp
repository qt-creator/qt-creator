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

#include "cpptoolssettings.h"

#include "cpptoolsconstants.h"
#include "cppcodestylepreferences.h"
#include "cppcodestylepreferencesfactory.h"

#include <coreplugin/icore.h>
#include <texteditor/commentssettings.h>
#include <texteditor/completionsettingspage.h>
#include <texteditor/codestylepool.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/settingsutils.h>

#include <QSettings>

static const char idKey[] = "CppGlobal";

using namespace Core;
using namespace CppTools;
using namespace CppTools::Internal;
using namespace TextEditor;

namespace CppTools {
namespace Internal {

class CppToolsSettingsPrivate
{
public:
    CppToolsSettingsPrivate()
        : m_globalCodeStyle(0)
    {}

    CommentsSettings m_commentsSettings;
    CppCodeStylePreferences *m_globalCodeStyle;
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

    d->m_commentsSettings = TextEditorSettings::commentsSettings();
    connect(TextEditorSettings::instance(), &TextEditorSettings::commentsSettingsChanged,
            this, &CppToolsSettings::setCommentsSettings);

    // code style factory
    ICodeStylePreferencesFactory *factory = new CppCodeStylePreferencesFactory();
    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    CodeStylePool *pool = new CodeStylePool(factory, this);
    TextEditorSettings::registerCodeStylePool(Constants::CPP_SETTINGS_ID, pool);

    // global code style settings
    d->m_globalCodeStyle = new CppCodeStylePreferences(this);
    d->m_globalCodeStyle->setDelegatingPool(pool);
    d->m_globalCodeStyle->setDisplayName(tr("Global", "Settings"));
    d->m_globalCodeStyle->setId(idKey);
    pool->addCodeStyle(d->m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(CppTools::Constants::CPP_SETTINGS_ID, d->m_globalCodeStyle);

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

    // GNU style
    CppCodeStylePreferences *gnuCodeStyle = new CppCodeStylePreferences();
    gnuCodeStyle->setId("gnu");
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

    QSettings *s = ICore::settings();
    // load global settings (after built-in settings are added to the pool)
    d->m_globalCodeStyle->fromSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);

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
                legacyTabSettings = TextEditorSettings::codeStyle()->currentTabSettings();
            }

            // create custom code style out of old settings
            QVariant v;
            v.setValue(legacyCodeStyleSettings);
            ICodeStylePreferences *oldCreator = pool->createCodeStyle(
                     "legacy", legacyTabSettings, v, tr("Old Creator"));

            // change the current delegate and save
            d->m_globalCodeStyle->setCurrentDelegate(oldCreator);
            d->m_globalCodeStyle->toSettings(QLatin1String(CppTools::Constants::CPP_SETTINGS_ID), s);
        }
        // mark old settings as transformed
        s->setValue(QLatin1String("CppCodeStyleSettings/LegacyTransformed"), true);
        // legacy handling stop
    }


    // mimetypes to be handled
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::C_SOURCE_MIMETYPE, Constants::CPP_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::C_HEADER_MIMETYPE, Constants::CPP_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::CPP_SOURCE_MIMETYPE, Constants::CPP_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(Constants::CPP_HEADER_MIMETYPE, Constants::CPP_SETTINGS_ID);
}

CppToolsSettings::~CppToolsSettings()
{
    TextEditorSettings::unregisterCodeStyle(Constants::CPP_SETTINGS_ID);
    TextEditorSettings::unregisterCodeStylePool(Constants::CPP_SETTINGS_ID);
    TextEditorSettings::unregisterCodeStyleFactory(Constants::CPP_SETTINGS_ID);

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
    return d->m_commentsSettings;
}

void CppToolsSettings::setCommentsSettings(const CommentsSettings &commentsSettings)
{
    d->m_commentsSettings = commentsSettings;
}

static QString sortEditorDocumentOutlineKey()
{
    return QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP)
         + QLatin1Char('/')
         + QLatin1String(CppTools::Constants::CPPTOOLS_SORT_EDITOR_DOCUMENT_OUTLINE);
}

bool CppToolsSettings::sortedEditorDocumentOutline() const
{
    return ICore::settings()->value(sortEditorDocumentOutlineKey(), true).toBool();
}

void CppToolsSettings::setSortedEditorDocumentOutline(bool sorted)
{
    ICore::settings()->setValue(sortEditorDocumentOutlineKey(), sorted);
    emit editorDocumentOutlineSortingChanged(sorted);
}

static QString showHeaderErrorInfoBarKey()
{
    return QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP)
         + QLatin1Char('/')
         + QLatin1String(CppTools::Constants::CPPTOOLS_SHOW_INFO_BAR_FOR_HEADER_ERRORS);
}

bool CppToolsSettings::showHeaderErrorInfoBar() const
{
    return ICore::settings()->value(showHeaderErrorInfoBarKey(), true).toBool();
}

void CppToolsSettings::setShowHeaderErrorInfoBar(bool show)
{
    ICore::settings()->setValue(showHeaderErrorInfoBarKey(), show);
    emit showHeaderErrorInfoBarChanged(show);
}

static QString showNoProjectInfoBarKey()
{
    return QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP)
         + QLatin1Char('/')
         + QLatin1String(CppTools::Constants::CPPTOOLS_SHOW_INFO_BAR_FOR_FOR_NO_PROJECT);
}

bool CppToolsSettings::showNoProjectInfoBar() const
{
    return ICore::settings()->value(showNoProjectInfoBarKey(), true).toBool();
}

void CppToolsSettings::setShowNoProjectInfoBar(bool show)
{
    ICore::settings()->setValue(showNoProjectInfoBarKey(), show);
    emit showNoProjectInfoBarChanged(show);
}
