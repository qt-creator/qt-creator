// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

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

    void updateResources()        { updateResourcesHelper(false); }
    void updateProjectResources() { updateResourcesHelper(true); }

private:
    void ensureInitialized();
    void updateResourcesHelper(bool updateProjectResources);

    QDesignerFormWindowInterface * const m_form = nullptr;
    QStringList m_originalUiQrcPaths;
    bool m_initialized = false;
    bool m_handlingResources = false;
};

} // namespace Internal
} // namespace Designer
