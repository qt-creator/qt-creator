/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljstoolssettings.h"
#include "qmljstoolsconstants.h"
#include "qmljscodestylepreferencesfactory.h"

#include <texteditor/texteditorsettings.h>
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/codestylepool.h>

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QSettings>

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
    if (const QSettings *s = Core::ICore::instance()->settings()) {
        d->m_globalCodeStyle->fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
    }

    // mimetypes to be handled
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::QML_MIMETYPE),
                Constants::QML_JS_SETTINGS_ID);
    textEditorSettings->registerMimeTypeForLanguageId(
                QLatin1String(Constants::JS_MIMETYPE),
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


