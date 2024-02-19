/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "kquicksyntaxhighlighter.h"

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QGuiApplication>
#include <QPalette>
#include <QQuickTextDocument>
#include <QTextDocument>

using namespace KSyntaxHighlighting;

extern Repository *defaultRepository();

KQuickSyntaxHighlighter::KQuickSyntaxHighlighter(QObject *parent)
    : QObject(parent)
    , m_textEdit(nullptr)
    , m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(this))
{
}

KQuickSyntaxHighlighter::~KQuickSyntaxHighlighter() = default;

QObject *KQuickSyntaxHighlighter::textEdit() const
{
    return m_textEdit;
}

void KQuickSyntaxHighlighter::setTextEdit(QObject *textEdit)
{
    if (m_textEdit != textEdit) {
        m_textEdit = textEdit;
        m_highlighter->setDocument(m_textEdit->property("textDocument").value<QQuickTextDocument *>()->textDocument());
    }
}

QVariant KQuickSyntaxHighlighter::definition() const
{
    return QVariant::fromValue(m_definition);
}

void KQuickSyntaxHighlighter::setDefinition(const QVariant &definition)
{
    Definition def;
    if (definition.userType() == QMetaType::QString) {
        def = unwrappedRepository()->definitionForName(definition.toString());
    } else {
        def = definition.value<Definition>();
    }

    if (m_definition != def) {
        m_definition = def;

        m_highlighter->setTheme(m_theme.isValid() ? m_theme : unwrappedRepository()->themeForPalette(QGuiApplication::palette()));
        m_highlighter->setDefinition(def);

        Q_EMIT definitionChanged();
    }
}

QVariant KQuickSyntaxHighlighter::theme() const
{
    return QVariant::fromValue(m_theme);
}

void KQuickSyntaxHighlighter::setTheme(const QVariant &theme)
{
    Theme t;
    if (theme.userType() == QMetaType::QString) {
        t = unwrappedRepository()->theme(theme.toString());
    } else if (theme.userType() == QMetaType::Int) {
        t = unwrappedRepository()->defaultTheme(static_cast<Repository::DefaultTheme>(theme.toInt()));
    } else {
        t = theme.value<Theme>();
    }

    if (m_theme.name() != t.name()) {
        m_theme = t;
        m_highlighter->setTheme(m_theme);
        m_highlighter->rehighlight();
        Q_EMIT themeChanged();
    }
}

Repository *KQuickSyntaxHighlighter::repository() const
{
    return m_repository;
}

void KQuickSyntaxHighlighter::setRepository(Repository *repository)
{
    if (m_repository == repository) {
        return;
    }
    m_repository = repository;
    Q_EMIT repositoryChanged();
}

Repository *KQuickSyntaxHighlighter::unwrappedRepository() const
{
    if (m_repository) {
        return m_repository;
    }
    return defaultRepository();
}

#include "moc_kquicksyntaxhighlighter.cpp"
