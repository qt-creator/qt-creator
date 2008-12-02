/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#ifndef VCSBaseBASEEDITORFACTORY_H
#define VCSBaseBASEEDITORFACTORY_H

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
}

#endif // VCSBaseBASEEDITOR_H
