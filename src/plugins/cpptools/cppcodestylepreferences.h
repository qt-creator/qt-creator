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

#ifndef CPPCODESTYLEPREFERENCES_H
#define CPPCODESTYLEPREFERENCES_H

#include "cpptools_global.h"
#include "cppcodestylesettings.h"

#include <texteditor/icodestylepreferences.h>

namespace CppTools {

class CPPTOOLS_EXPORT CppCodeStylePreferences : public TextEditor::ICodeStylePreferences
{
    Q_OBJECT
public:
    explicit CppCodeStylePreferences(
        QObject *parent = 0);

    virtual QVariant value() const;
    virtual void setValue(const QVariant &);

    CppCodeStyleSettings codeStyleSettings() const;

    // tracks parent hierarchy until currentParentSettings is null
    CppCodeStyleSettings currentCodeStyleSettings() const;

    virtual void toMap(const QString &prefix, QVariantMap *map) const;
    virtual void fromMap(const QString &prefix, const QVariantMap &map);

public slots:
    void setCodeStyleSettings(const CppTools::CppCodeStyleSettings &data);

signals:
    void codeStyleSettingsChanged(const CppTools::CppCodeStyleSettings &);
    void currentCodeStyleSettingsChanged(const CppTools::CppCodeStyleSettings &);

protected:
    virtual QString settingsSuffix() const;

private slots:
    void slotCurrentValueChanged(const QVariant &);

private:
    CppCodeStyleSettings m_data;
};

} // namespace CppTools

#endif // CPPCODESTYLEPREFERENCES_H
