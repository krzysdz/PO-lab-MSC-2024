#include "TreeModel.hpp"
#include "../ModelARX.h"
#include "../ObiektStatyczny.hpp"
#include "../RegulatorPID.h"
#include <optional>

#define _RET_QSTRING_IF_CAST_OK(T, ptr)                                                            \
    if (dynamic_cast<T *>(ptr))                                                                    \
    return QString(#T)

std::optional<QModelIndex> TreeModel::find_idx(PętlaUAR *loop, ObiektSISO *ptr, int loop_row) const
{
    for (std::size_t i = 0; i < loop->m_loop.size(); ++i) {
        auto cptr = static_cast<ObiektSISO *>(loop->m_loop[i].get());
        if (cptr == ptr)
            return this->createIndex(loop_row, 0, loop);
        if (auto *loop_ptr = dynamic_cast<PętlaUAR *>(cptr)) {
            auto result = find_idx(loop_ptr, ptr, i);
            if (result != std::nullopt)
                return result;
        }
    }
    return std::nullopt;
}

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel{ parent }
{
}

TreeModel::TreeModel(PętlaUAR *root_loop, QObject *parent)
    : QAbstractItemModel{ parent }
    , m_root_loop{ root_loop }
{
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        if (!index.isValid() || index.internalPointer() == nullptr)
            return {};

        auto raw_ptr = static_cast<ObiektSISO *>(index.internalPointer());
        _RET_QSTRING_IF_CAST_OK(PętlaUAR, raw_ptr);
        _RET_QSTRING_IF_CAST_OK(ModelARX, raw_ptr);
        _RET_QSTRING_IF_CAST_OK(RegulatorPID, raw_ptr);
        _RET_QSTRING_IF_CAST_OK(ObiektStatyczny, raw_ptr);
    }
    return {};
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal && section == 0)
        return QString("Component");
    return {};
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row != 0)
            return {};
        return createIndex(row, column, m_root_loop);
    }

    if (auto as_loop
        = dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(parent.internalPointer())))
        return createIndex(row, column, as_loop->m_loop.at(row).get());

    return {};
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    auto target_child_ptr = index.internalPointer();
    if (target_child_ptr == m_root_loop)
        return {};

    auto result = find_idx(m_root_loop, static_cast<ObiektSISO *>(target_child_ptr), 0);
    return result.value_or(QModelIndex{});
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;

    auto as_loop = dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(parent.internalPointer()));
    return as_loop != nullptr ? as_loop->size() : 0;
}

int TreeModel::columnCount(const QModelIndex &) const { return 1; }

bool TreeModel::insertChild(const QModelIndex &parent, int position,
                            std::unique_ptr<ObiektSISO> &&component)
{
    if (!parent.isValid() || parent.internalPointer() == nullptr || position < 0)
        return false;

    auto parent_loop
        = dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(parent.internalPointer()));
    if (parent_loop == nullptr)
        return false;

    auto &vec = parent_loop->m_loop;
    if (static_cast<std::size_t>(position) > vec.size())
        return false;

    beginInsertRows(parent, position, position);
    if (static_cast<std::size_t>(position) == vec.size()) {
        vec.push_back(std::move(component));
    } else {
        auto insert_it = std::next(vec.cbegin(), position);
        vec.insert(insert_it, std::move(component));
    }
    endInsertRows();
    return true;
}

bool TreeModel::appendChild(const QModelIndex &parent, std::unique_ptr<ObiektSISO> &&component)
{
    if (parent.isValid()) {
        if (auto as_loop
            = dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(parent.internalPointer())))
            return insertChild(parent, static_cast<int>(as_loop->size()), std::move(component));
    }
    return false;
}

bool TreeModel::removeRow(const QModelIndex &parent, int position)
{
    if (!parent.isValid() || parent.internalPointer() == nullptr || position < 0)
        return false;

    auto parent_loop
        = dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(parent.internalPointer()));
    if (parent_loop == nullptr)
        return false;

    auto &vec = parent_loop->m_loop;
    if (static_cast<std::size_t>(position) >= vec.size())
        return false;

    beginRemoveRows(parent, position, position);
    auto erase_it = std::next(vec.cbegin(), position);
    vec.erase(erase_it);
    endRemoveRows();
    return true;
}

void TreeModel::begin_reset() { beginResetModel(); }

void TreeModel::end_reset() { endResetModel(); }
