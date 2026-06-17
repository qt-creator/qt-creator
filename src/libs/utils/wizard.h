// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "result.h"
#include "utils_global.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QWizard>
#include <QWizardPage>

#include <functional>

namespace Utils {

class FilePath;
class WizardPrivate;
class WizardProgress;

const char SHORT_TITLE_PROPERTY[] = "shortTitle";

class QTCREATOR_UTILS_EXPORT Wizard : public QWizard
{
    Q_OBJECT
    Q_PROPERTY(bool automaticProgressCreationEnabled READ isAutomaticProgressCreationEnabled WRITE setAutomaticProgressCreationEnabled)

public:
    explicit Wizard(Qt::WindowFlags flags = {});
    ~Wizard() override;

    bool isAutomaticProgressCreationEnabled() const;
    void setAutomaticProgressCreationEnabled(bool enabled);

    void setStartId(int pageId);

    WizardProgress *wizardProgress() const;

    template<class T> T *find() const
    {
        const QList<int> pages = pageIds();
        for (int id : pages) {
            if (T *result = qobject_cast<T *>(page(id)))
                return result;
        }
        return 0;
    }

    // will return true for all fields registered via Utils::WizardPage::registerFieldWithName(...)
    bool hasField(const QString &name) const;
    void registerFieldName(const QString &name);
    QSet<QString> fieldNames() const;

    virtual QHash<QString, QVariant> variables() const;

    void showVariables();

    // allows to skip pages
    void setSkipForSubprojects(bool skip);
    int nextId() const override;

protected:
    virtual QString stringify(const QVariant &v) const;
    virtual QString evaluate(const QVariant &v) const;
    bool event(QEvent *event) override;

private:
    void _q_currentPageChanged(int pageId);
    void _q_pageAdded(int pageId);
    void _q_pageRemoved(int pageId);

    Q_DECLARE_PRIVATE(Wizard)
    class WizardPrivate *d_ptr;
};

class WizardProgressItem;
class WizardProgressPrivate;

class QTCREATOR_UTILS_EXPORT WizardProgress : public QObject
{
    Q_OBJECT

public:
    WizardProgress(QObject *parent = nullptr);
    ~WizardProgress() override;

    WizardProgressItem *addItem(const QString &title);
    void removeItem(WizardProgressItem *item);

    void removePage(int pageId);

    static QList<int> pages(WizardProgressItem *item);
    WizardProgressItem *item(int pageId) const;

    WizardProgressItem *currentItem() const;

    QSet<WizardProgressItem *> items() const;

    WizardProgressItem *startItem() const;

    QList<WizardProgressItem *> visitedItems() const;
    QList<WizardProgressItem *> directlyReachableItems() const;
    bool isFinalItemDirectlyReachable() const; // return  availableItems().last()->isFinalItem();

Q_SIGNALS:
    void currentItemChanged(WizardProgressItem *item);

    void itemChanged(WizardProgressItem *item); // contents of the item: title or icon
    void itemAdded(WizardProgressItem *item);
    void itemRemoved(WizardProgressItem *item);
    void nextItemsChanged(WizardProgressItem *item, const QList<WizardProgressItem *> &items);
    void nextShownItemChanged(WizardProgressItem *item, WizardProgressItem *nextShownItem);
    void startItemChanged(WizardProgressItem *item);

private:
    void setCurrentPage(int pageId);
    void setStartPage(int pageId);

private:
    friend class Wizard;
    friend class WizardProgressItem;

    friend QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &debug, const WizardProgress &progress);

    Q_DECLARE_PRIVATE(WizardProgress)

    class WizardProgressPrivate *d_ptr;
};

class WizardProgressItemPrivate;

class QTCREATOR_UTILS_EXPORT WizardProgressItem // managed by WizardProgress
{

public:
    void addPage(int pageId);
    QList<int> pages() const;
    void setNextItems(const QList<WizardProgressItem *> &items);
    QList<WizardProgressItem *> nextItems() const;
    void setNextShownItem(WizardProgressItem *item);
    WizardProgressItem *nextShownItem() const;
    bool isFinalItem() const; // return nextItems().isEmpty();

    void setTitle(const QString &title);
    QString title() const;
    void setTitleWordWrap(bool wrap);
    bool titleWordWrap() const;

protected:
    WizardProgressItem(WizardProgress *progress, const QString &title);
    virtual ~WizardProgressItem();

private:
    friend class WizardProgress;
    friend QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &d, const WizardProgressItem &item);

    Q_DECLARE_PRIVATE(WizardProgressItem)

    class WizardProgressItemPrivate *d_ptr;
};

namespace Internal {

class QTCREATOR_UTILS_EXPORT ObjectToFieldWidgetConverter : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value NOTIFY valueChanged)

public:
    template<class T, typename... Arguments>
    static ObjectToFieldWidgetConverter *create(T *sender,
                                                void (T::*member)(Arguments...),
                                                const std::function<QVariant()> &toVariantFunction)
    {
        auto widget = new ObjectToFieldWidgetConverter();
        widget->toVariantFunction = toVariantFunction;
        connect(sender, &QObject::destroyed, widget, &QObject::deleteLater);
        connect(sender, member, widget, [widget] { emit widget->valueChanged(widget->value()); });
        return widget;
    }

signals:
    void valueChanged(const QVariant &);

private:
    ObjectToFieldWidgetConverter () = default;

    // is used by the property value
    QVariant value() { return toVariantFunction(); }
    std::function<QVariant()> toVariantFunction;
};

} // Internal

class QTCREATOR_UTILS_EXPORT WizardPage : public QWizardPage
{
    Q_OBJECT

public:
    using QWizardPage::QWizardPage;

    virtual void pageWasAdded(); // called when this page was added to a Utils::Wizard

    template<class T, typename... Arguments>
    void registerObjectAsFieldWithName(const QString &name,
                                       T *sender,
                                       void (T::*changeSignal)(Arguments...),
                                       const std::function<QVariant()> &senderToVariant)
    {
        registerFieldWithName(name,
                              Internal::ObjectToFieldWidgetConverter::create(sender,
                                                                             changeSignal,
                                                                             senderToVariant),
                              "value",
                              SIGNAL(valueChanged(QValue)));
    }

    void registerFieldWithName(const QString &name, QWidget *widget,
                               const char *property = nullptr, const char *changedSignal = nullptr);

    void setSkipForSubprojects(bool skip) { m_skipForSubproject = skip; }
    bool skipForSubprojects() const { return m_skipForSubproject; }

    virtual bool handleReject();
    virtual bool handleAccept();

signals:
    // Emitted when there is something that the developer using this page should be aware of.
    void reportError(const QString &errorMessage);

private:
    void registerFieldName(const QString &name);

    QSet<QString> m_toRegister;
    bool m_skipForSubproject = false;
};

class QTCREATOR_UTILS_EXPORT FileWizardPage : public WizardPage
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName DESIGNABLE true)

public:
    explicit FileWizardPage(QWidget *parent = nullptr);
    ~FileWizardPage() override;

    QString fileName() const;
    [[deprecated("Use filePath()")]] QString path() const;

    Utils::FilePath filePath() const;

    bool isComplete() const override;

    void setFileNameLabel(const QString &label);
    void setPathLabel(const QString &label);
    void setDefaultSuffix(const QString &suffix);

    bool forceFirstCapitalLetterForFileName() const;
    void setForceFirstCapitalLetterForFileName(bool b);
    void setAllowDirectoriesInFileSelector(bool allow);

    // Validate a base name entry field (potentially containing extension)
    static Utils::Result<> validateBaseName(const QString &name);

signals:
    void activated();
    void pathChanged();

public slots:
    void setPath(const QString &path); // Deprecated: Use setFilePath
    void setPathVisible(bool visible);
    void setFileName(const QString &name);
    void setFilePath(const Utils::FilePath &filePath);

private:
    void slotValidChanged();
    void slotActivated();

    class FileWizardPagePrivate *d;
};

QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &debug, const WizardProgress &progress);

QTCREATOR_UTILS_EXPORT QDebug &operator<<(QDebug &debug, const WizardProgressItem &item);

} // namespace Utils
