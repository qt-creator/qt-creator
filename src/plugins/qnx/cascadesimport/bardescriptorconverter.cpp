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

#include "bardescriptorconverter.h"

#include <coreplugin/generatedfile.h>
#include <utils/qtcassert.h>

#include <QDomDocument>

namespace Qnx {
namespace Internal {

//////////////////////////////////////////////////////////////////////////////
//
// BarDescriptorConverter
//
//////////////////////////////////////////////////////////////////////////////
static const char S_ASSET[] = "asset";
static const char S_PATH[] = "path";
static const char S_SRC_DIR[] = "%SRC_DIR%/";

BarDescriptorConverter::BarDescriptorConverter(ConvertedProjectContext &ctx)
    : FileConverter(ctx)
{
}

QString BarDescriptorConverter::projectName() const
{
    return convertedProjectContext().projectName();
}

QString BarDescriptorConverter::applicationBinaryName() const
{
    return projectName();
}

QString BarDescriptorConverter::applicationBinaryPath() const
{
    return projectName();
}

QDomElement BarDescriptorConverter::findElement(QDomDocument &doc, const QString &tagName,
        const QString &attributeName, const QString &attributeValue)
{
    QDomElement ret;
    QTC_ASSERT(!tagName.isEmpty(), return ret);
    QDomElement rootElement = doc.documentElement();
    static const QLatin1String elementTextFakeAttributeNameString("S_ELEMENT_TEXT_FAKE_ATTRIBUTE_NAME");
    bool isFindText = (attributeName == elementTextFakeAttributeNameString);
    QRegExp rxAttrValue;
    if (!isFindText && !attributeValue.isEmpty())
        rxAttrValue = QRegExp(attributeValue, Qt::CaseSensitive, QRegExp::Wildcard);
    for (QDomElement el = rootElement.firstChildElement(tagName);
            !el.isNull(); el = el.nextSiblingElement(tagName)) {
        if (attributeName.isEmpty()) {
            // take first matching tag name
            ret = el;
            break;
        } else if (isFindText) {
            QString s = el.text();
            if (s == attributeValue) {
                ret = el;
                break;
            }
        } else if (el.hasAttribute(attributeName)) {
            if (attributeValue.isEmpty() || rxAttrValue.exactMatch(el.attribute(attributeName))) {
                ret = el;
                break;
            }
        }
    }
    return ret;
}

QDomElement BarDescriptorConverter::ensureElement(QDomDocument &doc, const QString &tagName,
            const QString &attributeName, const QString &attributeValue)
{
    QDomElement ret = findElement(doc, tagName, attributeName, attributeValue);
    if (ret.isNull()) {
        QDomElement rootElement = doc.documentElement();
        ret = rootElement.appendChild(doc.createElement(tagName)).toElement();
        QTC_ASSERT(!ret.isNull(), return ret);
    }
    if (!attributeName.isEmpty())
        ret.setAttribute(attributeName, attributeValue);
    return ret;
}

QDomElement BarDescriptorConverter::removeElement(QDomDocument &doc, const QString &tagName,
            const QString &attributeName, const QString &attributeValue)
{
    QDomElement ret = findElement(doc, tagName, attributeName, attributeValue);
    if (!ret.isNull()) {
        QDomNode nd = ret.parentNode();
        QTC_ASSERT(!nd.isNull(), return ret);
        nd.removeChild(ret);
    }
    return ret;
}

void BarDescriptorConverter::setEnv(QDomDocument &doc, const QString &name, const QString &value)
{
    QDomElement el = ensureElement(doc, QLatin1String("env"), QLatin1String("var"), name);
    QTC_ASSERT(!el.isNull(), return);
    el.setAttribute(QString::fromLatin1("value"), value);
}

void BarDescriptorConverter::setAsset(QDomDocument &doc, const QString &srcPath,
        const QString &destPath, const QString &type, bool isEntry)
{
    ImportLog &log = convertedProjectContext().importLog();
    log.logInfo(tr("Setting asset path: %1 to %2 type: %3 entry point: %4")
        .arg(srcPath).arg(destPath).arg(type).arg(isEntry));
    QDomElement assetElement = ensureElement(doc, QLatin1String(S_ASSET), QLatin1String(S_PATH), srcPath);
    QTC_ASSERT(!assetElement.isNull(), return);
    while (assetElement.hasChildNodes()) {
        QDomNode nd = assetElement.firstChild();
        assetElement.removeChild(nd);
    }
    assetElement.appendChild(doc.createTextNode(destPath));

    const QString typeString = QLatin1String("type");
    QString s = assetElement.attribute(typeString);
    if (s != type)
        assetElement.setAttribute(typeString, type);

    const QString entryString = QLatin1String("entry");
    s = assetElement.attribute(entryString);
    bool b = (s.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0)
        || (s.compare(QLatin1String("1")) == 0);
    if (b != isEntry)
        assetElement.setAttribute(entryString, QVariant(isEntry).toString());
}

void BarDescriptorConverter::removeAsset(QDomDocument &doc, const QString &srcPath)
{
    ImportLog &log = convertedProjectContext().importLog();
    log.logInfo(tr("Removing asset path: %1").arg(srcPath));
    removeElement(doc, QLatin1String(S_ASSET), QLatin1String(S_PATH), srcPath);
}

void BarDescriptorConverter::replaceAssetSourcePath(QDomDocument &doc, const QString &oldSrcPath,
        const QString &newSrcPath)
{
    ImportLog &log = convertedProjectContext().importLog();
    QDomElement el = ensureElement(doc, QLatin1String(S_ASSET), QLatin1String(S_PATH), oldSrcPath);
    if (!el.isNull()) {
        log.logInfo(tr("Replacing asset source path: %1 -> %2").arg(oldSrcPath).arg(newSrcPath));
        el.setAttribute(QLatin1String(S_PATH), newSrcPath);
    }
}

void BarDescriptorConverter::fixImageAsset(QDomDocument &doc, const QString &definitionElementName)
{
    ImportLog &log = convertedProjectContext().importLog();
    QString target;
    QDomElement el = findElement(doc, definitionElementName, QString(), QString());
    if (!el.isNull()) {
        const QString imageString = QLatin1String("image");
        for (QDomElement imageElement = el.firstChildElement(imageString); !imageElement.isNull();
                imageElement = imageElement.nextSiblingElement(imageString)) {
            target = imageElement.text();
            if (!target.isEmpty())
                replaceAssetSourcePath(doc, target, QLatin1String(S_SRC_DIR) + target);
        }
    } else {
        log.logWarning(tr("Cannot find image asset definition: <%1>").arg(definitionElementName));
    }
}

void BarDescriptorConverter::fixIconAsset(QDomDocument &doc)
{
    const QString iconString = QString::fromLatin1("icon");
    fixImageAsset(doc, iconString);
}

void BarDescriptorConverter::fixSplashScreensAsset(QDomDocument &doc)
{
    const QString splashScreensString = QString::fromLatin1("splashScreens");
    fixImageAsset(doc, splashScreensString);
}

bool BarDescriptorConverter::convertFile(Core::GeneratedFile &file, QString &errorMessage)
{
    FileConverter::convertFile(file, errorMessage);
    if (errorMessage.isEmpty()) {
        QDomDocument doc;
        if (!doc.setContent(file.binaryContents(), &errorMessage)) {
            errorMessage = tr("Error parsing XML file \"%1\": %2").arg(file.path()).arg(errorMessage);
            return false;
        }

        // remove <configuration> elements, since they are Momentics specific
        QDomElement rootElement = doc.documentElement();
        const QString configurationString = QLatin1String("configuration");
        while (true) {
            QDomElement el = rootElement.firstChildElement(configurationString);
            if (el.isNull())
                break;
            rootElement.removeChild(el);
        }

        // remove obsolete assets
        removeAsset(doc, QLatin1String("translations"));
        removeAsset(doc, QLatin1String("translations/*"));
        // assets
        setAsset(doc, applicationBinaryPath(), applicationBinaryName(),
                 QLatin1String("Qnx/Elf"), true);
        const QString assetsString = QLatin1String("assets");
        replaceAssetSourcePath(doc, assetsString, QLatin1String(S_SRC_DIR) + assetsString);
        fixIconAsset(doc);
        fixSplashScreensAsset(doc);

        file.setBinaryContents(doc.toByteArray(4));
    }
    return errorMessage.isEmpty();
}

} // namespace Internal
} // namespace Qnx
