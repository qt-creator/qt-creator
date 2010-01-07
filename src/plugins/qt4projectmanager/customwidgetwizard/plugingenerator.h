/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PLUGINGENERATOR_H
#define PLUGINGENERATOR_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Core {
    class GeneratedFile;
}

namespace Qt4ProjectManager {
namespace Internal {

struct PluginOptions;

struct GenerationParameters {
    QString path;
    QString fileName;
    QString license;
    QString templatePath;
};

class PluginGenerator : public QObject
{
    Q_OBJECT

public:
    static QList<Core::GeneratedFile> generatePlugin(const GenerationParameters& p,
                                                     const PluginOptions &options,
                                                     QString *errorMessage);

private:
    typedef QMap<QString,QString> SubstitutionMap;
    static QString processTemplate(const QString &tmpl, const SubstitutionMap &substMap, QString *errorMessage);
    static QString cStringQuote(QString s);
};

}
}

#endif
