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

#ifndef VCSBASE_BASEEDITORFACTORY_H
#define VCSBASE_BASEEDITORFACTORY_H

#include "vcsbase_global.h"

#include <coreplugin/editormanager/ieditorfactory.h>

namespace VCSBase {

class  VCSBaseSubmitEditor;
struct VCSBaseSubmitEditorParameters;
struct BaseVCSSubmitEditorFactoryPrivate;

// Parametrizable base class for editor factories creating instances of
// VCSBaseSubmitEditor subclasses.
class VCSBASE_EXPORT BaseVCSSubmitEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

protected:
    explicit BaseVCSSubmitEditorFactory(const VCSBaseSubmitEditorParameters *parameters);

public:
    virtual ~BaseVCSSubmitEditorFactory();

    virtual Core::IEditor *createEditor(QWidget *parent);
    virtual QString kind() const;
    virtual QStringList mimeTypes() const;
    Core::IFile *open(const QString &fileName);

private:
    virtual VCSBaseSubmitEditor
        *createBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                QWidget *parent) = 0;

    BaseVCSSubmitEditorFactoryPrivate *m_d;
};

// Utility template to create an editor that has a constructor taking the
// parameter struct and a parent widget.

template <class Editor>
class VCSSubmitEditorFactory : public BaseVCSSubmitEditorFactory
{
public:
    explicit VCSSubmitEditorFactory(const VCSBaseSubmitEditorParameters *parameters);

private:
    virtual VCSBaseSubmitEditor
        *createBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                QWidget *parent);
};

template <class Editor>
VCSSubmitEditorFactory<Editor>::VCSSubmitEditorFactory(const VCSBaseSubmitEditorParameters *parameters) :
    BaseVCSSubmitEditorFactory(parameters)
{
}

template <class Editor>
VCSBaseSubmitEditor *VCSSubmitEditorFactory<Editor>::createBaseSubmitEditor(const VCSBaseSubmitEditorParameters *parameters,
                                                                            QWidget *parent)
{
    return new Editor(parameters, parent);
}

} // namespace VCSBase

#endif // VCSBASE_BASEEDITOR_H
