/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASEVCSEDITORFACTORY_H
#define BASEVCSEDITORFACTORY_H

#include "vcsbase_global.h"
#include "vcsbaseeditor.h"

#include <coreplugin/editormanager/ieditorfactory.h>

#include <QStringList>

namespace VcsBase {
namespace Internal {
class BaseVcsEditorFactoryPrivate;
} // namespace Internal

class VCSBASE_EXPORT BaseVcsEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT
public:
    explicit BaseVcsEditorFactory(const VcsBaseEditorParameters *type);
    ~BaseVcsEditorFactory();

    QStringList mimeTypes() const;
    // IEditorFactory

    Core::Id id() const;
    QString displayName() const;
    Core::IEditor *createEditor(QWidget *parent);

private:
    // Implement to create and initialize (call init()) a
    // VcsBaseEditor subclass
    virtual VcsBaseEditorWidget *createVcsBaseEditor(const VcsBaseEditorParameters *type,
                                               QWidget *parent) = 0;

    Internal::BaseVcsEditorFactoryPrivate *const d;
};

// Utility template to create an editor.
template <class Editor>
class VcsEditorFactory : public BaseVcsEditorFactory
{
public:
    explicit VcsEditorFactory(const VcsBaseEditorParameters *type,
                              QObject *describeReceiver = 0,
                              const char *describeSlot = 0);

private:
    VcsBaseEditorWidget *createVcsBaseEditor(const VcsBaseEditorParameters *type,
                                             QWidget *parent);
    QObject *m_describeReceiver;
    const char *m_describeSlot;
};

template <class Editor>
VcsEditorFactory<Editor>::VcsEditorFactory(const VcsBaseEditorParameters *type,
                                           QObject *describeReceiver,
                                           const char *describeSlot) :
    BaseVcsEditorFactory(type),
    m_describeReceiver(describeReceiver),
    m_describeSlot(describeSlot)
{
}

template <class Editor>
VcsBaseEditorWidget *VcsEditorFactory<Editor>::createVcsBaseEditor(const VcsBaseEditorParameters *type,
                                                             QWidget *parent)
{
    VcsBaseEditorWidget *rc = new Editor(type, parent);
    rc->init();
    if (m_describeReceiver)
        connect(rc, SIGNAL(describeRequested(QString,QString)), m_describeReceiver, m_describeSlot);
    return rc;

}

} // namespace VcsBase

#endif // BASEVCSEDITORFACTORY_H

