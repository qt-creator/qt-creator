// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

namespace QmlJSEditor {
namespace Internal {

namespace Ui { class ComponentNameDialog; }

class ComponentNameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComponentNameDialog(QWidget *parent = nullptr);
    ~ComponentNameDialog() override;

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
    Ui::ComponentNameDialog *ui;
    QStringList m_sourcePreview;
};

} // namespace Internal
} // namespace QmlJSEditor
