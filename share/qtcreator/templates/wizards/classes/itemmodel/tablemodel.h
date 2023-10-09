%{JS: Cpp.licenseTemplate()}\
@if '%{JS: Cpp.usePragmaOnce()}' == 'true'
#pragma once
@else
#ifndef %{GUARD}
#define %{GUARD}
@endif

%{JS: QtSupport.qtIncludes([ 'QtCore/%{Base}' ], [ 'QtCore/%{Base}' ])}\
%{JS: Cpp.openNamespaces('%{Class}')}\

class %{CN} : public %{Base}
{
     Q_OBJECT

public:
    explicit %{CN}(QObject *parent = nullptr);

@if %{CustomHeader}
    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

@if %{Editable}
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

@endif
@endif
    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

@if %{DynamicFetch}
    // Fetch data dynamically:
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

@endif
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

@if %{Editable}
    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

@endif
@if %{AddData}
    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

@endif
@if %{RemoveData}
    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

@endif
private:
};
%{JS: Cpp.closeNamespaces('%{Class}')}
@if '%{JS: Cpp.usePragmaOnce()}' === 'false'
#endif // %{GUARD}
@endif
