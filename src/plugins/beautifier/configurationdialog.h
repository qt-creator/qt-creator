// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

namespace Beautifier::Internal {

class AbstractSettings;
class ConfigurationEditor;

class ConfigurationDialog : public QDialog
{
public:
    explicit ConfigurationDialog(QWidget *parent = nullptr);
    ~ConfigurationDialog() override;
    void setSettings(AbstractSettings *settings);

    void clear();
    QString key() const;
    void setKey(const QString &key);
    QString value() const;

private:
    void updateOkButton();
    void updateDocumentation(const QString &word = QString(), const QString &docu = QString());

    AbstractSettings *m_settings = nullptr;
    QString m_currentKey;

    QLineEdit *m_name;
    ConfigurationEditor *m_editor;
    QLabel *m_documentationHeader;
    QTextEdit *m_documentation;
    QDialogButtonBox *m_buttonBox;
};

} // Beautifier::Internal
