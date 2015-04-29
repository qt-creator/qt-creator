/**************************************************************************
**
** Copyright (C) 2015 Hugues Delorme
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
#ifndef BAZAARCONTROL_H
#define BAZAARCONTROL_H

#include <coreplugin/iversioncontrol.h>

QT_BEGIN_NAMESPACE
class QVariant;
QT_END_NAMESPACE

namespace Bazaar {
namespace Internal {

class BazaarClient;

//Implements just the basics of the Version Control Interface
//BazaarClient handles all the work
class BazaarControl: public Core::IVersionControl
{
    Q_OBJECT

public:
    explicit BazaarControl(BazaarClient *bazaarClient);

    QString displayName() const override;
    Core::Id id() const override;

    bool managesDirectory(const QString &filename, QString *topLevel = 0) const override;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const override;
    bool isConfigured() const override;
    bool supportsOperation(Operation operation) const override;
    bool vcsOpen(const QString &fileName) override;
    bool vcsAdd(const QString &filename) override;
    bool vcsDelete(const QString &filename) override;
    bool vcsMove(const QString &from, const QString &to) override;
    bool vcsCreateRepository(const QString &directory) override;
    bool vcsAnnotate(const QString &file, int line) override;

    Core::ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FileName &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs) override;

public slots:
    // To be connected to the VCSTask's success signal to emit the repository/
    // files changed signals according to the variant's type:
    // String -> repository, StringList -> files
    void changed(const QVariant &);

private:
    BazaarClient *m_bazaarClient;
};

} // namespace Internal
} // namespace Bazaar

#endif // BAZAARCONTROL_H
