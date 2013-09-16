/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_VCPROJECTNODE_H
#define VCPROJECTMANAGER_INTERNAL_VCPROJECTNODE_H

#include <QDomNode>

namespace VcProjectManager {
namespace Internal {

class VcNodeWidget;

class IVcProjectXMLNode
{
public:
    IVcProjectXMLNode();
    virtual ~IVcProjectXMLNode();

    /*!
     * This member function is called when project file (vcproj) is processed.
     * Note that before processing, the project file is loaded using XML DOM.
     * After that, DOM document is processed recursively, node by node.
     * When DOM node is processed a new project node is created.
     * Attributes and children of that DOM node must be processed. That's when this member function is called.
     * \param node represents current XML DOM node we are processing
     */
    virtual void processNode(const QDomNode &node) = 0;

    /*!
     * This member function is used to create settings widget for a node.
     * For example, every Configuration project node creates it's own settings widget.
     * This widget is created by calling createSettingsWidget() of every Tool project node present
     * in that configuration and “appending” that widget as a child of a parent configuration widget.
     * \return A pointer to a newly created settings widhget for a node.
     */
    virtual VcNodeWidget* createSettingsWidget() = 0;

    /*!
     * This member function is called when project is creating it's own XML DOM  representation.
     * Parent's implementation of toXMLDomNode is calling toXMLDomNode for every child.
     * For example, we call this one in VcProjectDocument::saveToFile(const QString &filePath) member
     * function's implementation in order to create project's XML DOM document which is used to save a project to a file.
     * \param domXMLDocument is the parent XML DOM document to which newly created XML DOM node will belong to.
     * \return A XML DOM node that represents this project node.
     */
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const = 0;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCPROJECTNODE_H
