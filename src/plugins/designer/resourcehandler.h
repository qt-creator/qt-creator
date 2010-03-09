/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef RESOURCEHANDLER_H
#define RESOURCEHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
namespace qdesigner_internal {
    class FormWindowBase;
}
QT_END_NAMESPACE

namespace ProjectExplorer {
class SessionNode;
class NodesWatcher;
}

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
    explicit ResourceHandler(qdesigner_internal::FormWindowBase *fw);
    virtual ~ResourceHandler();

public slots:
    void updateResources();

private:
    void ensureInitialized();

    qdesigner_internal::FormWindowBase * const m_form;

    QStringList m_originalUiQrcPaths;
    ProjectExplorer::SessionNode *m_sessionNode;
    ProjectExplorer::NodesWatcher *m_sessionWatcher;
};

} // namespace Internal
} // namespace Designer

#endif // RESOURCEHANDLER_H
