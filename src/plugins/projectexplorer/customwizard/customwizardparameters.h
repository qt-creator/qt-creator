/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CUSTOMPROJECTWIZARDPARAMETERS_H
#define CUSTOMPROJECTWIZARDPARAMETERS_H

#include <coreplugin/basefilewizard.h>

#include <QtCore/QList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QIODevice;
class QDebug;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

struct CustomWizardField {
    // Parameters of the widget control are stored as map
    typedef QMap<QString, QString> ControlAttributeMap;
    CustomWizardField();
    void clear();

    QString description;
    QString name;
    ControlAttributeMap controlAttributes;
    bool mandatory;
};

struct CustomWizardFile {
    QString source;
    QString target;
};

struct CustomWizardParameters
{
public:
    CustomWizardParameters();
    void clear();
    bool parse(QIODevice &device, const QString &configFileFullPath,
               Core::BaseFileWizardParameters *bp, QString *errorMessage);
    bool parse(const QString &configFileFullPath,
               Core::BaseFileWizardParameters *bp, QString *errorMessage);

    QString directory;
    QString klass;
    QList<CustomWizardFile> files;
    QString fieldPageTitle;
    QList<CustomWizardField> fields;
    int firstPageId;
};

QDebug operator<<(QDebug d, const CustomWizardField &);
QDebug operator<<(QDebug d, const CustomWizardFile &);
QDebug operator<<(QDebug d, const CustomWizardParameters &);

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMPROJECTWIZARDPARAMETERS_H
