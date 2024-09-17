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
#include <filesystem>
#include <optional>
#include <vector>

#ifdef IE_TESTS
class ImportExportTest;
#endif

/// Main application window.
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    /// @e File menu
    QMenu *menu_file;
    /// @e Export sumbenu of the @link #menu_file @e File menu@endlink
    QMenu *submenu_export;
    /// @e Import sumbenu of the @link #menu_file @e File menu@endlink
    QMenu *submenu_import;
    /// @e Exit action in the @link #menu_file @e File menu@endlink
    QAction *action_exit;
    /// @e Open action in the @link #menu_file @e File menu@endlink
    QAction *action_open;
    /// @e Save action in the @link #menu_file @e File menu@endlink
    QAction *action_save;
    /// _Export loop model_ action in the @link #submenu_export @e Export submenu@endlink
    QAction *action_export_model;
    /// _Export generators_ action in the @link #submenu_export @e Export submenu@endlink
    QAction *action_export_generators;
    /// _Import loop model_ action in the @link #submenu_import @e Import submenu@endlink
    QAction *action_import_model;
    /// _Import generators_ action in the @link #submenu_import @e Import submenu@endlink
    QAction *action_import_generators;
    /// @e Edit menu
    QMenu *menu_edit;
    /// _Append child component_ submenu of the @link #menu_edit @e Edit menu@endlink
    QMenu *submenu_append_child;
    /// _Insert component_ submenu of the @link #menu_edit @e Edit menu@endlink
    QMenu *submenu_insert_component;
    /// _Append loop_ action in the @link #submenu_append_child @e Append submenu@endlink
    QAction *action_append_loop;
    /// _Append ARX model_ action in the @link #submenu_append_child @e Append submenu@endlink
    QAction *action_append_ARX;
    /// _Append PID regulator_ action in the @link #submenu_append_child @e Append submenu@endlink
    QAction *action_append_PID;
    /// _Append ObiektStatyczny_ action in the @link #submenu_append_child @e Append submenu@endlink
    QAction *action_append_static;
    /// _Insert loop_ action in the @link #submenu_insert_component @e Insert submenu@endlink
    QAction *action_insert_loop;
    /// _Insert ARX model_ action in the @link #submenu_insert_component @e Insert submenu@endlink
    QAction *action_insert_ARX;
    /// @brief _Insert PID regulator_ action in the @link #submenu_insert_component @e Insert
    /// submenu@endlink
    QAction *action_insert_PID;
    /// @brief _Insert ObiektStatyczny_ action in the @link #submenu_insert_component @e Insert
    /// submenu@endlink
    QAction *action_insert_static;
    /// _Remove component_ action of the @link #menu_edit @e Edit menu@endlink
    QAction *action_remove_component;
    /// _Reset simulation_ action of the @link #menu_edit @e Edit menu@endlink
    QAction *action_reset_sim;
    /// _Reset simulation (incl. generators)_ action of the @link #menu_edit @e Edit menu@endlink
    QAction *action_reset_sim_gen;
    /// Widget on the right with inputs, generators and plot
    QWidget *widget_right;
    /// Widget with component tree and component settings
    QWidget *widget_components;
    /// Widget with manual simualtion inputs
    QWidget *widget_inputs;
    /// @brief Tabbed widget with different simulation options - manual (#widget_inputs) and
    /// generators (#panel_generators)
    QTabWidget *tabs_input;
    /// Splitter between left (#widget_components) and right (#widget_right) part of the window
    QSplitter *main_columns_splitter;
    /// Layout of the #widget_components widget
    QVBoxLayout *layout_components;
    /// Layout of the #widget_right widget
    QVBoxLayout *layout_right_col;
    /// Chart with simulation results
    QChart *plot;
    /// Widget rendering the #plot
    QChartView *chart_view;
    /// Tree model of the #loop
    TreeModel *tree_model;
    /// Tree view of the #tree_model showing all components
    QTreeView *tree_view;
    /// Layout with component @link ParamEditor parameter editors@endlink
    QStackedLayout *layout_param_editors;
    /// Empty widget to show in #layout_param_editors when nothing is selected in #tree_view
    QWidget *editor_nothing;
    /// Editor for RegulatorPID components shown in #layout_param_editors
    PIDParams *editor_pid;
    /// Editor for ObiektStatyczny components shown in #layout_param_editors
    ObiektStatycznyParams *editor_static;
    /// Editor for ModelARX components shown in #layout_param_editors
    ARXParams *editor_arx;
    /// Editor for PętlaUAR components shown in #layout_param_editors
    UARParams *editor_uar;
    /// _Save changes_ button shown in #layout_components to save editor changes
    QPushButton *button_save_params;
    /// Layout of the #widget_inputs widget
    QGridLayout *layout_inputs;
    /// @brief Text input accepting a comma separated list of numbers to use as manual input shown
    /// in #widget_inputs
    QLineEdit *input_inputs;
    /// Spinbox with number of repetitions of input from #input_inputs for manual simulation
    QSpinBox *input_repetitions;
    /// @e Simulate button for manual simulation
    QPushButton *button_simulate;
    /// Widget with generator configuration and simulation
    GeneratorsConfig *panel_generators;

    /// The main control loop
    PętlaUAR loop{};
    /// Simulation outputs
    std::vector<double> real_outputs;
    /// simulation inputs
    std::vector<double> given_inputs;
    /// Source of simulation
    enum class sources {
        MANUAL, ///< Manual inputs
        GENERATOR ///< Input from generators
    };
    /// Last used simulation source
    sources last_source;

    /// Perform UI setup
    void setup_ui();
    /// Configure menu bar and connect menu actions' signals
    void prepare_menu_bar();
    /// Prepare main window layout and connect signals
    void prepare_layout();
    /// @brief Parse a comma-delimited string of decimal numbers
    /// @param coeff_text string with comma-separated decimal numbers
    /// @return A vector of parsed coefficients.
    std::vector<double> parse_coefficients(const QString &coeff_text);
    /// @brief Perform simulation using provided or manual inputs
    /// @param out_inputs inputs to evaluate, manual input is used if vector is empty
    void simulate(const std::vector<double> &out_inputs);
    /// Update the plot with simulation results based on #real_outputs and #given_inputs
    void plot_results();
    /// @brief Clear saved inputs and outputs, call PętlaUAR::reset()
    /// @param incl_generators whether generators simulation time should be reset too using
    /// (GeneratorsConfig::reset_sim())
    void reset_sim(bool incl_generators = false);

    /// @brief Set #last_source to @link sources::MANUAL MANUAL@endlink, @link #reset_sim() reset
    /// simulation@endlink if previous was @link sources::GENERATOR GENERATOR@endlink and perform
    /// the simulation
    void simulate_manual();
    /// @brief Set #last_source to @link sources::GENERATOR GENERATOR@endlink, @link #reset_sim()
    /// reset simulation@endlink if previous was @link sources::MANUAL MANUAL@endlink and perform
    /// the simulation
    /// @param inputs simulation input sequence received from generator
    void simulate_gen(std::vector<double> inputs);

    /// @brief Read whole file into memory
    /// @param path file path
    /// @return A pair with a pointer to the file data and the file size
    static std::pair<std::unique_ptr<uint8_t[]>, std::size_t>
    read_file(const std::filesystem::path &path);
    /// @brief Write bytes to file
    /// @param path file path
    /// @param data data to write
    static void write_file(const std::filesystem::path &path, const std::vector<uint8_t> &data);
    /// @brief Replace the #loop and refresh the tree view without violating the model
    /// @param l new loop
    void replace_loop(PętlaUAR &&l);
    /// Open and import any supported file
    void open_file();
    /// Save full config (loop and generators) into a single file
    void save_full_config();
    /// Export the loop model to file
    void export_model();
    /// Export generators to file
    void export_generators();
    /// Import the loop model from file
    void import_model();
    /// Import generators from file
    void import_generators();

    /// Remove component currently selected in #tree_view
    void remove_component();
    /// @brief Insert or append component to the loop configured using a dialog
    /// @tparam E type of ParamEditor to use in dialog
    /// @param title title of the dialog
    /// @param append whether the component should be appended to the selected element element (only
    /// if it is PętlaUAR) or inserted before it
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

    /// @brief Change editor presented by #layout_param_editors to one matching the type and values
    /// of currently selected component
    /// @param index index of currently selected component
    void change_active_editor(const QModelIndex &index);
    /// Call #change_active_editor() with the index of currently selected component
    void refresh_editor();
    /// @brief Update menu actions (enabled/disabled) based on wheter they are valid for the
    /// specified index
    /// @param index index determining actions' state
    void update_tree_actions(const QModelIndex &index);
    /// Update the UI according to component selection change in #tree_view
    void on_tree_selection_change();
    /// @brief Update object based on values from the editor
    /// @tparam T type of the object
    /// @param editor editor satisfying @link ParamEditor ParamEditor<T>@endlink with new values
    /// of the object
    /// @param ptr pointer to the object which should be updated
    template <std::derived_from<ObiektSISO> T>
    void update_from_editor(const ParamEditor<T> auto *editor, ObiektSISO *ptr)
    {
        const auto obj_ptr = dynamic_cast<T *>(ptr);
        Q_ASSERT(obj_ptr != nullptr);
        Q_ASSERT(layout_param_editors->currentWidget() == editor);
        editor->update_obj(*obj_ptr);
    }
    /// Apply settings from editor to the selected component
    void save_params();

public:
    /// @brief Construct main window with given parent and flags
    /// @see QMainWindow::QMainWindow()
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : QMainWindow{ parent, flags }
    {
    }
    /// @brief Initialize and show the window
    void start();

#ifdef IE_TESTS
    friend class ImportExportTest;
#endif
};
