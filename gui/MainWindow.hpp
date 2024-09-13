#pragma once
#include "../ModelARX.h"
#include "../PętlaUAR.hpp"
#include "../RegulatorPID.h"
#include "../generators.hpp"
#include "GeneratorsConfig.hpp"
#include "TreeModel.hpp"
#include "param_editors.hpp"
#include <QAction>
#include <QChart>
#include <QChartView>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSessionManager>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedLayout>
#include <QTabWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <optional>
#include <vector>

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QMenu *menu_file;
    QMenu *submenu_export;
    QMenu *submenu_import;
    QAction *action_exit;
    QAction *action_export_model;
    QAction *action_export_generators;
    QAction *action_import_model;
    QAction *action_import_pid;
    QMenu *menu_edit;
    QMenu *submenu_append_child;
    QMenu *submenu_insert_component;
    QAction *action_append_loop;
    QAction *action_append_ARX;
    QAction *action_append_PID;
    QAction *action_append_static;
    QAction *action_insert_loop;
    QAction *action_insert_ARX;
    QAction *action_insert_PID;
    QAction *action_insert_static;
    QAction *action_remove_component;
    QAction *action_reset_sim;
    QAction *action_reset_sim_gen;
    QWidget *widget_right;
    QWidget *widget_components;
    QWidget *widget_inputs;
    QTabWidget *tabs_input;
    QSplitter *main_columns_splitter;
    QVBoxLayout *layout_components;
    QVBoxLayout *layout_right_col;
    QChart *plot;
    QChartView *chart_view;
    TreeModel *tree_model;
    QTreeView *tree_view;
    QStackedLayout *layout_param_editors;
    QWidget *editor_nothing;
    PIDParams *editor_pid;
    ObiektStatycznyParams *editor_static;
    ARXParams *editor_arx;
    UARParams *editor_uar;
    QPushButton *button_save_params;
    QGridLayout *layout_inputs;
    QLineEdit *input_inputs;
    QSpinBox *input_repetitions;
    QPushButton *button_simulate;
    GeneratorsConfig *panel_generators;

    PętlaUAR loop{};
    std::vector<double> real_outputs;
    std::vector<double> given_inputs;
    enum class sources { MANUAL, GENERATOR } last_source;

    void setup_ui();
    void prepare_menu_bar();
    void prepare_layout();
    std::vector<double> parse_coefficients(const QString &coeff_text);
    void simulate(const std::vector<double> &out_inputs);
    void plot_results();
    void reset_sim(bool incl_generators = false);

    void simulate_manual();
    void simulate_gen(std::vector<double> inputs);

    void export_model();
    void export_generators();
    void import_model();
    void import_generators();

    void remove_component();
    template <typename E>
        requires ParamEditor<E, typename E::ObjectT>
    void insert_component(const QString &title, bool append = false)
    {
        const auto index = tree_view->selectionModel()->currentIndex();
        const auto insertion_parent = append ? index : tree_model->parent(index);
        if (!insertion_parent.isValid())
            return;
        const auto as_loop = dynamic_cast<PętlaUAR *>(
            static_cast<ObiektSISO *>(insertion_parent.internalPointer()));
        if (as_loop == nullptr)
            return;
        auto component = EditorDialog<E>::get_component(this, title);
        if (component == nullptr)
            return;
        if (append)
            tree_model->appendChild(insertion_parent, std::move(component));
        else
            tree_model->insertChild(insertion_parent, index.row(), std::move(component));
        // Make sure that the new component is visible
        tree_view->expand(insertion_parent);
    }

    void change_active_editor(const QModelIndex &index);
    void refresh_editor();
    void update_tree_actions(const QModelIndex &index);
    void on_tree_selection_change();
    template <std::derived_from<ObiektSISO> T>
    void update_from_editor(const ParamEditor<T> auto *editor, ObiektSISO *ptr)
    {
        const auto obj_ptr = dynamic_cast<T *>(ptr);
        Q_ASSERT(obj_ptr != nullptr);
        Q_ASSERT(layout_param_editors->currentWidget() == editor);
        editor->update_obj(*obj_ptr);
    }
    void save_params();

public:
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : QMainWindow{ parent, flags }
    {
    }
    void start();
};
