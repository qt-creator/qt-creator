/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_ITOOLATTRIBUTESETTINGSDATAITEM_H
#define VCPROJECTMANAGER_INTERNAL_ITOOLATTRIBUTESETTINGSDATAITEM_H

class QString;

namespace VcProjectManager {
namespace Internal {

class ToolAttributeOption;
class IToolAttribute;

class IAttributeDescriptionDataItem
{
public:
    virtual ~IAttributeDescriptionDataItem() {}
    virtual QString descriptionText() const = 0;
    virtual QString displayText() const = 0;
    virtual QString key() const = 0;
    virtual ToolAttributeOption* firstOption() const = 0;
    virtual void setFirstOption(ToolAttributeOption *opt) = 0;
    virtual QString defaultValue() const = 0;
    virtual IToolAttribute* createAttribute() const = 0;
    virtual QString optionalValue(const QString &key) const = 0;
    virtual void setOptionalValue(const QString &key, const QString &value) = 0;
    virtual void removeOptionalValue(const QString &key) = 0;
    virtual QString type() const = 0;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_ITOOLATTRIBUTESETTINGSDATAITEM_H
