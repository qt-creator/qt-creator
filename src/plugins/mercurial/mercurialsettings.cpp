/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "mercurialsettings.h"
#include "constants.h"

#include <QtCore/QSettings>

namespace Mercurial {
namespace Internal {

    const QLatin1String diffIgnoreWhiteSpaceKey("diffIgnoreWhiteSpace");
    const QLatin1String diffIgnoreBlankLinesKey("diffIgnoreBlankLines");

    MercurialSettings::MercurialSettings() :
        diffIgnoreWhiteSpace(false),
        diffIgnoreBlankLines(false)
    {
        setSettingsGroup(QLatin1String("Mercurial"));
        setBinary(QLatin1String(Constants::MERCURIALDEFAULT));
    }

    MercurialSettings& MercurialSettings::operator=(const MercurialSettings& other)
    {
        VCSBase::VCSBaseClientSettings::operator=(other);
        if (this != &other) {
            diffIgnoreWhiteSpace = other.diffIgnoreWhiteSpace;
            diffIgnoreBlankLines = other.diffIgnoreBlankLines;
        }
        return *this;
    }

    void MercurialSettings::writeSettings(QSettings *settings) const
    {
        VCSBaseClientSettings::writeSettings(settings);
        settings->beginGroup(this->settingsGroup());
        settings->setValue(diffIgnoreWhiteSpaceKey, diffIgnoreWhiteSpace);
        settings->setValue(diffIgnoreBlankLinesKey, diffIgnoreBlankLines);
        settings->endGroup();
    }

    void MercurialSettings::readSettings(const QSettings *settings)
    {
        VCSBaseClientSettings::readSettings(settings);
        const QString keyRoot = this->settingsGroup() + QLatin1Char('/');
        diffIgnoreWhiteSpace = settings->value(keyRoot + diffIgnoreWhiteSpaceKey, false).toBool();
        diffIgnoreBlankLines = settings->value(keyRoot + diffIgnoreBlankLinesKey, false).toBool();
    }

    bool MercurialSettings::equals(const VCSBaseClientSettings &rhs) const
    {
        const MercurialSettings *hgRhs = dynamic_cast<const MercurialSettings *>(&rhs);
        if (hgRhs == 0)
            return false;
        return VCSBaseClientSettings::equals(rhs)
                && diffIgnoreWhiteSpace == hgRhs->diffIgnoreWhiteSpace
                && diffIgnoreBlankLines == hgRhs->diffIgnoreBlankLines;
    }

} // namespace Internal
} // namespace Mercurial
