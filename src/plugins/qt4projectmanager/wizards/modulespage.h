/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef MODULESPAGE_H
#define MODULESPAGE_H

#include <QMap>
#include <QStringList>
#include <QWizard>

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

    QStringList selectedModulesList() const;
    QStringList deselectedModulesList() const;

    void setModuleSelected(const QString &module, bool selected = true) const;
    void setModuleEnabled(const QString &module, bool enabled = true) const;

    // Return the key that goes into the Qt config line for a module
    static QString idOfModule(const QString &module);

private:
    QMap<QString, QCheckBox*> m_moduleCheckBoxMap;
    QStringList modules(bool selected = true) const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MODULESPAGE_H
