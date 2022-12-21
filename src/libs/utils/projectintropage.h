// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "infolabel.h"
#include "wizardpage.h"

namespace Utils {

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

    class ProjectIntroPagePrivate *d;
};

} // namespace Utils
