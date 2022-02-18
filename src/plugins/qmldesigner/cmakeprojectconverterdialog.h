/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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


#ifndef CMAKEPROJECTCONVERTERDIALOG_H
#define CMAKEPROJECTCONVERTERDIALOG_H

#include <qmlprojectmanager/qmlproject.h>
#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>

#include <QDialog>

namespace QmlDesigner {
namespace GenerateCmake {

class CmakeProjectConverterDialog : public QDialog
{
    Q_OBJECT

public:
    CmakeProjectConverterDialog(const QmlProjectManager::QmlProject *oldProject);
    const Utils::FilePath newPath() const;

public slots:
    void pathValidChanged();

private:
    const QString startsWithBlacklisted(const QString &text) const;
    const QString errorText() const;
    const QString uniqueProjectName(const Utils::FilePath &dir, const QString &oldName) const;
    bool isValid();

private:
    Utils::FilePath m_newProjectDir;
    Utils::FancyLineEdit *m_nameEditor;
    Utils::PathChooser *m_dirSelector;
    Utils::InfoLabel *m_errorLabel;
    QPushButton *m_okButton;
};

} //GenerateCmake
} //Qmldesigner

#endif // CMAKEPROJECTCONVERTERDIALOG_H
