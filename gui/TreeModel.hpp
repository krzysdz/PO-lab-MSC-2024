#pragma once
#include "../PętlaUAR.hpp"
#include <QAbstractItemModel>
#include <optional>

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

private:
    PętlaUAR *m_root_loop;

    std::optional<QModelIndex> find_idx(PętlaUAR *loop, ObiektSISO *ptr, int loop_row) const;

public:
    Q_DISABLE_COPY_MOVE(TreeModel)

    explicit TreeModel(QObject *parent = nullptr);
    TreeModel(PętlaUAR *root_loop, QObject *parent = nullptr);
    ~TreeModel() override = default;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;

    bool insertChild(const QModelIndex &parent, int position,
                     std::unique_ptr<ObiektSISO> &&component);
    bool appendChild(const QModelIndex &parent, std::unique_ptr<ObiektSISO> &&component);
    bool removeRow(const QModelIndex &parent, int position);

    void begin_reset();
    void end_reset();
};
