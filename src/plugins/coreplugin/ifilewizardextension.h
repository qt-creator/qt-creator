// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"
#include "generatedfile.h"

#include <QObject>
#include <QList>
#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QWizardPage;
QT_END_NAMESPACE

namespace Core {

class IWizardFactory;

/*!
  Hook to add generic wizard pages to implementations of IWizard.
  Used e.g. to add "Add to Project File/Add to Version Control" page
  */
class CORE_EXPORT IFileWizardExtension : public QObject
{
    Q_OBJECT
public:
    IFileWizardExtension();
    ~IFileWizardExtension() override;

    /* Return a list of pages to be added to the Wizard (empty list if not
     * applicable). */
    virtual QList<QWizardPage *> extensionPages(const IWizardFactory *wizard) = 0;

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
