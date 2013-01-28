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

#include "formtemplatewizardpage.h"
#include "formeditorw.h"
#include "designerconstants.h"

#if QT_VERSION >= 0x050000
#    include <QDesignerNewFormWidgetInterface>
#else
#    include "qt_private/abstractnewformwidget_p.h"
#endif

#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QByteArray>
#include <QBuffer>

#include <QVBoxLayout>
#include <QMessageBox>
#include <QAbstractButton>

#ifdef USE_XSLT
#    include <QXmlQuery>
#else
#    include <QDomDocument>
#endif

namespace Designer {
namespace Internal {

// ----------------- FormTemplateWizardPage

FormTemplateWizardPage::FormTemplateWizardPage(QWidget * parent) :
    QWizardPage(parent),
    m_newFormWidget(QDesignerNewFormWidgetInterface::createNewFormWidget(FormEditorW::instance()->designerEditor())),
    m_templateSelected(m_newFormWidget->hasCurrentTemplate())
{
    setTitle(tr("Choose a Form Template"));
    QVBoxLayout *layout = new QVBoxLayout;

    connect(m_newFormWidget, SIGNAL(currentTemplateChanged(bool)),
            this, SLOT(slotCurrentTemplateChanged(bool)));
    connect(m_newFormWidget, SIGNAL(templateActivated()),
            this, SIGNAL(templateActivated()));
    layout->addWidget(m_newFormWidget);

    setLayout(layout);
}

bool FormTemplateWizardPage::isComplete() const
{
    return m_templateSelected;
}

void FormTemplateWizardPage::slotCurrentTemplateChanged(bool templateSelected)
{
    if (m_templateSelected == templateSelected)
        return;
    m_templateSelected = templateSelected;
    emit completeChanged();
}

bool FormTemplateWizardPage::validatePage()
{
    QString errorMessage;
    m_templateContents = m_newFormWidget->currentTemplate(&errorMessage);
    if (m_templateContents.isEmpty()) {
        QMessageBox::critical(this, tr("%1 - Error").arg(title()), errorMessage);
        return false;
    }
    return true;
}

QString FormTemplateWizardPage::stripNamespaces(const QString &className)
{
    QString rc = className;
    const int namespaceIndex = rc.lastIndexOf(QLatin1String("::"));
    if (namespaceIndex != -1)
        rc.remove(0, namespaceIndex + 2);
    return rc;
}

bool FormTemplateWizardPage::getUIXmlData(const QString &uiXml,
                                              QString *formBaseClass,
                                              QString *uiClassName)
{
    // Parse UI xml to determine
    // 1) The ui class name from "<class>Designer::Internal::FormClassWizardPage</class>"
    // 2) the base class from: widget class="QWizardPage"...
    QXmlStreamReader reader(uiXml);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("class")) {
                *uiClassName = reader.readElementText();
            } else {
                if (reader.name() == QLatin1String("widget")) {
                    const QXmlStreamAttributes attrs = reader.attributes();
                    *formBaseClass = reader.attributes().value(QLatin1String("class")).toString();
                    return !uiClassName->isEmpty() && !formBaseClass->isEmpty();
                }
            }
        }
    }
    return false;
}

#ifdef USE_XSLT

// Change the UI class name in UI xml: This occurs several times, as contents
// of the <class> element, as name of the first <widget> element, and possibly
// in the signal/slot connections

static const char *classNameChangingSheetFormatC =
"<?xml version=\"1.0\"?>\n"
"<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"2.0\">\n"
"<xsl:output method=\"xml\" indent=\"yes\" encoding=\"UTF-8\" />\n"
"\n"
"<!-- Grab old class name to be replaced by evaluating <ui><class> -->\n"
"<xsl:param name=\"oldClassName\"><xsl:value-of select=\"/ui/class\"/></xsl:param>\n"
"\n"
"<!-- heavy wizardry to copy nodes and attributes (emulating the default  rules) -->\n"
"<xsl:template match=\"@*|node()\">\n"
"   <xsl:copy>\n"
"        <xsl:apply-templates select=\"@*|node()\"/>\n"
"   </xsl:copy>\n"
"</xsl:template>\n"
"\n"
"<!-- Change <ui><class> tag -->\n"
"<xsl:template match=\"class\">\n"
"    <xsl:element name=\"class\">\n"
"    <xsl:text>%1</xsl:text>\n"
"    </xsl:element>\n"
"</xsl:template>\n"
"\n"
"<!-- Change first <widget> tag -->\n"
"<xsl:template match=\"/ui/widget/@name\">\n"
"<xsl:attribute name=\"name\">\n"
"<xsl:text>%1</xsl:text>\n"
"</xsl:attribute>\n"
"</xsl:template>\n"
"\n"
"<!-- Change <sender>/<receiver> elements (for pre-wired QDialog signals) -->\n"
"<xsl:template match=\"receiver[.='Dialog']\">\n"
"    <xsl:element name=\"receiver\">\n"
"        <xsl:text>%1</xsl:text>\n"
"    </xsl:element>\n"
"</xsl:template>\n"
"<xsl:template match=\"sender[.='Dialog']\">\n"
"    <xsl:element name=\"sender\">\n"
"        <xsl:text>%1</xsl:text>\n"
"    </xsl:element>\n"
"</xsl:template>\n"
"</xsl:stylesheet>\n";

QString FormTemplateWizardPage::changeUiClassName(const QString &uiXml, const QString &newUiClassName)
{
    // Prepare I/O: Sheet
    const QString xsltSheet = QString::fromLatin1(classNameChangingSheetFormatC).arg(newUiClassName);
    QByteArray xsltSheetBA = xsltSheet.toUtf8();
    QBuffer xsltSheetBuffer(&xsltSheetBA);
    xsltSheetBuffer.open(QIODevice::ReadOnly);
    // Prepare I/O: Xml
    QByteArray xmlBA = uiXml.toUtf8();
    QBuffer  xmlBuffer(&xmlBA);
    xmlBuffer.open(QIODevice::ReadOnly);
    // Prepare I/O: output
    QBuffer outputBuffer;
    outputBuffer.open(QIODevice::WriteOnly);

    // Run query
    QXmlQuery query(QXmlQuery::XSLT20);
    query.setFocus(&xmlBuffer);
    query.setQuery(&xsltSheetBuffer);
    if (!query.evaluateTo(&outputBuffer)) {
        qWarning("Unable to change the ui class name in a form template.\n%s\nUsing:\n%s\n",
                 xmlBA.constData(), xsltSheetBA.constData());
        return uiXml;
    }
    outputBuffer.close();
    return QString::fromUtf8(outputBuffer.data());
}
#else

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

QString FormTemplateWizardPage::changeUiClassName(const QString &uiXml, const QString &newUiClassName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << '>' << Q_FUNC_INFO << newUiClassName;
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
    if (Designer::Constants::Internal::debug > 1)
        qDebug() << '<' << Q_FUNC_INFO << newUiClassName << rc;
    return rc;
}
#endif // USE_XSLT

} // namespace Internal
} // namespace Designer
