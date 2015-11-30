/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTDESIGNERFORMCLASSCODEGENERATOR_H
#define QTDESIGNERFORMCLASSCODEGENERATOR_H

#include <QString>
#include <QVariant>
#include <QObject>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Designer {
class FormClassWizardParameters;

// Publicly registered service to generate the code for a form class
// (See PluginManager::Invoke) to be accessed by Qt4ProjectManager.
class QtDesignerFormClassCodeGenerator : public QObject
{
    Q_OBJECT
public:
    explicit QtDesignerFormClassCodeGenerator(QObject *parent = 0);

    static bool generateCpp(const FormClassWizardParameters &parameters,
                            QString *header, QString *source, int indentation = 4);

    // Generate form class code according to settings.
    Q_INVOKABLE QVariant generateFormClassCode(const Designer::FormClassWizardParameters &parameters);
};
} // namespace Designer

#endif // QTDESIGNERFORMCLASSCODEGENERATOR_H
