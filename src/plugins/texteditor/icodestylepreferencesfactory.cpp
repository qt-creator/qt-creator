// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icodestylepreferencesfactory.h"

#include "codestylepool.h"
#include "icodestylepreferences.h"
#include "texteditortr.h"

#include <utils/qtcassert.h>

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

    if (m_pool) {
        unregisterCodeStyle(m_languageId);
        // The factory owns the global style (created here); the built-in styles
        // are members of the concrete factory and are already gone by the time
        // this base destructor runs; the pool owns and deletes its custom styles.
        if (QTC_GUARD(m_globalCodeStyle))
            m_globalCodeStyle->setCurrentDelegate(nullptr);
        delete m_globalCodeStyle;
        delete m_pool;
    }
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

QWidget *ICodeStylePreferencesFactory::createValueEditor(ICodeStylePreferences *codeStyle) const
{
    return m_valueEditorCreator ? m_valueEditorCreator(codeStyle) : nullptr;
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

void ICodeStylePreferencesFactory::setValueEditorCreator(const ValueEditorCreator &creator)
{
    m_valueEditorCreator = creator;
}

void ICodeStylePreferencesFactory::setProjectEditorCreator(const ProjectEditorCreator &creator)
{
    m_projectEditorCreator = creator;
}

void ICodeStylePreferencesFactory::setGlobalCodeStyleId(const QByteArray &id)
{
    m_globalCodeStyleId = id;
}

void ICodeStylePreferencesFactory::setDefaultCodeStyleId(const QByteArray &id)
{
    m_defaultCodeStyleId = id;
}

void ICodeStylePreferencesFactory::setBuiltInCodeStyles(
    const std::function<void(CodeStylePool *)> &adder)
{
    m_builtInCodeStyles = adder;
}

void ICodeStylePreferencesFactory::setupCodeStyles()
{
    QTC_ASSERT(!m_pool, return);
    m_pool = new CodeStylePool(this, m_languageId);

    // The editable global style is added first (so it leads the per-project
    // "based on" list), then the built-ins, so the default delegate is already
    // in the pool when it is selected.
    m_globalCodeStyle = createCodeStyle();
    QTC_ASSERT(m_globalCodeStyle, return);
    m_globalCodeStyle->setDelegatingPool(m_pool);
    m_globalCodeStyle->setId(m_globalCodeStyleId);
    m_globalCodeStyle->setDisplayName(Tr::tr("Global", "Settings"));
    m_pool->addCodeStyle(m_globalCodeStyle);

    if (m_builtInCodeStyles)
        m_builtInCodeStyles(m_pool);

    if (ICodeStylePreferences *defaultStyle = m_pool->codeStyle(m_defaultCodeStyleId))
        m_globalCodeStyle->setCurrentDelegate(defaultStyle);

    m_pool->loadCustomCodeStyles();

    // Load the saved global settings after the pool is complete.
    m_globalCodeStyle->fromSettings(m_languageId.toKey());

    registerCodeStyle(m_languageId, m_globalCodeStyle);
}

ICodeStylePreferences *ICodeStylePreferencesFactory::globalCodeStyle() const
{
    return m_globalCodeStyle;
}

CodeStylePool *ICodeStylePreferencesFactory::codeStylePool() const
{
    return m_pool;
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
