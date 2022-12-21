// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "fancylineedit.h"

namespace Utils {

struct ClassNameValidatingLineEditPrivate;

class QTCREATOR_UTILS_EXPORT ClassNameValidatingLineEdit : public FancyLineEdit
{
    Q_OBJECT
    Q_PROPERTY(bool namespacesEnabled READ namespacesEnabled WRITE setNamespacesEnabled DESIGNABLE true)
    Q_PROPERTY(bool lowerCaseFileName READ lowerCaseFileName WRITE setLowerCaseFileName)

public:
    explicit ClassNameValidatingLineEdit(QWidget *parent = nullptr);
    ~ClassNameValidatingLineEdit() override;

    bool namespacesEnabled() const;
    void setNamespacesEnabled(bool b);

    QString namespaceDelimiter();
    void setNamespaceDelimiter(const QString &delimiter);

    bool lowerCaseFileName() const;
    void setLowerCaseFileName(bool v);

    bool forceFirstCapitalLetter() const;
    void setForceFirstCapitalLetter(bool b);

    // Clean an input string to get a valid class name.
    static QString createClassName(const QString &name);

signals:
    // Will be emitted with a suggestion for a base name of the
    // source/header file of the class.
    void updateFileName(const QString &t);

protected:
    bool validateClassName(FancyLineEdit *edit, QString *errorMessage) const;
    void handleChanged(const QString &t) override;
    QString fixInputString(const QString &string) override;

private:
    void updateRegExp() const;

    ClassNameValidatingLineEditPrivate *d;
};

} // namespace Utils
