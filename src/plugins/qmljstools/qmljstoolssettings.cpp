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

#include <texteditor/texteditorsettings.h>
#include <texteditor/tabpreferences.h>

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <QtCore/QSettings>

static const char *idKey = "QmlJSGlobal";

using namespace QmlJSTools;

namespace QmlJSTools {
namespace Internal {

class QmlJSToolsSettingsPrivate
{
public:
    TextEditor::TabPreferences *m_tabPreferences;
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

    if (const QSettings *s = Core::ICore::instance()->settings()) {
        TextEditor::TextEditorSettings *textEditorSettings = TextEditor::TextEditorSettings::instance();
        TextEditor::TabPreferences *tabPrefs = textEditorSettings->tabPreferences();
        d->m_tabPreferences
                = new TextEditor::TabPreferences(QList<TextEditor::IFallbackPreferences *>()
                                                 << tabPrefs, this);
        d->m_tabPreferences->setCurrentFallback(tabPrefs);
        d->m_tabPreferences->setFallbackEnabled(tabPrefs, false);
        d->m_tabPreferences->fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID, s);
        d->m_tabPreferences->setDisplayName(tr("Global Qt Quick", "Settings"));
        d->m_tabPreferences->setId(idKey);
        textEditorSettings->registerLanguageTabPreferences(QmlJSTools::Constants::QML_JS_SETTINGS_ID, d->m_tabPreferences);
    }
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

TextEditor::TabPreferences *QmlJSToolsSettings::tabPreferences() const
{
    return d->m_tabPreferences;
}


