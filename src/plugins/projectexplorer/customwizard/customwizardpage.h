// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QComboBox>
#include <QCheckBox>
#include <QWizardPage>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QTextEdit;
class QLabel;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer {
namespace Internal {

class CustomWizardField;
class CustomWizardParameters;
class CustomWizardContext;

// Documentation inside.
class CustomWizardFieldPage : public QWizardPage {
    Q_OBJECT
public:
    using FieldList = QList<CustomWizardField>;

    explicit CustomWizardFieldPage(const QSharedPointer<CustomWizardContext> &ctx,
                                   const QSharedPointer<CustomWizardParameters> &parameters,
                                   QWidget *parent = nullptr);

    bool validatePage() override;
    void initializePage() override;
    void cleanupPage() override;

    static QMap<QString, QString> replacementMap(const QWizard *w,
                                                 const QSharedPointer<CustomWizardContext> &ctx,
                                                 const FieldList &f);

protected:
    inline void addRow(const QString &name, QWidget *w);
    void showError(const QString &);
    void clearError();
private:
    class LineEditData {
    public:
        explicit LineEditData(QLineEdit *le = nullptr, const QString &defText = QString(), const QString &pText = QString());
        QLineEdit *lineEdit;
        QString defaultText;
        QString placeholderText;
        QString userChange;
    };
    class TextEditData {
    public:
        explicit TextEditData(QTextEdit *le = nullptr, const QString &defText = QString());
        QTextEdit *textEdit;
        QString defaultText;
        QString userChange;
    };
    class PathChooserData {
    public:
        explicit PathChooserData(Utils::PathChooser *pe = nullptr, const QString &defText = QString());
        Utils::PathChooser *pathChooser;
        QString defaultText;
        QString userChange;
    };

    using LineEditDataList = QList<LineEditData>;
    using TextEditDataList = QList<TextEditData>;
    using PathChooserDataList = QList<PathChooserData>;

    QWidget *registerLineEdit(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerComboBox(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerTextEdit(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerPathChooser(const QString &fieldName, const CustomWizardField &field);
    QWidget *registerCheckBox(const QString &fieldName,
                              const QString &fieldDescription,
                              const CustomWizardField &field);
    void addField(const CustomWizardField &f);

    const QSharedPointer<CustomWizardParameters> m_parameters;
    const QSharedPointer<CustomWizardContext> m_context;
    QFormLayout *m_formLayout;
    LineEditDataList m_lineEdits;
    TextEditDataList m_textEdits;
    PathChooserDataList m_pathChoosers;
    QLabel *m_errorLabel;
};

// Documentation inside.
class CustomWizardPage : public CustomWizardFieldPage
{
    Q_OBJECT

public:
    explicit CustomWizardPage(const QSharedPointer<CustomWizardContext> &ctx,
                              const QSharedPointer<CustomWizardParameters> &parameters,
                              QWidget *parent = nullptr);

    Utils::FilePath filePath() const;
    void setFilePath(const Utils::FilePath &path);

    bool isComplete() const override;

private:
    Utils::PathChooser *m_pathChooser;
};

} // namespace Internal
} // namespace ProjectExplorer
