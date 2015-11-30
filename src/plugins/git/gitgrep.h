/****************************************************************************
**
** Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
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

#ifndef GITGREP_H
#define GITGREP_H

#include <texteditor/basefilefind.h>

#include <utils/fileutils.h>

#include <QPointer>
#include <QStringListModel>

namespace Utils {
class FancyLineEdit;
class PathChooser;
}

namespace Git {
namespace Internal {

class GitGrep : public TextEditor::BaseFileFind
{
    Q_OBJECT

public:
    QString id() const override;
    QString displayName() const override;
    QFuture<Utils::FileSearchResultList> executeSearch(
            const TextEditor::FileFindParameters &parameters) override;
    QWidget *createConfigWidget() override;
    void writeSettings(QSettings *settings) override;
    void readSettings(QSettings *settings) override;
    bool isValid() const override;

    void setDirectory(const Utils::FileName &directory);

protected:
    Utils::FileIterator *files(const QStringList &nameFilters,
                               const QVariant &additionalParameters) const override;
    QVariant additionalParameters() const override;
    QString label() const override;
    QString toolTip() const override;

private:
    Utils::FileName path() const;
    bool validateDirectory(Utils::FancyLineEdit *edit, QString *errorMessage) const;

    QPointer<QWidget> m_configWidget;
    QPointer<Utils::PathChooser> m_directory;
};

} // namespace Internal
} // namespace Git

#endif // FINDINFILES_H
