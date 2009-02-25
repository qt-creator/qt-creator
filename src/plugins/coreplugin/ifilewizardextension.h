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

#ifndef IFILEWIZARDEXTENSION_H
#define IFILEWIZARDEXTENSION_H

#include <coreplugin/core_global.h>

#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QWizardPage;
QT_END_NAMESPACE

namespace Core {

class IWizard;
class GeneratedFile;

/*!
  Hook to add generic wizard pages to implementations of IWizard.
  Used e.g. to add "Add to Project File/Add to version control" page
  */
class CORE_EXPORT IFileWizardExtension : public QObject
{
    Q_OBJECT
public:
    /* Return a list of pages to be added to the Wizard (empty list if not
     * applicable). */
    virtual QList<QWizardPage *> extensionPages(const IWizard *wizard) = 0;

    /* Process the files using the extension parameters */
    virtual bool process(const QList<GeneratedFile> &files, QString *errorMessage) = 0;

public slots:
    /* Notification about the first extension page being shown. */
    virtual void firstExtensionPageShown(const QList<GeneratedFile> &) {}
};

} // namespace Core

#endif // IFILEWIZARDEXTENSION_H
