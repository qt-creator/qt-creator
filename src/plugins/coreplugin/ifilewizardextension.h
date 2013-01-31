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

#ifndef IFILEWIZARDEXTENSION_H
#define IFILEWIZARDEXTENSION_H

#include <coreplugin/core_global.h>

#include <QObject>
#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QWizardPage;
QT_END_NAMESPACE

namespace Core {

class IWizard;
class GeneratedFile;

/*!
  Hook to add generic wizard pages to implementations of IWizard.
  Used e.g. to add "Add to Project File/Add to Version Control" page
  */
class CORE_EXPORT IFileWizardExtension : public QObject
{
    Q_OBJECT
public:
    /* Return a list of pages to be added to the Wizard (empty list if not
     * applicable). */
    virtual QList<QWizardPage *> extensionPages(const IWizard *wizard) = 0;

    /* Process the files using the extension parameters */
    virtual bool processFiles(const QList<GeneratedFile> &files,
                         bool *removeOpenProjectAttribute,
                         QString *errorMessage) = 0;
    /* Applies code style settings which may depend on the project to which
     * the files will be added.
     * This function is called before the files are actually written out,
     * before processFiles() is called*/
    virtual void applyCodeStyle(GeneratedFile *file) const = 0;

public slots:
    /* Notification about the first extension page being shown. */
    virtual void firstExtensionPageShown(const QList<GeneratedFile> &files, const QVariantMap &extraValues) {
        Q_UNUSED(files)
        Q_UNUSED(extraValues)
        }
};

} // namespace Core

#endif // IFILEWIZARDEXTENSION_H
