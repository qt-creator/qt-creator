/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#ifndef VCPROJECTMANAGER_INTERNAL_GLOBALS_H
#define VCPROJECTMANAGER_INTERNAL_GLOBALS_H

#include "ivcprojectnodemodel.h"

#include <QList>
#include <QHash>

#include "global.h"

namespace VcProjectManager {
namespace Internal {

class Global;

class Globals : public IVcProjectXMLNode
{
public:
    typedef QSharedPointer<Globals> Ptr;

    Globals();
    Globals(const Globals &globals);
    Globals& operator=(const Globals &globals);
    ~Globals();

    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    bool isEmpty() const;

    void processGlobal(const QDomNode &globalNode);

    void addGlobal(Global::Ptr global);
    void removeGlobal(Global::Ptr global);
    void removeGlobal(const QString &globalName);
    QList<Global::Ptr > globals() const;
    Global::Ptr global(const QString &name);

private:
    QList<Global::Ptr > m_globals;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_GLOBALS_H
