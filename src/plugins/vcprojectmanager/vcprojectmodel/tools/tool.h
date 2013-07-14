/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#ifndef VCPROJECTMANAGER_INTERNAL_TOOL_H
#define VCPROJECTMANAGER_INTERNAL_TOOL_H

#include "../ivcprojectnodemodel.h"

#include <QHash>
#include <QString>
#include <QSharedPointer>

namespace VcProjectManager {
namespace Internal {

class Tool : public IVcProjectXMLNode
{
public:
    typedef QSharedPointer<Tool>    Ptr;

    Tool();
    Tool(const Tool &tool);
    Tool& operator= (const Tool& tool);
    ~Tool();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    virtual QString nodeWidgetName() const = 0;
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    QString name() const;
    void setName(const QString &name);
    QString attributeValue(const QString &attributeName) const;
    void setAttribute(const QString &attributeName, const QString &attributeValue);
    void clearAttribute(const QString &attributeName);
    void removeAttribute(const QString &attributeName);
    bool containsAttribute(const QString &attributeName) const;

    /*!
     * Implement this member function to return a clone of a tool
     * \return A shared pointer to a cloned tool object.
     */
    virtual Tool::Ptr clone() const = 0;
protected:
    bool readBooleanAttribute(const QString &attributeName, bool defaultBoolValue) const;
    void setBooleanAttribute(const QString &attributeName, bool value, bool defaultValue);

    QStringList readStringListAttribute(const QString &attributeName, const QString &splitter) const;
    void setStringListAttribute(const QString &attributeName, const QStringList &values, const QString &joinCharacters);

    QString readStringAttribute(const QString &attributeName, const QString &defaultValue) const;
    void setStringAttribute(const QString &attributeName, const QString &attributeValue, const QString &defaultValue);

    template<class T> T readIntegerEnumAttribute(const QString &attributeName, int maximunAttributeValue, int defaultValue) const
    {
        if (containsAttribute(attributeName)) {
            bool ok;
            int val = attributeValue(attributeName).toInt(&ok);
            if (ok && val <= maximunAttributeValue)
                return static_cast<T>(val);
        }

        return static_cast<T>(defaultValue);
    }

    void setIntegerEnumAttribute(const QString &attributeName, int value, int defaultValue);

    QHash<QString, QString> m_anyAttribute;
    QString m_name;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_TOOL_H
