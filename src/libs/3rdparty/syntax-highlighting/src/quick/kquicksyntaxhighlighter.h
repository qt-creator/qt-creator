/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KQUICKSYNTAXHIGHLIGHTER_H
#define KQUICKSYNTAXHIGHLIGHTER_H

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

#include <QObject>
#include <QVariant>

namespace KSyntaxHighlighting
{
class Repository;
class SyntaxHighlighter;
}

class KQuickSyntaxHighlighter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject *textEdit READ textEdit WRITE setTextEdit NOTIFY textEditChanged)
    Q_PROPERTY(QVariant definition READ definition WRITE setDefinition NOTIFY definitionChanged)
    Q_PROPERTY(QVariant theme READ theme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(KSyntaxHighlighting::Repository *repository READ repository WRITE setRepository NOTIFY repositoryChanged)

public:
    explicit KQuickSyntaxHighlighter(QObject *parent = nullptr);
    ~KQuickSyntaxHighlighter() override;

    QObject *textEdit() const;
    void setTextEdit(QObject *textEdit);

    QVariant definition() const;
    void setDefinition(const QVariant &definition);

    QVariant theme() const;
    void setTheme(const QVariant &theme);

    KSyntaxHighlighting::Repository *repository() const;
    void setRepository(KSyntaxHighlighting::Repository *repository);

Q_SIGNALS:
    void textEditChanged() const;
    void definitionChanged() const;
    void themeChanged();
    void repositoryChanged();

private:
    KSyntaxHighlighting::Repository *unwrappedRepository() const;

    QObject *m_textEdit;
    KSyntaxHighlighting::Definition m_definition;
    KSyntaxHighlighting::Theme m_theme;
    KSyntaxHighlighting::Repository *m_repository = nullptr;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;
};

#endif // KQUICKSYNTAXHIGHLIGHTER_H
