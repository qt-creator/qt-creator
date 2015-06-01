/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RESOURCEHANDLER_H
#define RESOURCEHANDLER_H

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace ProjectExplorer { class SessionNode; }

namespace Designer {
namespace Internal {

/* ResourceHandler: Constructed on a form window and activated on open/save as
 * (see README.txt). The form can have 2 states:
 * 1) standalone: Uses the form editor's list of resource files.
 * 2) Within a project: Use the list of resources files of the projects.
 *
 * When initializing, store the original list of qrc files of the form and
 * connect to various signals of the project explorer to re-check.
 * In updateResources, check whether the form is part of a project and use
 * the project's resource files or the stored ones. */

class ResourceHandler : public QObject
{
    Q_OBJECT
public:
    explicit ResourceHandler(QDesignerFormWindowInterface *fw);
    virtual ~ResourceHandler();

public slots:
    void updateResources()        { updateResourcesHelper(false); }
    void updateProjectResources() { updateResourcesHelper(true); }

private:
    void ensureInitialized();
    void updateResourcesHelper(bool updateProjectResources);

    QDesignerFormWindowInterface * const m_form;
    QStringList m_originalUiQrcPaths;
    bool m_initialized = false;
    bool m_handlingResources = false;
};

} // namespace Internal
} // namespace Designer

#endif // RESOURCEHANDLER_H
