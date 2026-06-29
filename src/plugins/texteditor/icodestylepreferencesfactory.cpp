// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icodestylepreferencesfactory.h"

#include <QMap>

using namespace Utils;

namespace TextEditor {

static QMap<Id, ICodeStylePreferencesFactory *> g_languageToFactory;

ICodeStylePreferencesFactory::ICodeStylePreferencesFactory(Id languageId)
    : m_languageId(languageId)
{
    if (languageId.isValid())
        g_languageToFactory.insert(languageId, this);
}

ICodeStylePreferencesFactory::~ICodeStylePreferencesFactory()
{
    if (m_languageId.isValid())
        g_languageToFactory.remove(m_languageId);
}

Id ICodeStylePreferencesFactory::languageId() const
{
    return m_languageId;
}

QString ICodeStylePreferencesFactory::displayName() const
{
    return m_displayName;
}

QString ICodeStylePreferencesFactory::snippetGroupId() const
{
    return m_snippetGroupId;
}

QString ICodeStylePreferencesFactory::previewText() const
{
    return m_previewText;
}

Indenter *ICodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return m_indenterCreator ? m_indenterCreator(doc) : nullptr;
}

ICodeStylePreferences *ICodeStylePreferencesFactory::createCodeStyle() const
{
    return m_codeStyleCreator ? m_codeStyleCreator() : nullptr;
}

CodeStyleEditor *ICodeStylePreferencesFactory::createSettingsEditor(
    ICodeStylePreferences *codeStyle) const
{
    return m_settingsEditorCreator ? m_settingsEditorCreator(codeStyle) : nullptr;
}

void ICodeStylePreferencesFactory::setDisplayName(const QString &displayName)
{
    m_displayName = displayName;
}

void ICodeStylePreferencesFactory::setSnippetGroupId(const QString &snippetGroupId)
{
    m_snippetGroupId = snippetGroupId;
}

void ICodeStylePreferencesFactory::setPreviewText(const QString &previewText)
{
    m_previewText = previewText;
}

void ICodeStylePreferencesFactory::setIndenterCreator(const IndenterCreator &creator)
{
    m_indenterCreator = creator;
}

void ICodeStylePreferencesFactory::setCodeStyleCreator(const CodeStyleCreator &creator)
{
    m_codeStyleCreator = creator;
}

void ICodeStylePreferencesFactory::setSettingsEditorCreator(const SettingsEditorCreator &creator)
{
    m_settingsEditorCreator = creator;
}

void ICodeStylePreferencesFactory::setProjectEditorCreator(const ProjectEditorCreator &creator)
{
    m_projectEditorCreator = creator;
}

ICodeStylePreferencesFactory *codeStyleFactory(Id languageId)
{
    return g_languageToFactory.value(languageId);
}

const QMap<Id, ICodeStylePreferencesFactory *> &codeStyleFactories()
{
    return g_languageToFactory;
}

} // namespace TextEditor
