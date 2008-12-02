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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef LIBRARYWIZARDDIALOG_H
#define LIBRARYWIZARDDIALOG_H

#include <QtGui/QWizard>

namespace Qt4ProjectManager {
namespace Internal {

struct QtProjectParameters;
class ModulesPage;
class LibraryIntroPage;
class FilesPage;
struct LibraryParameters;

// Library wizard dialog.
class LibraryWizardDialog : public QWizard
{
    Q_OBJECT

public:
    explicit LibraryWizardDialog(const QString &templateName,
                                 const QIcon &icon,
                                 const QList<QWizardPage*> &extensionPages,
                                 QWidget *parent = 0);

    void setSuffixes(const QString &header, const QString &source,  const QString &form= QString());

    QtProjectParameters parameters() const;
    LibraryParameters libraryParameters() const;

public slots:
    void setPath(const QString &path);
    void setName(const QString &name);

private slots:
    void slotCurrentIdChanged(int);

private:
    LibraryIntroPage *m_introPage;
    ModulesPage *m_modulesPage;
    FilesPage *m_filesPage;
    bool m_pluginBaseClassesInitialized;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // LIBRARYWIZARDDIALOG_H
