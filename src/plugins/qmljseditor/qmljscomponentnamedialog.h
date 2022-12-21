// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QListWidget;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils {
class ClassNameValidatingLineEdit;
class PathChooser;
}

namespace QmlJSEditor {
namespace Internal {

class ComponentNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComponentNameDialog(QWidget *parent = nullptr);

    static bool go(QString *proposedName, QString *proposedPath, QString *proposedSuffix,
                   const QStringList &properties, const QStringList &sourcePreview, const QString &oldFileName,
                   QStringList *result,
                   QWidget *parent = nullptr);

    void setProperties(const QStringList &properties);

    QStringList propertiesToKeep() const;

    void generateCodePreview();

    void validate();

protected:
    QString isValid() const;

private:
    QStringList m_sourcePreview;

    Utils::ClassNameValidatingLineEdit *m_componentNameEdit;
    QLabel *m_messageLabel;
    Utils::PathChooser *m_pathEdit;
    QLabel *m_label;
    QListWidget *m_listWidget;
    QPlainTextEdit *m_plainTextEdit;
    QCheckBox *m_checkBox;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace QmlJSEditor
