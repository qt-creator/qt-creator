// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "wizardpage.h"

namespace Utils {

class FilePath;

class QTCREATOR_UTILS_EXPORT FileWizardPage : public WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName DESIGNABLE true)

public:
    explicit FileWizardPage(QWidget *parent = nullptr);
    ~FileWizardPage() override;

    QString fileName() const;
    QString path() const; // Deprecated: Use filePath()

    Utils::FilePath filePath() const;

    bool isComplete() const override;

    void setFileNameLabel(const QString &label);
    void setPathLabel(const QString &label);
    void setDefaultSuffix(const QString &suffix);

    bool forceFirstCapitalLetterForFileName() const;
    void setForceFirstCapitalLetterForFileName(bool b);
    void setAllowDirectoriesInFileSelector(bool allow);

    // Validate a base name entry field (potentially containing extension)
    static bool validateBaseName(const QString &name, QString *errorMessage = nullptr);

signals:
    void activated();
    void pathChanged();

public slots:
    void setPath(const QString &path); // Deprecated: Use setFilePath
    void setFileName(const QString &name);
    void setFilePath(const Utils::FilePath &filePath);

private:
    void slotValidChanged();
    void slotActivated();

    class FileWizardPagePrivate *d;
};

} // namespace Utils
