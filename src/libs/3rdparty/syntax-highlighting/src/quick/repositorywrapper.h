/*
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef REPOSITORYWRAPPER_H
#define REPOSITORYWRAPPER_H

#include <QObject>

namespace KSyntaxHighlighting
{
class Definition;
class Repository;
class Theme;
}

// TODO KF6: merge this into KSyntaxHighlighting::Repository
class RepositoryWrapper : public QObject
{
    Q_OBJECT
    // TODO KF6: NOTIFY on reload
    Q_PROPERTY(QVector<KSyntaxHighlighting::Definition> definitions READ definitions CONSTANT)
    Q_PROPERTY(QVector<KSyntaxHighlighting::Theme> themes READ themes CONSTANT)
public:
    explicit RepositoryWrapper(QObject *parent = nullptr);

    Q_INVOKABLE KSyntaxHighlighting::Definition definitionForName(const QString &defName) const;
    Q_INVOKABLE KSyntaxHighlighting::Definition definitionForFileName(const QString &fileName) const;
    Q_INVOKABLE QVector<KSyntaxHighlighting::Definition> definitionsForFileName(const QString &fileName) const;
    Q_INVOKABLE KSyntaxHighlighting::Definition definitionForMimeType(const QString &mimeType) const;
    Q_INVOKABLE QVector<KSyntaxHighlighting::Definition> definitionsForMimeType(const QString &mimeType) const;
    QVector<KSyntaxHighlighting::Definition> definitions() const;

    QVector<KSyntaxHighlighting::Theme> themes() const;
    Q_INVOKABLE KSyntaxHighlighting::Theme theme(const QString &themeName) const;
    enum DefaultTheme { LightTheme, DarkTheme };
    Q_ENUM(DefaultTheme)
    Q_INVOKABLE KSyntaxHighlighting::Theme defaultTheme(DefaultTheme t = LightTheme) const;

    KSyntaxHighlighting::Repository *m_repository = nullptr;
};

#endif // REPOSITORYWRAPPER_H
