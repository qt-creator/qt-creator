/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef MODULESPAGE_H
#define MODULESPAGE_H

#include <QtCore/QMap>
#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {

class ModulesPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit ModulesPage(QWidget* parent = 0);
    QString selectedModules() const;
    QString deselectedModules() const;
    void setModuleSelected(const QString &module, bool selected = true) const;
    void setModuleEnabled(const QString &module, bool enabled = true) const;

    // Return the key that goes into the Qt config line for a module
    static QString idOfModule(const QString &module);

private:
    QMap<QString, QCheckBox*> m_moduleCheckBoxMap;
    QString modules(bool selected = true) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MODULESPAGE_H
