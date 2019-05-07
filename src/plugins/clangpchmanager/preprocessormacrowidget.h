/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QPushButton>
#include <QTreeView>

#include <memory>

namespace Utils {
class DetailsWidget;
class NameValueModel;
} // namespace Utils

namespace ClangPchManager {
class ClangIndexingProjectSettings;
using PreprocessorMacro = std::pair<QString, QString>;
using PreprocessorMacros = QVector<PreprocessorMacro>;

class PreprocessorMacroWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PreprocessorMacroWidget(QWidget *parent = nullptr);
    ~PreprocessorMacroWidget() override;
    void setBasePreprocessorMacros(const PreprocessorMacros &macros);
    void setSettings(ClangIndexingProjectSettings *settings);

signals:
    void userChangesChanged();

private:
    void updateButtons();
    void focusIndex(const QModelIndex &index);
    void invalidateCurrentIndex();
    void editButtonClicked();
    void addButtonClicked();
    void removeButtonClicked();
    void unsetButtonClicked();
    void currentIndexChanged(const QModelIndex &current);
    void linkActivated(const QString &link);
    void updateSummaryText();
    void saveSettings();

private:
    std::unique_ptr<Utils::NameValueModel> m_model;
    Utils::DetailsWidget *m_detailsContainer;
    QTreeView *m_preprocessorMacrosView;
    QPushButton *m_editButton;
    QPushButton *m_addButton;
    QPushButton *m_resetButton;
    QPushButton *m_unsetButton;
    ClangIndexingProjectSettings *m_settings = {};
};

} // namespace ClangPchManager
