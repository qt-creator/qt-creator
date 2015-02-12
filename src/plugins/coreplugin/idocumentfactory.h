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

#ifndef IDOCUMENTFACTORY_H
#define IDOCUMENTFACTORY_H

#include "id.h"

#include <QObject>
#include <QStringList>

#include <functional>

namespace Core {

class IDocument;

class CORE_EXPORT IDocumentFactory : public QObject
{
    Q_OBJECT

public:
    IDocumentFactory(QObject *parent = 0) : QObject(parent) {}

    typedef std::function<IDocument *(const QString &fileName)> Opener;
    IDocument *open(const QString &filename);

    QStringList mimeTypes() const { return m_mimeTypes; }

    void setOpener(const Opener &opener) { m_opener = opener; }
    void setMimeTypes(const QStringList &mimeTypes) { m_mimeTypes = mimeTypes; }
    void addMimeType(const char *mimeType) { m_mimeTypes.append(QLatin1String(mimeType)); }
    void addMimeType(const QString &mimeType) { m_mimeTypes.append(mimeType); }

private:
    Id m_id;
    Opener m_opener;
    QStringList m_mimeTypes;
    QString m_displayName;
};

} // namespace Core

#endif // IDOCUMENTFACTORY_H
