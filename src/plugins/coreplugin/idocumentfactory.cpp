// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idocumentfactory.h"

#include <utils/qtcassert.h>

namespace Core {

static QList<IDocumentFactory *> g_documentFactories;

/*!
    \class Core::IDocumentFactory
    \inheaderfile coreplugin/idocumentfactory.cpp
    \inmodule QtCreator

    \brief The IDocumentFactory class is used to open files that are not opened in an editor.

    When users open a document, the document factory responsible for the corresponding MIME type is determined by checking the \l mimeTypes() of the factories.

    Document factories that are \l {isProjectFactory()} {project factories} are not
    considered if the user specifically requests to open a file, unless they are
    \l {isUsedForOpenFile()} {used for opening files}.

    \sa setMimeTypes()
    \sa addMimeType()
    \sa setIsProjectFactory()
    \sa setUsedForOpenFile()
    \sa setOpener()
*/

/*!
    \fn IDocumentFactory::setIsProjectFactory(bool isProjectFactory)
    \brief Sets whether this factory is used to open projects to \a isProjectFactory.

    Factories that are not used to open projects are considered when the
    user triggers File > Open File.

    \sa isProjectFactory()
*/

/*!
    \fn IDocumentFactory::isProjectFactory() const
    \brief Returns whether this factory is used to open projects.

    Defaults to \c false.

    \sa setIsProjectFactory()
*/

/*!
    \fn IDocumentFactory::setUsedForOpenFile(bool usedForOpenFile)
    \brief Sets whether this factory is considered when the user requests to
    open a file to \a usedForOpenFile.

    This is for project factories that open something that cannot be opened
    in an editor, such as a directory.

    \sa isUsedForOpenFile()
*/

/*!
    \fn IDocumentFactory::isUsedForOpenFile() const
    \brief Returns whether this factory is considered when the user requests to
    open a file.

    Defaults to \c true for factories that are not \l {isProjectFactory()}
    {project factories}, and to \c false for those that are.

    \sa setUsedForOpenFile()
*/

/*!
    \fn IDocumentFactory::setOpener(const Opener &opener)
    \brief Sets the \a opener function that is called to open a file.
*/

/*!
    \fn IDocumentFactory::setMimeTypes(const QStringList &mimeTypes)
    \brief Sets the \a mimeTypes that this factory can open.
*/

/*!
    \fn IDocumentFactory::addMimeType(const char *mimeType)
    \brief Adds the \a mimeType to the list of mime types that this factory can open.
*/

/*!
    \fn IDocumentFactory::addMimeType(const QString &mimeType)
    \brief Adds the MIME type with name \a mimeType to the list of mime types that this factory can open.
*/

/*!
    \internal
    \fn IDocumentFactory::allDocumentFactories()
*/

/*!
    \fn IDocumentFactory::mimeTypes() const
    \brief Returns the list of mime types that this factory can open.

    \sa setMimeTypes()
    \sa addMimeType()
*/

/*!
    Creates a new document factory.
*/
IDocumentFactory::IDocumentFactory()
{
    g_documentFactories.append(this);
}

/*!
    \internal
*/
IDocumentFactory::~IDocumentFactory()
{
    g_documentFactories.removeOne(this);
}

/*!
    \internal
*/
const QList<IDocumentFactory *> IDocumentFactory::allDocumentFactories()
{
    return g_documentFactories;
}

/*!
    \brief Opens the file at \a filePath.

    This uses the opener function set by \l setOpener() to open the file.
    Returns the document that was opened or \c nullptr if an error occurred.

    \sa setOpener()
*/
IDocument *IDocumentFactory::open(const Utils::FilePath &filePath)
{
    QTC_ASSERT(m_opener, return nullptr);
    return m_opener(filePath);
}

} // namespace Core
