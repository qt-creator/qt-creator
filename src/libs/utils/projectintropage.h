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

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "infolabel.h"
#include "wizardpage.h"

namespace Utils {

class ProjectIntroPagePrivate;

class QTCREATOR_UTILS_EXPORT ProjectIntroPage : public WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString description READ description WRITE setDescription DESIGNABLE true)
    Q_PROPERTY(Utils::FilePath filePath READ filePath WRITE setFilePath DESIGNABLE true)
    Q_PROPERTY(QString projectName READ projectName WRITE setProjectName DESIGNABLE true)
    Q_PROPERTY(bool useAsDefaultPath READ useAsDefaultPath WRITE setUseAsDefaultPath DESIGNABLE true)
    Q_PROPERTY(bool forceSubProject READ forceSubProject WRITE setForceSubProject DESIGNABLE true)

public:
    explicit ProjectIntroPage(QWidget *parent = nullptr);
    ~ProjectIntroPage() override;

    QString projectName() const;
    FilePath filePath() const;
    QString description() const;
    bool useAsDefaultPath() const;

    // Insert an additional control into the form layout for the target.
    void insertControl(int row, QWidget *label, QWidget *control);

    bool isComplete() const override;

    bool forceSubProject() const;
    void setForceSubProject(bool force);
    void setProjectList(const QStringList &projectList);
    void setProjectDirectories(const FilePaths &directoryList);
    int projectIndex() const;

    bool validateProjectName(const QString &name, QString *errorMessage);

    // Calls slotChanged() - i.e. tell the page that some of its fields have been updated.
    // This function is useful if you programmatically update the fields of the page (i.e. from
    // your client code).
    void fieldsUpdated();

signals:
    void activated();
    void statusMessageChanged(InfoLabel::InfoType type, const QString &message);

public slots:
    void setFilePath(const FilePath &path);
    void setProjectName(const QString &name);
    void setDescription(const QString &description);
    void setUseAsDefaultPath(bool u);
    void setProjectNameRegularExpression(const QRegularExpression &regEx, const QString &userErrorMessage);

private:
    void slotChanged();
    void slotActivated();

    bool validate();
    void displayStatusMessage(InfoLabel::InfoType t, const QString &);
    void hideStatusLabel();

    ProjectIntroPagePrivate *d;
};

} // namespace Utils
