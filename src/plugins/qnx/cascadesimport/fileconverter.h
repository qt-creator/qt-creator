/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry Limited (blackberry-qt@qnx.com)
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

#ifndef QNX_BLACKBERRY_CASCADESPROJECTIMPORT_FILECONVERTER_H
#define QNX_BLACKBERRY_CASCADESPROJECTIMPORT_FILECONVERTER_H

#include "importlog.h"

#include <QStringList>
#include <QCoreApplication>

namespace Core { class GeneratedFile; }

namespace Qnx {
namespace Internal {

class ConvertedProjectContext
{
public:
    void setSrcProjectPath(const QString &p) {m_srcProjectPath = p;}
    QString srcProjectPath() const {return m_srcProjectPath;}
    void setDestProjectPath(const QString &p) {m_destProjectPath = p;}
    QString destProjectPath() const {return m_destProjectPath;}

    QString projectName() const;
    const QStringList& collectedFiles() const {return m_collectedFiles;}
    void setCollectedFiles(const QStringList &files) {m_collectedFiles = files;}
    ImportLog& importLog() {return m_importLog;}
private:
    QString m_srcProjectPath;
    QString m_destProjectPath;
    ImportLog m_importLog;
    QStringList m_collectedFiles;
};

class FileConverter
{
    Q_DECLARE_TR_FUNCTIONS(FileConverter);
public:
    FileConverter(ConvertedProjectContext &ctx) : m_convertedProjectContext(ctx) {}
    virtual ~FileConverter() {}

    virtual bool convertFile(Core::GeneratedFile &file, QString &errorMessage);
protected:
    ConvertedProjectContext& convertedProjectContext() const {return m_convertedProjectContext;}

    bool loadFileContent(Core::GeneratedFile &file, QString &errorMessage);
    QByteArray loadFileContent(const QString &filePath, QString &errorMessage);
    void logError(const QString &errorMessage);

    ConvertedProjectContext &m_convertedProjectContext;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_BLACKBERRY_CASCADESPROJECTIMPORT_FILECONVERTER_H
