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

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Core {
namespace Internal {

class ExternalTool
{
public:
    enum OutputHandling {
        ShowInPane,
        ReplaceSelection,
        ReloadDocument
    };

    ExternalTool();

    QString id() const;
    QString description() const;
    QString displayName() const;
    QString displayCategory() const;
    int order() const;
    OutputHandling outputHandling() const;

    QStringList executables() const;
    QString arguments() const;
    QString workingDirectory() const;

    static ExternalTool *createFromXml(const QString &xml, QString *errorMessage = 0, const QString &locale = QString());

private:
    QString m_id;
    QString m_description;
    QString m_displayName;
    QString m_displayCategory;
    int m_order;
    QStringList m_executables;
    QString m_arguments;
    QString m_workingDirectory;
    OutputHandling m_outputHandling;
};

} // Internal
} // Core

#endif // EXTERNALTOOL_H
