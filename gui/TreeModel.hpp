#pragma once
#include "../PętlaUAR.hpp"
#include <QAbstractItemModel>
#include <optional>

/// A tree model representing #PętlaUAR structure.
class TreeModel : public QAbstractItemModel {
    Q_OBJECT

private:
    /// @brief Pointer to the loop used as tree root.
    /// @details Must remain valid while the model is used.
    PętlaUAR *m_root_loop;

    /// @brief Find QModelIndex of `ptr`s parent, starting at `loop`, which is in `loop_row` row in
    /// its parent.
    /// @param loop loop in which to look for
    /// @param ptr pointer which we are looking for
    /// @param loop_row row of `loop` in its parent
    /// @return A QModelIndex of `ptr`'s parent or a `std::nullopt` if not found.
    std::optional<QModelIndex> find_idx(PętlaUAR *loop, ObiektSISO *ptr, int loop_row) const;

public:
    Q_DISABLE_COPY_MOVE(TreeModel)

    TreeModel() = delete;
    /// @brief Construct a tree model representing `root_loop` with parent `parent`.
    /// @param root_loop pointer to the loop represented by the model; **must remain valid while the
    /// model is in use**
    /// @param parent pointer to parent object
    /// @see QAbstractItemModel::QAbstractItemModel()
    TreeModel(PętlaUAR *root_loop, QObject *parent = nullptr);
    ~TreeModel() override = default;

    /// @brief Returns the name of component with given index.
    /// @param index index of the element
    /// @param role role, data returned only for Qt::DisplayRole
    /// @return Component name or nothing
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    /// @brief Returns the data for the given `role` and `section` in the header with the specified
    /// orientation.
    /// @param section column number (assuming horizontal orientation)
    /// @param orientation orientation (only Qt::Orientation::Horizontal is supported)
    /// @param role role, data returned only for Qt::DisplayRole
    /// @return Column name or nothing.
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    /// @brief Get the index of item described by the parameters.
    /// @param row element's row (inside parent)
    /// @param column element's column (expected 0)
    /// @param parent parent's index
    /// @return Index to the element. Invalid if the element does not exist.
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    /// @brief Get index of parent of given index.
    /// @param index valid index
    /// @return Index to the parent. Invalid if index is invalid or points to #m_root_loop.
    QModelIndex parent(const QModelIndex &index) const override;

    /// Get child row count of `parent`.
    int rowCount(const QModelIndex &parent = {}) const override;
    /// Get column count (of `parent`). Always returns `1`.
    int columnCount(const QModelIndex &parent = {}) const override;

    /// @brief Insert component as a child of parent at given position.
    /// @param parent an index of a parent PętlaUAR
    /// @param position insertion position in the parent loop
    /// @param component component to be inserted
    /// @return `true` if the insertion succeeded.
    bool insertChild(const QModelIndex &parent, int position,
                     std::unique_ptr<ObiektSISO> &&component);
    /// @brief Append component as the last child of parent.
    /// @param parent an index of a parent PętlaUAR
    /// @param component component to be appended
    /// @return `true` if the operation succeeded.
    bool appendChild(const QModelIndex &parent, std::unique_ptr<ObiektSISO> &&component);
    /// @brief Remove a row number `position` from parent.
    /// @param parent parent of the row to remove
    /// @param position row number inside parent
    /// @return `true` if the operation succeeded.
    bool removeRow(const QModelIndex &parent, int position);

    /// Call (protected) #beginResetModel(). Must be called before external changes to #m_root_loop.
    void begin_reset();
    /// Call (protected) #endResetModel(). Must be called after external changes to #m_root_loop.
    void end_reset();
};
