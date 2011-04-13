/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef FORMEDITORFACTORY_H
#define FORMEDITORFACTORY_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QStringList>

namespace Core {
class IEditor;
class IFile;
}

namespace Designer {
namespace Internal {

class FormEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    FormEditorFactory();

    virtual QStringList mimeTypes() const;

    // IEditorFactory
    virtual QString id() const;
    virtual QString displayName() const;
    Core::IFile *open(const QString &fileName);
    Core::IEditor *createEditor(QWidget *parent);

private slots:
    void designerModeClicked();
    void updateEditorInfoBar(Core::IEditor *editor);

private:
    const QStringList m_mimeTypes;
};

} // namespace Internal
} // namespace Designer

#endif // FORMEDITORFACTORY_H
