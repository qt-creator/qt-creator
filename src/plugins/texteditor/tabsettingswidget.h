// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QSpinBox;
QT_END_NAMESPACE

namespace TextEditor {

class TabSettings;

class TEXTEDITOR_EXPORT TabSettingsWidget : public QGroupBox
{
    Q_OBJECT

public:
    enum CodingStyleLink {
        CppLink,
        QtQuickLink
    };

    explicit TabSettingsWidget(QWidget *parent = nullptr);
    ~TabSettingsWidget() override;

    TabSettings tabSettings() const;

    void setCodingStyleWarningVisible(bool visible);
    void setTabSettings(const TextEditor::TabSettings& s);

signals:
    void settingsChanged(const TextEditor::TabSettings &);
    void codingStyleLinkClicked(TextEditor::TabSettingsWidget::CodingStyleLink link);

private:
    void slotSettingsChanged();
    void codingStyleLinkActivated(const QString &linkString);

    QLabel *m_codingStyleWarning;
    QComboBox *m_tabPolicy;
    QSpinBox *m_tabSize;
    QSpinBox *m_indentSize;
    QComboBox *m_continuationAlignBehavior;
};

} // namespace TextEditor
