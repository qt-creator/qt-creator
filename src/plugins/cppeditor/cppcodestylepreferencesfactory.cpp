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

using namespace TextEditor;

namespace CppEditor {

class CppCodeStyleEditor final : public CodeStyleEditor
{
public:
    static CppCodeStyleEditor *create(
        const ICodeStylePreferencesFactory *factory,
        ProjectExplorer::Project *project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent)
    {
        auto editor = new CppCodeStyleEditor{parent};
        editor->init(factory, wrapProject(project), codeStyle);
        return editor;
    }

private:
    CppCodeStyleEditor(QWidget *parent)
        : CodeStyleEditor{parent}
    {}

    CodeStyleEditorWidget *createEditorWidget(
        const void * /*project*/,
        ICodeStylePreferences *codeStyle,
        QWidget *parent) const final
    {
        auto cppPreferences = dynamic_cast<CppCodeStylePreferences *>(codeStyle);
        if (cppPreferences == nullptr)
            return nullptr;

        auto widget = new CppCodeStylePreferencesWidget(parent);

        widget->layout()->setContentsMargins(0, 0, 0, 0);
        widget->setCodeStyle(cppPreferences);

        return widget;
    }

    QString previewText() const final
    {
        return QString::fromLatin1(Constants::DEFAULT_CODE_STYLE_SNIPPETS[0]);
    }

    QString snippetProviderGroupId() const final
    {
        return CppEditor::Constants::CPP_SNIPPETS_GROUP_ID;
    }
};

// CppCodeStylePreferencesFactory

class CppCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    CppCodeStylePreferencesFactory() = default;

private:
    CodeStyleEditorWidget *createCodeStyleEditor(
            const ProjectWrapper &project,
            ICodeStylePreferences *codeStyle,
            QWidget *parent) const final
    {
        return CppCodeStyleEditor::create(
                    this, ProjectExplorer::unwrapProject(project), codeStyle, parent);
    }

    Utils::Id languageId() final
    {
        return Constants::CPP_SETTINGS_ID;
    }

    QString displayName() final
    {
        return Tr::tr(Constants::CPP_SETTINGS_NAME);
    }

    ICodeStylePreferences *createCodeStyle() const final
    {
        return new CppCodeStylePreferences;
    }

    Indenter *createIndenter(QTextDocument *doc) const final
    {
        return createCppQtStyleIndenter(doc);
    }
};

ICodeStylePreferencesFactory *createCppCodeStylePreferencesFactory()
{
    return new CppCodeStylePreferencesFactory;
}

} // namespace CppEditor
