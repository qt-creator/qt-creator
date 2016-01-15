/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "codegenerator.h"

#include "codegensettings.h"

#include <coreplugin/icore.h>

#include <utils/codegeneration.h>
#include <utils/qtcassert.h>

#include <QDomDocument>
#include <QSettings>
#include <QTextStream>
#include <QXmlStreamReader>

namespace QtSupport {

// Change the contents of a DOM element to a new value if it matches
// a predicate
template <class Predicate>
bool changeDomElementContents(const QDomElement &element,
                              Predicate p,
                              const QString &newValue,
                              QString *ptrToOldValue = 0)
{
    // Find text in "<element>text</element>"
    const QDomNodeList children = element.childNodes();
    if (children.size() != 1)
        return false;
    const QDomNode first = children.at(0);
    if (first.nodeType() != QDomNode::TextNode)
        return false;
    QDomCharacterData data = first.toCharacterData();
    const QString oldValue = data.data();

    if (p(oldValue)) {
        if (ptrToOldValue)
            *ptrToOldValue = oldValue;
        data.setData(newValue);
        return true;
    }
    return false;
}

namespace {
bool truePredicate(const QString &) { return true; }

// Predicate that matches a string value
class MatchPredicate {
public:
    MatchPredicate(const QString &m) : m_match(m) {}
    bool operator()(const QString &s) const { return s == m_match; }
private:
    const QString m_match;
};

// Change <sender> and <receiver> in a Dom UI <connections> list
// if they match the class name passed on
void changeDomConnectionList(const QDomElement &connectionsNode,
                             const QString &oldClassName,
                             const QString &newClassName)
{
    const MatchPredicate oldClassPredicate(oldClassName);
    const QString senderTag = QLatin1String("sender");
    const QString receiverTag = QLatin1String("receiver");
    const QDomNodeList connections = connectionsNode.childNodes();
    const int connectionsCount =  connections.size();
    // Loop <connection>
    for (int c = 0; c < connectionsCount; c++) {
        const QDomNodeList connectionElements = connections.at(c).childNodes();
        const int connectionElementCount = connectionElements.count();
        // Loop <sender>, <receiver>, <signal>, <slot>
        for (int ce = 0; ce < connectionElementCount; ce++) {
            const QDomNode connectionElementNode = connectionElements.at(ce);
            if (connectionElementNode.isElement()) {
                const QDomElement connectionElement = connectionElementNode.toElement();
                const QString tagName =  connectionElement.tagName();
                if (tagName == senderTag || tagName == receiverTag)
                    changeDomElementContents(connectionElement, oldClassPredicate, newClassName);
            }
        }
    }
}
}

// Change the UI class name in UI xml: This occurs several times, as contents
// of the <class> element, as name of the first <widget> element, and possibly
// in the signal/slot connections

QString CodeGenerator::changeUiClassName(const QString &uiXml, const QString &newUiClassName)
{
    QDomDocument domUi;
    if (!domUi.setContent(uiXml)) {
        qWarning("Failed to parse:\n%s", uiXml.toUtf8().constData());
        return uiXml;
    }

    bool firstWidgetElementFound = false;
    QString oldClassName;

    // Loop first level children. First child is <ui>
    const QDomNodeList children = domUi.firstChildElement().childNodes();
    const QString classTag = QLatin1String("class");
    const QString widgetTag = QLatin1String("widget");
    const QString connectionsTag = QLatin1String("connections");
    const int count = children.size();
    for (int i = 0; i < count; i++) {
        const QDomNode node = children.at(i);
        if (node.isElement()) {
            // Replace <class> element text
            QDomElement element = node.toElement();
            const QString name = element.tagName();
            if (name == classTag) {
                if (!changeDomElementContents(element, truePredicate, newUiClassName, &oldClassName)) {
                    qWarning("Unable to change the <class> element:\n%s", uiXml.toUtf8().constData());
                    return uiXml;
                }
            } else {
                // Replace first <widget> element name attribute
                if (!firstWidgetElementFound && name == widgetTag) {
                    firstWidgetElementFound = true;
                    const QString nameAttribute = QLatin1String("name");
                    if (element.hasAttribute(nameAttribute))
                        element.setAttribute(nameAttribute, newUiClassName);
                } else {
                    // Replace <sender>, <receiver> tags of dialogs.
                    if (name == connectionsTag)
                        changeDomConnectionList(element, oldClassName, newUiClassName);
                }
            }
        }
    }
    const QString rc = domUi.toString();
    return rc;
}

bool CodeGenerator::uiData(const QString &uiXml, QString *formBaseClass, QString *uiClassName)
{
    // Parse UI xml to determine
    // 1) The ui class name from "<class>Designer::Internal::FormClassWizardPage</class>"
    // 2) the base class from: widget class="QWizardPage"...
    QXmlStreamReader reader(uiXml);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("class")) {
                *uiClassName = reader.readElementText();
            } else if (reader.name() == QLatin1String("widget")) {
                const QXmlStreamAttributes attrs = reader.attributes();
                *formBaseClass = attrs.value(QLatin1String("class")).toString();
                return !uiClassName->isEmpty() && !formBaseClass->isEmpty();
            }
        }
    }
    return false;
}

QString CodeGenerator::uiClassName(const QString &uiXml)
{
    QString base;
    QString klass;
    QTC_ASSERT(uiData(uiXml, &base, &klass), return QString());
    return klass;
}

QString CodeGenerator::qtIncludes(const QStringList &qt4, const QStringList &qt5)
{
    CodeGenSettings settings;
    settings.fromSettings(Core::ICore::settings());

    QString result;
    QTextStream str(&result);
    Utils::writeQtIncludeSection(qt4, qt5, settings.addQtVersionCheck, settings.includeQtModule, str);
    return result;
}

} // namespace QtSupport
