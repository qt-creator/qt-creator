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

#ifndef IPROJECTPROPERTIES_H
#define IPROJECTPROPERTIES_H

#include "projectexplorer_export.h"
#include "project.h"

#include <coreplugin/icontext.h>

namespace ProjectExplorer {

class PropertiesPanel;

class PROJECTEXPLORER_EXPORT IPanelFactory : public QObject
{
    Q_OBJECT
public:
    virtual bool supports(Project *project) = 0;
    virtual PropertiesPanel *createPanel(Project *project) = 0;
};

class PROJECTEXPLORER_EXPORT PropertiesPanel : public Core::IContext
{
    Q_OBJECT
public:
    virtual void finish() {}
    virtual QString name() const = 0;

    // IContext
    virtual QList<int> context() const { return QList<int>(); }
};

} // namespace ProjectExplorer

#endif // IPROJECTPROPERTIES_H
