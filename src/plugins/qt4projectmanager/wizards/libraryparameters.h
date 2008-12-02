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

#ifndef LIBRARYPARAMETERS_H
#define LIBRARYPARAMETERS_H

#include "qtprojectparameters.h"

#include <QtCore/QString>

namespace Qt4ProjectManager {
namespace Internal {

// Additional parameters required besides QtProjectParameters for creating
// libraries
struct LibraryParameters {

    // generate class
    void generateCode(QtProjectParameters:: Type t,
                      const QString &projectTarget,
                      const QString &headerName,
                      const QString &sharedHeader,
                      const QString &exportMacro,
                      int indentation,
                      QString *header,
                      QString *source) const;

    // Generate the code of the shared header containing the export macro
    static QString generateSharedHeader(const QString &globalHeaderFileName,
                                        const QString &projectTarget,
                                        const QString &exportMacro);

    QString className;
    QString baseClassName;
    QString sourceFileName;
    QString headerFileName;
    QString baseClassModule;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // LIBRARYPARAMETERS_H
