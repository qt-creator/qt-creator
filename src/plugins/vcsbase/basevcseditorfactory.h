/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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

// Base class for editor factories creating instances of VCSBaseEditor
//  subclasses.
class VCSBASE_EXPORT BaseVCSEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    explicit BaseVCSEditorFactory(const VCSBaseEditorParameters *type);
    virtual ~BaseVCSEditorFactory();

    virtual QStringList mimeTypes() const;
    // IEditorFactory

    virtual QString kind() const;
    virtual Core::IFile *open(const QString &fileName);
    virtual Core::IEditor *createEditor(QWidget *parent);

private:
    // Implement to create and initialize (call init()) a
    // VCSBaseEditor subclass
    virtual VCSBaseEditor *createVCSBaseEditor(const VCSBaseEditorParameters *type,
                                               QWidget *parent) = 0;

    BaseVCSEditorFactoryPrivate *m_d;
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
    virtual VCSBaseEditor *createVCSBaseEditor(const VCSBaseEditorParameters *type,
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
VCSBaseEditor *VCSEditorFactory<Editor>::createVCSBaseEditor(const VCSBaseEditorParameters *type,
                                                             QWidget *parent)
{
    VCSBaseEditor *rc = new Editor(type, parent);
    rc->init();
    if (m_describeReceiver)
        connect(rc, SIGNAL(describeRequested(QString,QString)), m_describeReceiver, m_describeSlot);
    return rc;

}

} // namespace VCSBase

#endif // BASEVCSEDITORFACTORY_H

