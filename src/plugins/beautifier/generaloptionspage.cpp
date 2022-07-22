/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "generaloptionspage.h"

#include "beautifierconstants.h"
#include "generalsettings.h"

#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>

namespace Beautifier::Internal {

class GeneralOptionsPageWidget : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Beautifier::Internal::GeneralOptionsPageWidget)

public:
    explicit GeneralOptionsPageWidget(const QStringList &toolIds);

private:
    void apply() final;

    QCheckBox *m_autoFormat;
    QComboBox *m_autoFormatTool;
    QLineEdit *m_autoFormatMime;
    QCheckBox *m_autoFormatOnlyCurrentProject;
};

GeneralOptionsPageWidget::GeneralOptionsPageWidget(const QStringList &toolIds)
{
    resize(817, 631);

    auto settings = GeneralSettings::instance();

    m_autoFormat = new QCheckBox(tr("Enable auto format on file save"));
    m_autoFormat->setChecked(settings->autoFormatOnSave());

    auto toolLabel = new QLabel(tr("Tool:"));
    toolLabel->setEnabled(false);

    auto mimeLabel = new QLabel(tr("Restrict to MIME types:"));
    mimeLabel->setEnabled(false);

    m_autoFormatMime = new QLineEdit(settings->autoFormatMimeAsString());
    m_autoFormatMime->setEnabled(false);

    m_autoFormatOnlyCurrentProject =
        new QCheckBox(tr("Restrict to files contained in the current project"));
    m_autoFormatOnlyCurrentProject->setEnabled(false);
    m_autoFormatOnlyCurrentProject->setChecked(settings->autoFormatOnlyCurrentProject());

    m_autoFormatTool = new QComboBox;
    m_autoFormatTool->setEnabled(false);
    m_autoFormatTool->addItems(toolIds);
    const int index = m_autoFormatTool->findText(settings->autoFormatTool());
    m_autoFormatTool->setCurrentIndex(qMax(index, 0));

    using namespace Utils::Layouting;
    const Break br;

    Column {
        Group {
            Title(tr("Automatic Formatting on File Save")),
            Form {
                Span(2, m_autoFormat), br,
                toolLabel, m_autoFormatTool, br,
                mimeLabel, m_autoFormatMime, br,
                Span(2, m_autoFormatOnlyCurrentProject)
            }
        },
        Stretch()
    }.attachTo(this);

    connect(m_autoFormat, &QCheckBox::toggled, m_autoFormatTool, &QComboBox::setEnabled);
    connect(m_autoFormat, &QCheckBox::toggled, m_autoFormatMime, &QLineEdit::setEnabled);
    connect(m_autoFormat, &QCheckBox::toggled, toolLabel, &QLabel::setEnabled);
    connect(m_autoFormat, &QCheckBox::toggled, mimeLabel, &QLabel::setEnabled);
    connect(m_autoFormat, &QCheckBox::toggled, m_autoFormatOnlyCurrentProject, &QCheckBox::setEnabled);
}

void GeneralOptionsPageWidget::apply()
{
    auto settings = GeneralSettings::instance();
    settings->setAutoFormatOnSave(m_autoFormat->isChecked());
    settings->setAutoFormatTool(m_autoFormatTool->currentText());
    settings->setAutoFormatMime(m_autoFormatMime->text());
    settings->setAutoFormatOnlyCurrentProject(m_autoFormatOnlyCurrentProject->isChecked());
    settings->save();
}

GeneralOptionsPage::GeneralOptionsPage(const QStringList &toolIds)
{
    setId(Constants::OPTION_GENERAL_ID);
    setDisplayName(GeneralOptionsPageWidget::tr("General"));
    setCategory(Constants::OPTION_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("Beautifier", "Beautifier"));
    setWidgetCreator([toolIds] { return new GeneralOptionsPageWidget(toolIds); });
    setCategoryIconPath(":/beautifier/images/settingscategory_beautifier.png");
}

} // Beautifier::Internal
