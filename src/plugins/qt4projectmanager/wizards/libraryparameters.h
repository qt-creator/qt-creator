/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
