%{Cpp:LicenseTemplate}\
#ifndef %{GUARD}
#define %{GUARD}

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

@endif
@if %{RemoveData}
    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

@endif
private:
};
%{JS: Cpp.closeNamespaces('%{Class}')}
#endif // %{GUARD}\
