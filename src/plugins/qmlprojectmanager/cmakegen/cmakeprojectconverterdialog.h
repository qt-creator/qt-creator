// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#ifndef CMAKEPROJECTCONVERTERDIALOG_H
#define CMAKEPROJECTCONVERTERDIALOG_H

#include <qmlprojectmanager/qmlproject.h>
#include <utils/fancylineedit.h>
#include <utils/filepath.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>

#include <QDialog>

namespace QmlProjectManager {
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
} //QmlProjectManager

#endif // CMAKEPROJECTCONVERTERDIALOG_H
