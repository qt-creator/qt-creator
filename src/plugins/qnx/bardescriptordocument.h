/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BARDESCRIPTORDOCUMENT_H
#define QNX_INTERNAL_BARDESCRIPTORDOCUMENT_H

#include <coreplugin/textdocument.h>
#include <utils/environment.h>

#include <QDomDocument>
#include <QMetaType>

namespace Qnx {
namespace Internal {

class BarDescriptorAsset {
public:
    QString source;
    QString destination;
    bool entry;

    bool operator==(const BarDescriptorAsset &asset) const
    {
        return source == asset.source && destination == asset.destination;
    }
};

typedef QList<BarDescriptorAsset> BarDescriptorAssetList;

class BarDescriptorDocument : public Core::TextDocument
{
    Q_OBJECT

    Q_ENUMS(Tag)

public:
    enum Tag {
        id = 0,
        versionNumber,
        buildId,
        name,
        description,
        icon,
        splashScreens,
        asset,
        aspectRatio,
        autoOrients,
        systemChrome,
        transparent,
        arg,
        action,
        env,
        author,
        publisher,
        authorId
    };

    explicit BarDescriptorDocument(QObject *parent = 0);
    ~BarDescriptorDocument();

    bool open(QString *errorString, const QString &fileName);
    bool save(QString *errorString, const QString &fileName = QString(), bool autoSave = false);

    QString defaultPath() const;
    QString suggestedFileName() const;

    bool shouldAutoSave() const;
    bool isModified() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

    QString xmlSource() const;
    bool loadContent(const QString &xmlCode, bool setDirty, QString *errorMessage = 0, int *errorLine = 0);

    QVariant value(Tag tag) const;

    void expandPlaceHolders(const QHash<QString, QString> &placeholdersKeyVals);

    QString bannerComment() const;
    void setBannerComment(const QString &commentText);

signals:
    void changed(BarDescriptorDocument::Tag tag, const QVariant &value);

public slots:
    void setValue(BarDescriptorDocument::Tag tag, const QVariant &value);
private:
    QString stringValue(const QString &tagName) const;
    void setStringValue(const QString &tagName, const QString &value);

    QStringList childStringListValue(const QString &tagName, const QString &childTagName) const;
    void setChildStringListValue(const QString &tagName, const QString &childTagName, const QStringList &stringList);

    QStringList stringListValue(const QString &tagName) const;
    void setStringListValue(const QString &tagName, const QStringList &stringList);

    BarDescriptorAssetList assets() const;
    void setAssets(const BarDescriptorAssetList &assets);

    QList<Utils::EnvironmentItem> environment() const;
    void setEnvironment(const QList<Utils::EnvironmentItem> &environment);

    int tagForElement(const QDomElement &element);
    bool expandPlaceHolder_helper(const QDomElement &el, const QString &placeholderKey,
                                  const QString &placeholderText,
                                  QSet<BarDescriptorDocument::Tag> &changedTags);

    void emitAllChanged();

    bool m_dirty;
    QDomDocument m_barDocument;
};

} // namespace Internal
} // namespace Qnx

Q_DECLARE_METATYPE(Qnx::Internal::BarDescriptorAssetList)
Q_DECLARE_METATYPE(QList<Utils::EnvironmentItem>)
Q_DECLARE_METATYPE(Qnx::Internal::BarDescriptorDocument::Tag)

#endif // QNX_INTERNAL_BARDESCRIPTORDOCUMENT_H
