#include "MainWindow.hpp"
#include "../ObiektStatyczny.hpp"
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFrame>
#include <QLabel>
#include <QLineSeries>
#include <QMenuBar>
#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <span>

namespace fs = std::filesystem;

void MainWindow::setup_ui()
{
    prepare_menu_bar();
    prepare_layout();
    on_tree_selection_change();
}

void MainWindow::prepare_menu_bar()
{
    menu_file = menuBar()->addMenu("&File");

    action_open = menu_file->addAction("&Open");
    action_open->setShortcut(QKeySequence::Open);
    connect(action_open, &QAction::triggered, this, &MainWindow::open_file);

    // Export
    submenu_export = menu_file->addMenu("Export");

    action_export_model = submenu_export->addAction("Loop model");
    connect(action_export_model, &QAction::triggered, this, &MainWindow::export_model);

    action_export_generators = submenu_export->addAction("Generators");
    connect(action_export_generators, &QAction::triggered, this, &MainWindow::export_generators);

    // Import
    submenu_import = menu_file->addMenu("Import");

    action_import_model = submenu_import->addAction("Loop model");
    connect(action_import_model, &QAction::triggered, this, &MainWindow::import_model);

    action_import_pid = submenu_import->addAction("Generators");
    connect(action_import_pid, &QAction::triggered, this, &MainWindow::import_generators);

    // Others
    action_exit = menu_file->addAction("Exit");
    action_exit->setShortcut(QKeySequence::Quit);
    connect(action_exit, &QAction::triggered, &QApplication::quit);

    menu_edit = menuBar()->addMenu("&Edit");

    // Appending
    submenu_append_child = menu_edit->addMenu("Append child component");

    action_append_PID = submenu_append_child->addAction("RegulatorPID");
    connect(action_append_PID, &QAction::triggered, this,
            [this]() { this->insert_component<PIDParams>("New RegulatorPID", true); });

    action_append_static = submenu_append_child->addAction("ObiektStatyczny");
    connect(action_append_static, &QAction::triggered, this, [this]() {
        this->insert_component<ObiektStatycznyParams>("New ObiektStatyczny", true);
    });

    action_append_ARX = submenu_append_child->addAction("ModelARX");
    connect(action_append_ARX, &QAction::triggered, this,
            [this]() { this->insert_component<ARXParams>("New ModelARX", true); });

    action_append_loop = submenu_append_child->addAction("PętlaUAR");
    connect(action_append_loop, &QAction::triggered, this,
            [this]() { this->insert_component<UARParams>("New PętlaUAR", true); });

    // Inserting
    submenu_insert_component = menu_edit->addMenu("Insert component before selected");

    action_insert_PID = submenu_insert_component->addAction("RegulatorPID");
    connect(action_insert_PID, &QAction::triggered, this,
            [this]() { this->insert_component<PIDParams>("New RegulatorPID"); });

    action_insert_static = submenu_insert_component->addAction("ObiektStatyczny");
    connect(action_insert_static, &QAction::triggered, this,
            [this]() { this->insert_component<ObiektStatycznyParams>("New ObiektStatyczny"); });

    action_insert_ARX = submenu_insert_component->addAction("ModelARX");
    connect(action_insert_ARX, &QAction::triggered, this,
            [this]() { this->insert_component<ARXParams>("New ModelARX"); });

    action_insert_loop = submenu_insert_component->addAction("PętlaUAR");
    connect(action_insert_loop, &QAction::triggered, this,
            [this]() { this->insert_component<UARParams>("New PętlaUAR"); });

    // Others
    action_remove_component = menu_edit->addAction("Remove selected component");
    connect(action_remove_component, &QAction::triggered, this, &MainWindow::remove_component);

    action_reset_sim = menu_edit->addAction("Reset simulation");
    connect(action_reset_sim, &QAction::triggered, this, &MainWindow::reset_sim);

    action_reset_sim_gen = menu_edit->addAction("Reset simulation (incl. generators)");
    connect(action_reset_sim_gen, &QAction::triggered, this, [this]() { this->reset_sim(true); });
}

void MainWindow::prepare_layout()
{
    main_columns_splitter = new QSplitter{ this };
    main_columns_splitter->setChildrenCollapsible(false);
    setCentralWidget(main_columns_splitter);

    // Parameters section - left part of the window
    widget_components = new QWidget{ main_columns_splitter };
    main_columns_splitter->addWidget(widget_components);

    layout_components = new QVBoxLayout{ widget_components };

    QRegularExpression coeff_pattern{ R"(^\s*(?:-?\d+(?:\.\d+)?(?:\s*?,\s*?|\s*?$))*$)" };
    const auto coeff_validator
        = new QRegularExpressionValidator{ coeff_pattern, widget_components };

    // Component tree
    tree_model = new TreeModel(&loop, widget_components);
    tree_view = new QTreeView(widget_components);
    tree_view->setModel(tree_model);
    tree_view->expandAll();
    layout_components->addWidget(tree_view);
    connect(tree_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::on_tree_selection_change);

    // Component parameter editors
    layout_param_editors = new QStackedLayout{};
    layout_components->addItem(layout_param_editors);

    editor_nothing = new QWidget{ widget_components };
    layout_param_editors->addWidget(editor_nothing);

    editor_pid = new PIDParams{ widget_components };
    layout_param_editors->addWidget(editor_pid);

    editor_static = new ObiektStatycznyParams{ widget_components };
    layout_param_editors->addWidget(editor_static);

    editor_arx = new ARXParams{ widget_components };
    layout_param_editors->addWidget(editor_arx);

    editor_uar = new UARParams{ widget_components };
    layout_param_editors->addWidget(editor_uar);

    button_save_params = new QPushButton{ "Save changes", widget_components };
    layout_components->addWidget(button_save_params);
    connect(button_save_params, &QPushButton::released, this, &MainWindow::save_params);

    // Right part of the window
    widget_right = new QWidget{ main_columns_splitter };
    main_columns_splitter->addWidget(widget_right);
    layout_right_col = new QVBoxLayout{ widget_right };

    tabs_input = new QTabWidget{ widget_right };
    tabs_input->setTabPosition(QTabWidget::TabPosition::South);
    tabs_input->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Minimum);
    layout_right_col->addWidget(tabs_input);

    // Inputs and simulation options
    widget_inputs = new QWidget{ tabs_input };
    layout_inputs = new QGridLayout{ widget_inputs };
    tabs_input->addTab(widget_inputs, "Manual");

    input_inputs = new QLineEdit{ widget_inputs };
    input_inputs->setValidator(coeff_validator);
    const auto label_inputs = new QLabel{ "Input sequence", widget_inputs };
    layout_inputs->addWidget(label_inputs, 0, 0);
    layout_inputs->addWidget(input_inputs, 0, 1);

    input_repetitions = new QSpinBox{ widget_inputs };
    input_repetitions->setMinimum(1);
    const auto label_repetitons = new QLabel{ "Repetitions", widget_inputs };
    layout_inputs->addWidget(label_repetitons, 0, 2);
    layout_inputs->addWidget(input_repetitions, 0, 3);

    button_simulate = new QPushButton{ "Simulate", widget_inputs };
    layout_inputs->addWidget(button_simulate, 1, 0);
    connect(button_simulate, &QPushButton::released, this, &MainWindow::simulate_manual);

    // Generators
    panel_generators = new GeneratorsConfig{ tabs_input };
    connect(panel_generators, &GeneratorsConfig::simulated, this, &MainWindow::simulate_gen);
    connect(panel_generators, &GeneratorsConfig::cleared, this, [this]() { this->reset_sim(); });
    tabs_input->addTab(panel_generators, "Generators");

    // Plot
    plot = new QChart{};
    chart_view = new QChartView{ plot, widget_right };
    chart_view->setRenderHint(QPainter::Antialiasing);
    layout_right_col->addWidget(chart_view);
}

std::vector<double> MainWindow::parse_coefficients(const QString &coeff_text)
{
    std::vector<double> coefficients;
    bool success{ true };
    const auto coeff_list = coeff_text.split(",", Qt::SplitBehaviorFlags::SkipEmptyParts);
    for (const auto &coeff : coeff_list) {
        bool ok;
        coefficients.push_back(coeff.trimmed().toDouble(&ok));
        success &= ok;
    }
    if (!success) {
        QMessageBox message_box{ QMessageBox::Icon::Warning, "Problem",
                                 "Could not parse a comma-separated list of doubles",
                                 QMessageBox::StandardButton::Close };
        message_box.exec();
    }
    return coefficients;
}

void MainWindow::simulate(const std::vector<double> &out_inputs)
{
    if (loop.size() == 0) {
        QMessageBox message_box{ QMessageBox::Icon::Warning, "Problem", "Loop has no components",
                                 QMessageBox::StandardButton::Close };
        message_box.exec();
        return;
    }
    std::vector<double> inputs;
    int repetitions;
    if (out_inputs.size()) {
        inputs = out_inputs;
        repetitions = 1;
    } else {
        inputs = parse_coefficients(input_inputs->text());
        repetitions = input_repetitions->value();
    }
    for (int i = 0; i < repetitions; ++i) {
        for (const auto &input : inputs) {
            const auto out = loop.symuluj(input);
            real_outputs.push_back(out);
        }
#if __cpp_lib_containers_ranges >= 202202L
        // This works with libc++, but it does not currently implement the
        // __cpp_lib_containers_ranges macro; only __cpp_lib_ranges_to_container is defined, even
        // though P1206R7 is fully implemented
        // Should be fixed in LLVM 19: https://github.com/llvm/llvm-project/pull/90914
        // A workaroud is provided in define_fixes.hpp
        given_inputs.append_range(inputs);
#else
        given_inputs.insert(given_inputs.end(), inputs.begin(), inputs.end());
#endif
    }
    plot_results();
}

void MainWindow::plot_results()
{
    const auto results_series = new QLineSeries{ plot };
    const auto inputs_series = new QLineSeries{ plot };
    assert(real_outputs.size() == given_inputs.size());
    for (std::size_t i = 0; i < real_outputs.size(); ++i) {
        results_series->append(static_cast<double>(i), real_outputs[i]);
        inputs_series->append(static_cast<double>(i), given_inputs[i]);
    }
    results_series->setName("Simulation results");
    inputs_series->setName("Inputs");
    plot->removeAllSeries();
    plot->addSeries(results_series);
    plot->addSeries(inputs_series);
    plot->createDefaultAxes();
}

void MainWindow::reset_sim(bool incl_generators)
{
    if (incl_generators)
        panel_generators->reset_sim();
    real_outputs.clear();
    given_inputs.clear();
    loop.reset();
    refresh_editor();
}

void MainWindow::simulate_manual()
{
    if (last_source != sources::MANUAL)
        reset_sim(true);

    last_source = sources::MANUAL;
    simulate({});
}

void MainWindow::simulate_gen(std::vector<double> inputs)
{
    if (last_source != sources::GENERATOR)
        reset_sim();

    last_source = sources::GENERATOR;
    simulate(inputs);
}

std::pair<std::unique_ptr<uint8_t[]>, std::size_t>
MainWindow::read_file(const std::filesystem::path &path)
{
    std::ifstream in{ path, std::ios::binary };
    in.seekg(0, std::ios::end);
    const auto file_size = in.tellg();
    in.seekg(0, std::ios::beg);
    auto buff = std::make_unique_for_overwrite<uint8_t[]>(static_cast<std::size_t>(file_size));
    in.read(reinterpret_cast<char *>(buff.get()), file_size);
    in.close();
    return { std::move(buff), file_size };
}

void MainWindow::open_file()
{
    QString filter{ "All supported files (*.lmod *.gens)" };
    const auto filename = QFileDialog::getOpenFileName(
        this, "Select file", QDir::currentPath(),
        "Loop model (*.lmod);;Generators (*.gens);;All supported files (*.lmod *.gens)", &filter);
    if (filename.isEmpty())
        return;
    const fs::path path{ filename.toStdU16String() };
    const auto ext = path.extension();
    if (ext != ".lmod" && ext != ".gens")
        return;
    const auto [buff, file_size] = read_file(path);
    if (ext == ".lmod") {
        tree_model->begin_reset();
        loop = PętlaUAR{ std::span(buff.get(), file_size) };
        tree_model->end_reset();
        tree_view->expandAll();
    } else {
        panel_generators->import(std::span(buff.get(), file_size));
    }
}

void MainWindow::export_model()
{
    QString filter{ "Loop model (*.lmod)" };
    const auto filename = QFileDialog::getSaveFileName(this, "Choose a filename to export to",
                                                       QDir::currentPath(), filter, &filter)
                              .toStdU16String();
    if (filename.empty())
        return;
    fs::path path{ filename };
    const auto ext = path.extension();
    if (ext != ".lmod")
        path.replace_extension(".lmod");

    const auto dump = loop.dump();
    std::ofstream out{ path, std::ios::out | std::ios::trunc | std::ios::binary };
    out.write(reinterpret_cast<const char *>(dump.data()), static_cast<int64_t>(dump.size()));
    out.flush();
}

void MainWindow::export_generators()
{
    if (panel_generators->empty()) {
        QMessageBox message_box{ QMessageBox::Icon::Warning, "Problem",
                                 "There are no generators configured.",
                                 QMessageBox::StandardButton::Close };
        message_box.exec();
        return;
    }
    QString filter{ "Generators (*.gens)" };
    const auto filename = QFileDialog::getSaveFileName(this, "Choose a filename to export to",
                                                       QDir::currentPath(), filter, &filter)
                              .toStdU16String();
    if (filename.empty())
        return;
    fs::path path{ filename };
    const auto ext = path.extension();
    if (ext != ".gens")
        path.replace_extension(".gens");

    const auto dump = panel_generators->dump();
    std::ofstream out{ path, std::ios::out | std::ios::trunc | std::ios::binary };
    out.write(reinterpret_cast<const char *>(dump.data()), static_cast<int64_t>(dump.size()));
    out.flush();
}

void MainWindow::import_model()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Select loop model file",
                                                       QDir::currentPath(), "Loop model (*.lmod)");
    if (filename.isEmpty())
        return;
    const auto [buff, file_size] = read_file(filename.toStdU16String());
    tree_model->begin_reset();
    loop = PętlaUAR{ std::span(buff.get(), file_size) };
    tree_model->end_reset();
    tree_view->expandAll();
}

void MainWindow::import_generators()
{
    const auto filename = QFileDialog::getOpenFileName(this, "Select generators file",
                                                       QDir::currentPath(), "Generators (*.gens)");
    if (filename.isEmpty())
        return;
    const auto [buff, file_size] = read_file(filename.toStdU16String());
    panel_generators->import(std::span(buff.get(), file_size));
}

void MainWindow::remove_component()
{
    const auto sel_idx = tree_view->selectionModel()->currentIndex();
    const auto parent = tree_model->parent(sel_idx);
    if (!parent.isValid())
        return;
    const auto idx_as_loop
        = dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(sel_idx.internalPointer()));
    const auto has_children = idx_as_loop && idx_as_loop->size() > 0;
    const auto response = QMessageBox::question(
        this, "Remove component",
        has_children ? "Do you want to remove the selected component and all of its children?"
                     : "Do you want to remove the selected component?");
    if (response == QMessageBox::StandardButton::Yes) {
        tree_model->removeRow(parent, sel_idx.row());
    }
}

void MainWindow::change_active_editor(const QModelIndex &index)
{
    if (!index.isValid()) {
        layout_param_editors->setCurrentIndex(0);
        return;
    }
    auto ptr = static_cast<ObiektSISO *>(index.internalPointer());
    if (auto p = dynamic_cast<RegulatorPID *>(ptr)) {
        editor_pid->update_from(*p);
        layout_param_editors->setCurrentIndex(1);
    } else if (auto p = dynamic_cast<ObiektStatyczny *>(ptr)) {
        editor_static->update_from(*p);
        layout_param_editors->setCurrentIndex(2);
    } else if (auto p = dynamic_cast<ModelARX *>(ptr)) {
        editor_arx->update_from(*p);
        layout_param_editors->setCurrentIndex(3);
    } else if (auto p = dynamic_cast<PętlaUAR *>(ptr)) {
        editor_uar->update_from(*p);
        layout_param_editors->setCurrentIndex(4);
    } else {
        layout_param_editors->setCurrentIndex(0);
    }
}

void MainWindow::refresh_editor()
{
    change_active_editor(tree_view->selectionModel()->currentIndex());
}

void MainWindow::update_tree_actions(const QModelIndex &index)
{
    if (!index.isValid()) {
        submenu_append_child->setEnabled(false);
        submenu_insert_component->setEnabled(false);
        action_remove_component->setEnabled(false);
        return;
    }
    submenu_append_child->setEnabled(
        dynamic_cast<PętlaUAR *>(static_cast<ObiektSISO *>(index.internalPointer())) != nullptr);
    const auto parent_is_valid = tree_model->parent(index).isValid();
    submenu_insert_component->setEnabled(parent_is_valid);
    action_remove_component->setEnabled(parent_is_valid);
}

void MainWindow::on_tree_selection_change()
{
    const auto idx = tree_view->selectionModel()->currentIndex();
    update_tree_actions(idx);
    change_active_editor(idx);
    button_save_params->setEnabled(idx.isValid());
}

void MainWindow::save_params()
{
    const auto sel_idx = tree_view->selectionModel()->currentIndex();
    if (!sel_idx.isValid())
        return;
    const auto ptr = static_cast<ObiektSISO *>(sel_idx.internalPointer());
    const auto editor_idx = layout_param_editors->currentIndex();
    if (editor_idx == 1) {
        update_from_editor<RegulatorPID>(editor_pid, ptr);
    } else if (editor_idx == 2) {
        update_from_editor<ObiektStatyczny>(editor_static, ptr);
    } else if (editor_idx == 3) {
        update_from_editor<ModelARX>(editor_arx, ptr);
    } else if (editor_idx == 4) {
        update_from_editor<PętlaUAR>(editor_uar, ptr);
    }
}

void MainWindow::start()
{
    setup_ui();
    setWindowTitle("PO lab 2");
    show();
}
