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

#ifndef FORMCLASSWIZARDPARAMETERS_H
#define FORMCLASSWIZARDPARAMETERS_H

#include <QtCore/QString>
#include <QtCore/QMetaType>

namespace Designer {
/* Parameters passed to generate the code part of a form class.
 * Code generation using the parameters is provided as an invokable
 * service by QtDesignerFormClassCodeGenerator.
 * Thus, the parameters class must not have linkage. */

class FormClassWizardParameters
{
public:
    QString uiTemplate;
    QString className;
    QString path;
    QString sourceFile;
    QString headerFile;
    QString uiFile;
};
} // namespace Designer

Q_DECLARE_METATYPE(Designer::FormClassWizardParameters)

#endif // FORMCLASSWIZARDPARAMETERS_H
