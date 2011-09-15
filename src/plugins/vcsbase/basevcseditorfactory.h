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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef BASEVCSEDITORFACTORY_H
#define BASEVCSEDITORFACTORY_H

#include "vcsbase_global.h"
#include "vcsbaseeditor.h"

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QStringList>

namespace VCSBase {

struct BaseVCSEditorFactoryPrivate;

class VCSBASE_EXPORT BaseVCSEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    explicit BaseVCSEditorFactory(const VCSBaseEditorParameters *type);
    virtual ~BaseVCSEditorFactory();

    virtual QStringList mimeTypes() const;
    // IEditorFactory

    virtual Core::Id id() const;
    virtual QString displayName() const;

    virtual Core::IFile *open(const QString &fileName);
    virtual Core::IEditor *createEditor(QWidget *parent);

private:
    // Implement to create and initialize (call init()) a
    // VCSBaseEditor subclass
    virtual VCSBaseEditorWidget *createVCSBaseEditor(const VCSBaseEditorParameters *type,
                                               QWidget *parent) = 0;

    BaseVCSEditorFactoryPrivate *d;
};

// Utility template to create an editor.
template <class Editor>
class VCSEditorFactory : public BaseVCSEditorFactory
{
public:
    explicit VCSEditorFactory(const VCSBaseEditorParameters *type,
                              QObject *describeReceiver = 0,
                              const char *describeSlot = 0);

private:
    virtual VCSBaseEditorWidget *createVCSBaseEditor(const VCSBaseEditorParameters *type,
                                               QWidget *parent);
    QObject *m_describeReceiver;
    const char *m_describeSlot;
};

template <class Editor>
VCSEditorFactory<Editor>::VCSEditorFactory(const VCSBaseEditorParameters *type,
                                           QObject *describeReceiver,
                                           const char *describeSlot) :
    BaseVCSEditorFactory(type),
    m_describeReceiver(describeReceiver),
    m_describeSlot(describeSlot)
{
}

template <class Editor>
VCSBaseEditorWidget *VCSEditorFactory<Editor>::createVCSBaseEditor(const VCSBaseEditorParameters *type,
                                                             QWidget *parent)
{
    VCSBaseEditorWidget *rc = new Editor(type, parent);
    rc->init();
    if (m_describeReceiver)
        connect(rc, SIGNAL(describeRequested(QString,QString)), m_describeReceiver, m_describeSlot);
    return rc;

}

} // namespace VCSBase

#endif // BASEVCSEDITORFACTORY_H

