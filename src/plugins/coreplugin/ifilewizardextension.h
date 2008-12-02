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
