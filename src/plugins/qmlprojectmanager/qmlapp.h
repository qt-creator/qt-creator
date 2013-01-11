/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLAPP_H
#define QMLAPP_H

#include <QString>
#include <QHash>
#include <QObject>

#include <coreplugin/generatedfile.h>

namespace QmlProjectManager {
namespace Internal {

class TemplateInfo
{
public:
    TemplateInfo();

    QString templateName;
    QString templatePath;
    QString displayName;
    QString description;
    QString openFile;
    QString wizardId;
    QString featuresRequired;

    int priority;
};

class QmlApp : public QObject
{
    Q_OBJECT
public:

    QmlApp(QObject *parent = 0);
    ~QmlApp();

    QString mainQmlFileName() const;
    QString projectDirectory() const;
    QString projectName() const;
    const TemplateInfo &templateInfo() const;
    QString templateDirectory() const;

    static QString templateRootDirectory();
    static QStringList templateNames();
    static QList<TemplateInfo> templateInfos();

    Core::GeneratedFiles generateFiles(QString *errorMessage);
    QString renameQmlFile(const QString &fileName);

    void setTemplateInfo(const TemplateInfo &templateInfo);

    QString creatorFileName() const;

public slots:
    void setProjectNameAndBaseDirectory(const QString &projectName, const QString &projectBaseDirectory);

protected:
    QString readAndAdaptTemplateFile(const QString &filePath, bool &ok) const;
    bool addTemplate(const QString &sourceDirectory,
                     const QString &sourceFileName,
                     const QString &targetDirectory,
                     const QString &targetFileName,
                     Core::GeneratedFiles *files,
                     QString *errorMessage) const;
    bool addBinaryFile(const QString &sourceDirectory,
                       const QString &sourceFileName,
                       const QString &targetDirectory,
                       const QString &targetFileName,
                       Core::GeneratedFiles *files,
                       QString *errorMessage) const;
    QByteArray readFile(const QString &filePath, bool &ok) const;

private:
    void setReplacementVariables();

    QString m_runtime;

    QString m_projectName;
    QString m_projectBaseDirectory;
    bool m_grantUserDataAccess;
    QHash<QString, QString> m_replacementVariables;
    TemplateInfo m_templateInfo;
    QString m_creatorFileName;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLAPP_H
