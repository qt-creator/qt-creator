// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodestylepreferencesfactory.h"

#include "cppcodestylesettings.h"
#include "cppcodestylesettingspage.h"
#include "cppcodestylesnippets.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppqtstyleindenter.h"

#include <projectexplorer/project.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>

#include <QLayout>
#include <QString>
#include <QTextDocument>
#include <QWidget>

namespace CppEditor {

class CppCodeStyleEditor final : public TextEditor::CodeStyleEditor
{
public:
    static CppCodeStyleEditor *create(
        const TextEditor::ICodeStylePreferencesFactory *factory,
        ProjectExplorer::Project *project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr);

private:
    CppCodeStyleEditor(QWidget *parent = nullptr);

    CodeStyleEditorWidget *createEditorWidget(
        const void * /*project*/,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const override;
    QString previewText() const override;
    QString snippetProviderGroupId() const override;
};

CppCodeStyleEditor *CppCodeStyleEditor::create(
    const TextEditor::ICodeStylePreferencesFactory *factory,
    ProjectExplorer::Project *project,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent)
{
    auto editor = new CppCodeStyleEditor{parent};
    editor->init(factory, wrapProject(project), codeStyle);
    return editor;
}

CppCodeStyleEditor::CppCodeStyleEditor(QWidget *parent)
    : CodeStyleEditor{parent}
{}

TextEditor::CodeStyleEditorWidget *CppCodeStyleEditor::createEditorWidget(
    const void * /*project*/,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    auto cppPreferences = dynamic_cast<CppCodeStylePreferences *>(codeStyle);
    if (cppPreferences == nullptr)
        return nullptr;

    auto widget = new CppCodeStylePreferencesWidget(parent);

    widget->layout()->setContentsMargins(0, 0, 0, 0);
    widget->setCodeStyle(cppPreferences);

    return widget;
}

QString CppCodeStyleEditor::previewText() const
{
    return QString::fromLatin1(Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]);
}

QString CppCodeStyleEditor::snippetProviderGroupId() const
{
    return CppEditor::Constants::CPP_SNIPPETS_GROUP_ID;
}

TextEditor::CodeStyleEditorWidget *CppCodeStylePreferencesFactory::createCodeStyleEditor(
    const TextEditor::ProjectWrapper &project,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    return CppCodeStyleEditor::create(
        this, ProjectExplorer::unwrapProject(project), codeStyle, parent);
}

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
    return new CppCodeStylePreferences;
}

TextEditor::Indenter *CppCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return createCppQtStyleIndenter(doc);
}

} // namespace CppEditor
