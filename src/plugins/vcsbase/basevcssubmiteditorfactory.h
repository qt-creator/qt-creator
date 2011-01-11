/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    virtual QString id() const;
    virtual QString displayName() const;
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
