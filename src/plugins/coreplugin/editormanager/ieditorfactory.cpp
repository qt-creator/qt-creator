/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ieditorfactory.h"
#include "ieditorfactory_p.h"
#include "editormanager.h"

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Core {

static QList<IEditorFactory *> g_editorFactories;
static QHash<Utils::MimeType, IEditorFactory *> g_userPreferredEditorFactories;

IEditorFactory::IEditorFactory(QObject *parent)
    : QObject(parent)
{
    g_editorFactories.append(this);
}

IEditorFactory::~IEditorFactory()
{
    g_editorFactories.removeOne(this);
}

const EditorFactoryList IEditorFactory::allEditorFactories()
{
    return g_editorFactories;
}

/*!
    Returns all available editors for this \a mimeType in the default order
    (editors ordered by mime type hierarchy).
*/
const EditorFactoryList IEditorFactory::defaultEditorFactories(const Utils::MimeType &mimeType)
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = IEditorFactory::allEditorFactories();
    Internal::mimeTypeFactoryLookup(mimeType, allFactories, &rc);
    return rc;
}

/*!
    Returns the available editors for \a fileName in order of preference.
    That is the default order for the file's MIME type but with a user overridden default
    editor first, and if the file is a too large text file, with the binary editor as the
    very first.
*/
const EditorFactoryList IEditorFactory::preferredEditorFactories(const QString &fileName)
{
    const QFileInfo fileInfo(fileName);
    // default factories by mime type
    const Utils::MimeType mimeType = Utils::mimeTypeForFile(fileInfo);
    EditorFactoryList factories = defaultEditorFactories(mimeType);
    const auto factories_moveToFront = [&factories](IEditorFactory *f) {
        factories.removeAll(f);
        factories.prepend(f);
    };
    // user preferred factory to front
    IEditorFactory *userPreferred = Internal::userPreferredEditorFactories().value(mimeType);
    if (userPreferred)
        factories_moveToFront(userPreferred);
    // open text files > 48 MB in binary editor
    if (fileInfo.size() > EditorManager::maxTextFileSize()
            && mimeType.inherits("text/plain")) {
        const Utils::MimeType binary = Utils::mimeTypeForName("application/octet-stream");
        const EditorFactoryList binaryEditors = defaultEditorFactories(binary);
        if (!binaryEditors.isEmpty())
            factories_moveToFront(binaryEditors.first());
    }
    return factories;
}

QHash<Utils::MimeType, Core::IEditorFactory *> Core::Internal::userPreferredEditorFactories()
{
    return g_userPreferredEditorFactories;
}

void Internal::setUserPreferredEditorFactories(const QHash<Utils::MimeType, IEditorFactory *> &factories)
{
    g_userPreferredEditorFactories = factories;
}

} // Core
