/*
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "repositorywrapper.h"

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

using namespace KSyntaxHighlighting;

RepositoryWrapper::RepositoryWrapper(QObject *parent)
    : QObject(parent)
{
}

Definition RepositoryWrapper::definitionForName(const QString &defName) const
{
    return m_repository->definitionForName(defName);
}

Definition RepositoryWrapper::definitionForFileName(const QString &fileName) const
{
    return m_repository->definitionForFileName(fileName);
}

QVector<Definition> RepositoryWrapper::definitionsForFileName(const QString &fileName) const
{
    return m_repository->definitionsForFileName(fileName);
}

Definition RepositoryWrapper::definitionForMimeType(const QString &mimeType) const
{
    return m_repository->definitionForMimeType(mimeType);
}

QVector<Definition> RepositoryWrapper::definitionsForMimeType(const QString &mimeType) const
{
    return m_repository->definitionsForMimeType(mimeType);
}

QVector<Definition> RepositoryWrapper::definitions() const
{
    return m_repository->definitions();
}

QVector<Theme> RepositoryWrapper::themes() const
{
    return m_repository->themes();
}

Theme RepositoryWrapper::theme(const QString &themeName) const
{
    return m_repository->theme(themeName);
}

Theme RepositoryWrapper::defaultTheme(DefaultTheme t) const
{
    return m_repository->defaultTheme(static_cast<Repository::DefaultTheme>(t));
}

#include "moc_repositorywrapper.cpp"
