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

#include "cppcodestylepreferencesfactory.h"

#include "cppcodestylesettingspage.h"
#include "cppcodestylepreferences.h"
#include "cpptoolsconstants.h"
#include "cppqtstyleindenter.h"

#include <cppeditor/cppeditorconstants.h>
#include <texteditor/snippets/isnippetprovider.h>

#include <extensionsystem/pluginmanager.h>

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

Core::Id CppCodeStylePreferencesFactory::languageId()
{
    return Constants::CPP_SETTINGS_ID;
}

QString CppCodeStylePreferencesFactory::displayName()
{
    return QString::fromUtf8(Constants::CPP_SETTINGS_NAME);
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
    return ExtensionSystem::PluginManager::getObject<TextEditor::ISnippetProvider>(
        [](TextEditor::ISnippetProvider *provider) {
            return provider->groupId() == QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
        });
}

QString CppCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(defaultPreviewText);
}

