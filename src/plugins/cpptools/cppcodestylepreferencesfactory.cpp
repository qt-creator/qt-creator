/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppcodestylepreferencesfactory.h"
#include "cppcodestylesettings.h"
#include "cppcodestylesettingspage.h"
#include "cppcodestylepreferences.h"
#include "cpptoolsconstants.h"
#include "cppqtstyleindenter.h"
#include <extensionsystem/pluginmanager.h>
#include <texteditor/tabsettings.h>
#include <texteditor/snippets/isnippetprovider.h>
#include <cppeditor/cppeditorconstants.h>
#include <QLayout>

using namespace CppTools;

static const char *defaultPreviewText =
    "#include <math.h>\n"
    "\n"
    "class Complex\n"
    "    {\n"
    "public:\n"
    "    Complex(double re, double im)\n"
    "        : _re(re), _im(im)\n"
    "        {}\n"
    "    double modulus() const\n"
    "        {\n"
    "        return sqrt(_re * _re + _im * _im);\n"
    "        }\n"
    "private:\n"
    "    double _re;\n"
    "    double _im;\n"
    "    };\n"
    "\n"
    "void bar(int i)\n"
    "    {\n"
    "    static int counter = 0;\n"
    "    counter += i;\n"
    "    }\n"
    "\n"
    "namespace Foo\n"
    "    {\n"
    "    namespace Bar\n"
    "        {\n"
    "        void foo(int a, int b)\n"
    "            {\n"
    "            for (int i = 0; i < a; i++)\n"
    "                {\n"
    "                if (i < b)\n"
    "                    bar(i);\n"
    "                else\n"
    "                    {\n"
    "                    bar(i);\n"
    "                    bar(b);\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        } // namespace Bar\n"
    "    } // namespace Foo\n"
    ;


CppCodeStylePreferencesFactory::CppCodeStylePreferencesFactory()
{
}

QString CppCodeStylePreferencesFactory::languageId()
{
    return Constants::CPP_SETTINGS_ID;
}

QString CppCodeStylePreferencesFactory::displayName()
{
    return Constants::CPP_SETTINGS_NAME;
}

TextEditor::ICodeStylePreferences *CppCodeStylePreferencesFactory::createCodeStyle() const
{
    return new CppCodeStylePreferences();
}

QWidget *CppCodeStylePreferencesFactory::createEditor(TextEditor::ICodeStylePreferences *preferences,
                                                           QWidget *parent) const
{
    CppCodeStylePreferences *cppPreferences = qobject_cast<CppCodeStylePreferences *>(preferences);
    if (!cppPreferences)
        return 0;
    Internal::CppCodeStylePreferencesWidget *widget = new Internal::CppCodeStylePreferencesWidget(parent);
    widget->layout()->setMargin(0);
    widget->setCodeStyle(cppPreferences);
    return widget;
}

TextEditor::Indenter *CppCodeStylePreferencesFactory::createIndenter() const
{
    return new CppQtStyleIndenter();
}

TextEditor::ISnippetProvider *CppCodeStylePreferencesFactory::snippetProvider() const
{
    const QList<TextEditor::ISnippetProvider *> &providers =
        ExtensionSystem::PluginManager::getObjects<TextEditor::ISnippetProvider>();
    foreach (TextEditor::ISnippetProvider *provider, providers)
        if (provider->groupId() == QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID))
            return provider;
    return 0;
}

QString CppCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(defaultPreviewText);
}

