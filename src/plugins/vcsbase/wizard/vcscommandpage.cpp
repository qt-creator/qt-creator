/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "vcscommandpage.h"

#include <coreplugin/id.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/shellcommand.h>
#include <coreplugin/vcsmanager.h>
#include <projectexplorer/jsonwizard/jsonwizard.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QDebug>

using namespace Core;
using namespace ProjectExplorer;

namespace VcsBase {
namespace Internal {

static char VCSCOMMAND_VCSID[] = "vcsId";
static char VCSCOMMAND_RUN_MESSAGE[] = "trRunMessage";
static char VCSCOMMAND_REPO[] = "repository";
static char VCSCOMMAND_DIR[] = "baseDirectory";
static char VCSCOMMAND_EXTRA_ARGS[] = "extraArguments";
static char VCSCOMMAND_CHECKOUTNAME[] = "checkoutName";

// ----------------------------------------------------------------------
// VcsCommandPageFactory:
// ----------------------------------------------------------------------

VcsCommandPageFactory::VcsCommandPageFactory()
{
    setTypeIdsSuffix(QLatin1String("VcsCommand"));
}

Utils::WizardPage *VcsCommandPageFactory::create(JsonWizard *wizard, Id typeId,
                                                 const QVariant &data)
{
    Q_UNUSED(wizard);

    QTC_ASSERT(canCreate(typeId), return 0);

    QVariantMap tmp = data.toMap();

    auto page = new VcsCommandPage;
    page->setVersionControlId(tmp.value(QLatin1String(VCSCOMMAND_VCSID)).toString());
    page->setRunMessage(tmp.value(QLatin1String(VCSCOMMAND_RUN_MESSAGE)).toString());

    QStringList args;
    const QVariant argsVar = tmp.value(QLatin1String(VCSCOMMAND_EXTRA_ARGS));
    if (!argsVar.isNull()) {
        if (argsVar.type() == QVariant::String) {
            args << argsVar.toString();
        } else if (argsVar.type() == QVariant::List) {
            args = Utils::transform(argsVar.toList(), [](const QVariant &in) {
                return in.toString();
            });
        } else {
            return 0;
        }
    }

    page->setCheckoutData(tmp.value(QLatin1String(VCSCOMMAND_REPO)).toString(),
                          tmp.value(QLatin1String(VCSCOMMAND_DIR)).toString(),
                          tmp.value(QLatin1String(VCSCOMMAND_CHECKOUTNAME)).toString(),
                          args);

    return page;
}

bool VcsCommandPageFactory::validateData(Id typeId, const QVariant &data, QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    QString em;
    if (data.type() != QVariant::Map)
        em = tr("\"data\" is no json object in \"VcsCommand\" page.");

    if (em.isEmpty()) {
        const QVariantMap tmp = data.toMap();

        QString str = tmp.value(QLatin1String(VCSCOMMAND_VCSID)).toString();
        if (str.isEmpty()) {
            em = tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_VCSID));
        }
        str = tmp.value(QLatin1String(VCSCOMMAND_REPO)).toString();
        if (str.isEmpty()) {
            em = tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_REPO));
        }
        str = tmp.value(QLatin1String(VCSCOMMAND_DIR)).toString();
        if (str.isEmpty()) {
            em = tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_DIR));;
        }
        str = tmp.value(QLatin1String(VCSCOMMAND_CHECKOUTNAME)).toString();
        if (str.isEmpty()) {
            em = tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_CHECKOUTNAME));
        }

        str = tmp.value(QLatin1String(VCSCOMMAND_RUN_MESSAGE)).toString();
        if (str.isEmpty()) {
            em = tr("\"%1\" not set in \"data\" section of \"VcsCommand\" page.")
                    .arg(QLatin1String(VCSCOMMAND_RUN_MESSAGE));
        }

        const QVariant extra = tmp.value(QLatin1String(VCSCOMMAND_EXTRA_ARGS));
        if (!extra.isNull() && extra.type() != QVariant::String && extra.type() != QVariant::List) {
            em = tr("\"%1\" in \"data\" section of \"VcsCommand\" page has unexpected type (unset, String or List).")
                    .arg(QLatin1String(VCSCOMMAND_EXTRA_ARGS));
        }
    }

    if (errorMessage)
        *errorMessage = em;

    return em.isEmpty();
}

// ----------------------------------------------------------------------
// VcsCommandPage:
// ----------------------------------------------------------------------

VcsCommandPage::VcsCommandPage()
{
    setTitle(tr("Checkout"));
}

void VcsCommandPage::initializePage()
{
    auto wiz = qobject_cast<JsonWizard *>(wizard());
    QTC_ASSERT(wiz, return);

    const QString vcsId = wiz->expander()->expand(m_vcsId);
    IVersionControl *vc = VcsManager::versionControl(Id::fromString(vcsId));
    if (!vc) {
        qWarning() << QCoreApplication::translate("VcsBase::VcsCommandPage",
                                                  "\"%1\" (%2) not found.")
                      .arg(QLatin1String(VCSCOMMAND_VCSID), vcsId);
        return;
    }
    if (!vc->isConfigured()) {
        qWarning() << QCoreApplication::translate("VcsBase::VcsCommandPage",
                                                  "Version control \"%1\" is not configured.")
                      .arg(vcsId);
        return;
    }
    if (!vc->supportsOperation(IVersionControl::InitialCheckoutOperation)) {
        qWarning() << QCoreApplication::translate("VcsBase::VcsCommandPage",
                                                  "Version control \"%1\" does not support initial checkouts.")
                      .arg(vcsId);
        return;
    }

    const QString repo = wiz->expander()->expand(m_repository);
    if (repo.isEmpty()) {
        qWarning() << QCoreApplication::translate("VcsBase::VcsCommandPage",
                                                  "\"%1\" is empty when trying to run checkout.")
                      .arg(QLatin1String(VCSCOMMAND_REPO));
        return;
    }

    const QString base = wiz->expander()->expand(m_directory);
    if (!QDir(base).exists()) {
        qWarning() << QCoreApplication::translate("VcsBase::VcsCommandPage",
                                                  "\"%1\" (%2) does not exist.")
                      .arg(QLatin1String(VCSCOMMAND_DIR), base);
        return;
    }

    const QString name = wiz->expander()->expand(m_name);
    if (name.isEmpty()) {
        qWarning() << QCoreApplication::translate("VcsBase::VcsCommandPage",
                                                  "\"%1\" is empty when trying to run checkout.")
                      .arg(QLatin1String(VCSCOMMAND_CHECKOUTNAME));
        return;
    }

    const QString runMessage = wiz->expander()->expand(m_runMessage);
    if (!runMessage.isEmpty())
        setStartedStatus(runMessage);

    QStringList extraArgs;
    foreach (const QString &in, m_arguments) {
        const QString tmp = wiz->expander()->expand(in);
        if (tmp.isEmpty())
           continue;
        if (tmp == QStringLiteral("\"\""))
            extraArgs << QString();
        extraArgs << tmp;
    }

    ShellCommand *command
            = vc->createInitialCheckoutCommand(repo, Utils::FileName::fromString(base),
                                               name, extraArgs);
    start(command);
}

void VcsCommandPage::setCheckoutData(const QString &repo, const QString &baseDir, const QString &name,
                                  const QStringList &args)
{
    m_repository = repo;
    m_directory = baseDir;
    m_name = name;
    m_arguments = args;
}

void VcsCommandPage::appendJob(bool skip, const QStringList &command)
{
    m_additionalJobs.append(JobData(skip, command));
}

void VcsCommandPage::setVersionControlId(const QString &id)
{
    m_vcsId = id;
}

void VcsCommandPage::setRunMessage(const QString &msg)
{
    m_runMessage = msg;
}

} // namespace Internal
} // namespace VcsBase
