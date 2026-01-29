/*
    SPDX-FileCopyrightText: 2025 Volker Krause <vkrause@kde.org>
    SPDX-License-Identifier: MIT
*/

#ifndef TYPES_H
#define TYPES_H

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include <QQmlEngine>

struct DefinitionForeign {
    Q_GADGET
    QML_FOREIGN(KSyntaxHighlighting::Definition)
    QML_VALUE_TYPE(definition)
};

struct ThemeForeign {
    Q_GADGET
    QML_FOREIGN(KSyntaxHighlighting::Theme)
    QML_VALUE_TYPE(theme)
};

struct RepositoryForeign {
    Q_GADGET
    QML_FOREIGN(KSyntaxHighlighting::Repository)
    QML_SINGLETON
    QML_NAMED_ELEMENT(Repository)
public:
    static KSyntaxHighlighting::Repository *create(QQmlEngine *engine, QJSEngine *scriptEngine);
};

#endif
