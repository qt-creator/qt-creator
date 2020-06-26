/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmakekitinformation.h"

#include "qmakeprojectmanagerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QLineEdit>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class QmakeKitAspectWidget : public KitAspectWidget
{
    Q_DECLARE_TR_FUNCTIONS(QmakeProjectManager::Internal::QmakeKitAspect)

public:
    QmakeKitAspectWidget(Kit *k, const KitAspect *ki)
        : KitAspectWidget(k, ki), m_lineEdit(new QLineEdit)
    {
        refresh(); // set up everything according to kit
        m_lineEdit->setToolTip(ki->description());
        connect(m_lineEdit, &QLineEdit::textEdited, this, &QmakeKitAspectWidget::mkspecWasChanged);
    }

    ~QmakeKitAspectWidget() override { delete m_lineEdit; }

private:
    QWidget *mainWidget() const override { return m_lineEdit; }
    void makeReadOnly() override { m_lineEdit->setEnabled(false); }

    void refresh() override
    {
        if (!m_ignoreChange)
            m_lineEdit->setText(QDir::toNativeSeparators(QmakeKitAspect::mkspec(m_kit)));
    }

    void mkspecWasChanged(const QString &text)
    {
        m_ignoreChange = true;
        QmakeKitAspect::setMkspec(m_kit, text, QmakeKitAspect::MkspecSource::User);
        m_ignoreChange = false;
    }

    QLineEdit *m_lineEdit = nullptr;
    bool m_ignoreChange = false;
};


QmakeKitAspect::QmakeKitAspect()
{
    setObjectName(QLatin1String("QmakeKitAspect"));
    setId(QmakeKitAspect::id());
    setDisplayName(tr("Qt mkspec"));
    setDescription(tr("The mkspec to use when building the project with qmake.<br>"
                      "This setting is ignored when using other build systems."));
    setPriority(24000);
}

Tasks QmakeKitAspect::validate(const Kit *k) const
{
    Tasks result;
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);

    const QString mkspec = QmakeKitAspect::mkspec(k);
    if (!version && !mkspec.isEmpty())
        result << BuildSystemTask(Task::Warning, tr("No Qt version set, so mkspec is ignored."));
    if (version && !version->hasMkspec(mkspec))
        result << BuildSystemTask(Task::Error, tr("Mkspec not found for Qt version."));

    return result;
}

KitAspectWidget *QmakeKitAspect::createConfigWidget(Kit *k) const
{
    return new Internal::QmakeKitAspectWidget(k, this);
}

KitAspect::ItemList QmakeKitAspect::toUserOutput(const Kit *k) const
{
    return {qMakePair(tr("mkspec"), QDir::toNativeSeparators(mkspec(k)))};
}

void QmakeKitAspect::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    expander->registerVariable("Qmake:mkspec", tr("Mkspec configured for qmake by the kit."),
                [kit]() -> QString {
                    return QDir::toNativeSeparators(mkspec(kit));
                });
}

Utils::Id QmakeKitAspect::id()
{
    return Constants::KIT_INFORMATION_ID;
}

QString QmakeKitAspect::mkspec(const Kit *k)
{
    if (!k)
        return {};
    return k->value(QmakeKitAspect::id()).toString();
}

QString QmakeKitAspect::effectiveMkspec(const Kit *k)
{
    if (!k)
        return {};
    const QString spec = mkspec(k);
    if (spec.isEmpty())
        return defaultMkspec(k);
    return spec;
}

void QmakeKitAspect::setMkspec(Kit *k, const QString &mkspec, MkspecSource source)
{
    QTC_ASSERT(k, return);
    k->setValue(QmakeKitAspect::id(), source == MkspecSource::Code && mkspec == defaultMkspec(k)
                ? QString() : mkspec);
}

QString QmakeKitAspect::defaultMkspec(const Kit *k)
{
    QtSupport::BaseQtVersion *version = QtSupport::QtKitAspect::qtVersion(k);
    if (!version) // No version, so no qmake
        return {};

    return version->mkspecFor(ToolChainKitAspect::cxxToolChain(k));
}

} // namespace Internal
} // namespace QmakeProjectManager
