// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodestylepreferencesfactory.h"

#include "cppcodestylepreferences.h"
#include "cppcodestylesettingspage.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppqtstyleindenter.h"

#include <QLayout>

namespace CppEditor {

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


CppCodeStylePreferencesFactory::CppCodeStylePreferencesFactory() = default;

Utils::Id CppCodeStylePreferencesFactory::languageId()
{
    return Constants::CPP_SETTINGS_ID;
}

QString CppCodeStylePreferencesFactory::displayName()
{
    return Tr::tr(Constants::CPP_SETTINGS_NAME);
}

TextEditor::ICodeStylePreferences *CppCodeStylePreferencesFactory::createCodeStyle() const
{
    return new CppCodeStylePreferences();
}

TextEditor::CodeStyleEditorWidget *CppCodeStylePreferencesFactory::createEditor(
    TextEditor::ICodeStylePreferences *preferences,
    ProjectExplorer::Project *project,
    QWidget *parent) const
{
    auto cppPreferences = qobject_cast<CppCodeStylePreferences *>(preferences);
    if (!cppPreferences)
        return nullptr;
    auto widget = new Internal::CppCodeStylePreferencesWidget(parent);

    widget->layout()->setContentsMargins(0, 0, 0, 0);
    widget->setCodeStyle(cppPreferences);

    const auto tab = additionalTab(preferences, project, parent);
    widget->addTab(tab.first, tab.second);

    return widget;
}

TextEditor::Indenter *CppCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return new Internal::CppQtStyleIndenter(doc);
}

QString CppCodeStylePreferencesFactory::snippetProviderGroupId() const
{
    return QString(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID);
}

QString CppCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(defaultPreviewText);
}

std::pair<CppCodeStyleWidget *, QString> CppCodeStylePreferencesFactory::additionalTab(
    TextEditor::ICodeStylePreferences *codeStyle,
    ProjectExplorer::Project *project,
    QWidget *parent) const
{
    Q_UNUSED(codeStyle)
    Q_UNUSED(parent)
    Q_UNUSED(project)
    return {nullptr, ""};
}

} // namespace CppEditor
