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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_BLACKBERRY_CASCADESPROJECTIMPORT_BARDESCRIPTORCONVERTER_H
#define QNX_BLACKBERRY_CASCADESPROJECTIMPORT_BARDESCRIPTORCONVERTER_H

#include "fileconverter.h"

#include <QCoreApplication>

QT_BEGIN_NAMESPACE
class QDomElement;
class QDomDocument;
QT_END_NAMESPACE

namespace Qnx {
namespace Internal {

class BarDescriptorConverter : public FileConverter
{
    Q_DECLARE_TR_FUNCTIONS(BarDescriptorConverter);
public:
    BarDescriptorConverter(ConvertedProjectContext &ctx);
    virtual ~BarDescriptorConverter() {}

    bool convertFile(Core::GeneratedFile &file, QString &errorMessage);
private:
    QString projectName() const;
    QString applicationBinaryName() const;
    QString applicationBinaryPath() const;

    QDomElement findElement(QDomDocument &doc, const QString &tagName,
                            const QString &attributeName, const QString &attributeValue);
    QDomElement ensureElement(QDomDocument &doc, const QString &tagName,
                              const QString &attributeName, const QString &attributeValue);
    QDomElement removeElement(QDomDocument &doc, const QString &tagName,
                              const QString &attributeName, const QString &attributeValue);

    void fixImageAsset(QDomDocument &doc, const QString &definitionElementName);
    void fixIconAsset(QDomDocument &doc);
    void fixSplashScreensAsset(QDomDocument &doc);

    void setEnv(QDomDocument &doc, const QString &name, const QString &value);
    void setAsset(QDomDocument &doc, const QString &srcPath, const QString &destPath, const QString &type = QString(), bool isEntry = false);
    void removeAsset(QDomDocument &doc, const QString &srcPath);
    void replaceAssetSourcePath(QDomDocument &doc, const QString &oldSrcPath, const QString &newSrcPath);
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_BLACKBERRY_CASCADESPROJECTIMPORT_BARDESCRIPTORCONVERTER_H
