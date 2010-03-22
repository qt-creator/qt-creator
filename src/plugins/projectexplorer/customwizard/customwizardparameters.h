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
    QString toString() const;

    QString directory;
    QString klass;
    QList<CustomWizardFile> files;
    QString fieldPageTitle;
    QList<CustomWizardField> fields;
    int firstPageId;
};

// Context used for one wizard run, shared between CustomWizard
// and the CustomWizardPage as it is used for the QLineEdit-type fields'
// default texts as well. Contains basic replacement fields
// like  '%CppSourceSuffix%', '%CppHeaderSuffix%' (settings-dependent)
// reset() should be called before each wizard run to refresh them.
// CustomProjectWizard additionally inserts '%ProjectName%' from
// the intro page to have it available for default texts.

struct CustomWizardContext {
    typedef QMap<QString, QString> FieldReplacementMap;

    void reset();

    // Replace field values delimited by '%' with special modifiers:
    // %Field% -> simple replacement
    // %Field:l% -> lower case replacement, 'u' upper case,
    // 'c' capitalize first letter.
    static void replaceFields(const FieldReplacementMap &fm, QString *s);

    FieldReplacementMap baseReplacements;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMPROJECTWIZARDPARAMETERS_H
