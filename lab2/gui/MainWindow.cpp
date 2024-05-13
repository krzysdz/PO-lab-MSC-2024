#include "MainWindow.hpp"
#include "../feedback_loop.hpp"
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

namespace fs = std::filesystem;

void MainWindow::setup_ui()
{
    prepare_menu_bar();
    prepare_layout();
}

void MainWindow::prepare_menu_bar()
{
    menu_file = menuBar()->addMenu("&File");

    // Export
    submenu_export = menu_file->addMenu("Export");

    action_export_model = submenu_export->addAction("ARX model");
    connect(action_export_model, &QAction::triggered, this, &MainWindow::export_model);

    action_export_pid = submenu_export->addAction("PID regulator");
    connect(action_export_pid, &QAction::triggered, this, &MainWindow::export_pid);

    // Import
    submenu_import = menu_file->addMenu("Import");

    action_import_model = submenu_import->addAction("ARX model");
    connect(action_import_model, &QAction::triggered, this, &MainWindow::import_model);

    action_import_pid = submenu_import->addAction("PID regulator");
    connect(action_import_pid, &QAction::triggered, this, &MainWindow::import_pid);

    // Others
    action_exit = menu_file->addAction("Exit");
    action_exit->setShortcut(QKeySequence::Quit);
    connect(action_exit, &QAction::triggered, &QApplication::quit);
}

void MainWindow::prepare_layout()
{
    main_columns_splitter = new QSplitter{ this };
    main_columns_splitter->setChildrenCollapsible(false);
    setCentralWidget(main_columns_splitter);

    // Parameters section - left part of the window
    widget_parameters = new QWidget{ main_columns_splitter };
    main_columns_splitter->addWidget(widget_parameters);

    layout_parameters = new QFormLayout{ widget_parameters };

    const auto pid_label = new QLabel{ "<b>PID parameters</b>", widget_parameters };
    layout_parameters->setWidget(0, QFormLayout::ItemRole::SpanningRole, pid_label);

    input_pid_k = new QDoubleSpinBox{ widget_parameters };
    input_pid_k->setMinimum(0.0);
    input_pid_k->setSingleStep(0.1);
    layout_parameters->addRow("k", input_pid_k);

    input_pid_ti = new QDoubleSpinBox{ widget_parameters };
    input_pid_ti->setMinimum(0.0);
    layout_parameters->addRow("Ti", input_pid_ti);

    input_pid_td = new QDoubleSpinBox{ widget_parameters };
    input_pid_td->setMinimum(0.0);
    layout_parameters->addRow("Td", input_pid_td);

    const auto param_sep = new QFrame{ widget_parameters };
    param_sep->setFrameShape(QFrame::Shape::HLine);
    param_sep->setFrameShadow(QFrame::Shadow::Sunken);
    layout_parameters->setWidget(4, QFormLayout::ItemRole::SpanningRole, param_sep);

    const auto arx_label = new QLabel{ "<b>ARX parameters</b>", widget_parameters };
    layout_parameters->setWidget(5, QFormLayout::ItemRole::SpanningRole, arx_label);

    input_arx_delay = new QSpinBox{ widget_parameters };
    input_arx_delay->setMinimum(1);
    layout_parameters->addRow("Delay", input_arx_delay);

    input_arx_stddev = new QDoubleSpinBox{ widget_parameters };
    input_arx_stddev->setMinimum(0.0);
    input_arx_stddev->setSingleStep(0.1);
    layout_parameters->addRow("stddev", input_arx_stddev);

    QRegularExpression coeff_pattern{ R"(^\s*(?:-?\d+(?:\.\d+)?(?:\s*?,\s*?|\s*?$))*$)" };
    const auto coeff_validator
        = new QRegularExpressionValidator{ coeff_pattern, widget_parameters };

    input_arx_coeff_a = new QLineEdit{ widget_parameters };
    input_arx_coeff_a->setToolTip("Comma separated list of coefficients");
    input_arx_coeff_a->setValidator(coeff_validator);
    layout_parameters->addRow("<b>A</b> coefficients", input_arx_coeff_a);

    input_arx_coeff_b = new QLineEdit{ widget_parameters };
    input_arx_coeff_b->setToolTip("Comma separated list of coefficients");
    input_arx_coeff_b->setValidator(coeff_validator);
    layout_parameters->addRow("<b>B</b> coefficients", input_arx_coeff_b);

    button_apply_params = new QPushButton{ "Apply parameters", widget_parameters };
    button_apply_params->setToolTip("Create a new feedback loop model with provided parameters");
    layout_parameters->setWidget(10, QFormLayout::ItemRole::SpanningRole, button_apply_params);
    connect(button_apply_params, &QPushButton::released, this, &MainWindow::configure_loop);

    // Right part of the window
    widget_right = new QWidget{ main_columns_splitter };
    main_columns_splitter->addWidget(widget_right);
    layout_right_col = new QVBoxLayout{ widget_right };

    // Inputs and simulation options
    widget_inputs = new QWidget{ widget_right };
    layout_inputs = new QGridLayout{ widget_inputs };
    layout_right_col->addWidget(widget_inputs);

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
    connect(button_simulate, &QPushButton::released, this, &MainWindow::simulate);

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

void MainWindow::configure_loop()
{
    auto coeff_a = parse_coefficients(input_arx_coeff_a->text());
    auto coeff_b = parse_coefficients(input_arx_coeff_b->text());
    const auto delay = input_arx_delay->value();
    const auto stddev = input_arx_stddev->value();
    const auto k = input_pid_k->value();
    const auto Ti = input_pid_ti->value();
    const auto Td = input_pid_td->value();
    model_opt.emplace(std::move(coeff_a), std::move(coeff_b), delay, stddev);
    regulator_opt.emplace(k, Ti, Td);
    should_reset = true;
}

void MainWindow::simulate()
{
    if (!regulator_opt.has_value() || !model_opt.has_value()) {
        QMessageBox message_box{ QMessageBox::Icon::Warning, "Problem",
                                 "Enter correct model and regulator parameters and click the "
                                 "<b>Apply parameters</b> button before simulating",
                                 QMessageBox::StandardButton::Close };
        message_box.exec();
        return;
    }
    std::optional<double> reset_opt{};
    if (should_reset) {
        reset_opt = 0.0;
        should_reset = false;
        real_outputs.clear();
        given_inputs.clear();
    }
    const auto inputs = parse_coefficients(input_inputs->text());
    const auto repetitions = input_repetitions->value();
    for (int i = 0; i < repetitions; ++i) {
        for (const auto &input : inputs) {
            const auto out
                = feedback_step(regulator_opt.value(), model_opt.value(), input, reset_opt);
            real_outputs.push_back(out);
            reset_opt.reset();
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

void MainWindow::export_model()
{
    if (!model_opt.has_value()) {
        QMessageBox message_box{ QMessageBox::Icon::Warning, "Problem",
                                 "Model is not defined. Make sure to click <b>Apply parameters</b> "
                                 "button before exporting.",
                                 QMessageBox::StandardButton::Close };
        message_box.exec();
        return;
    }

    QString filter{ "ARX binary model (*.arx);;ARX text model (*.arxt)" };
    const auto filename = QFileDialog::getSaveFileName(this, "Choose a filename to export to",
                                                       QDir::currentPath(), filter, &filter)
                              .toStdU16String();
    if (filename.empty())
        return;
    fs::path path{ filename };
    const auto ext = path.extension();
    if (ext != ".arx" && ext != ".arxt")
        path.replace_extension(".arx");
    if (filename.back() == u't') {
        std::ofstream out{ path, std::ios::out | std::ios::trunc };
        out << model_opt.value();
    } else {
        const auto dump = model_opt->dump();
        std::ofstream out{ path, std::ios::out | std::ios::trunc | std::ios::binary };
        out.write(reinterpret_cast<const char *>(dump.data()), static_cast<int64_t>(dump.size()));
        out.flush();
    }
}

void MainWindow::export_pid()
{
    if (!regulator_opt.has_value()) {
        QMessageBox message_box{ QMessageBox::Icon::Warning, "Problem",
                                 "Regulator is not defined. Make sure to click <b>Apply "
                                 "parameters</b> button before exporting.",
                                 QMessageBox::StandardButton::Close };
        message_box.exec();
        return;
    }

    QString filter{ "PID binary model (*.pid);;PID text model (*.pidt)" };
    const auto filename = QFileDialog::getSaveFileName(this, "Choose a filename to export to",
                                                       QDir::currentPath(), filter, &filter)
                              .toStdU16String();
    if (filename.empty())
        return;
    fs::path path{ filename };
    const auto ext = path.extension();
    if (ext != ".pid" && ext != ".pidt")
        path.replace_extension(".pid");
    if (filename.back() == u't') {
        std::ofstream out{ path, std::ios::out | std::ios::trunc };
        out << regulator_opt.value();
    } else {
        const auto dump = regulator_opt->dump();
        std::ofstream out{ path, std::ios::out | std::ios::trunc | std::ios::binary };
        out.write(reinterpret_cast<const char *>(dump.data()), static_cast<int64_t>(dump.size()));
        out.flush();
    }
}

void MainWindow::import_model()
{
    const auto filename = QFileDialog::getOpenFileName(
        this, "Select ARX model file", QDir::currentPath(),
        "ARX model (*.arx *.arxt);;ARX binary model (*.arx);;ARX text model (*.arxt)");
    if (filename.isEmpty())
        return;
    const fs::path path{ filename.toStdU16String() };
    if (filename.back() == u't') {
        std::ifstream in{ path };
        ModelARX m{ {}, {}, 1 };
        in >> m;
        model_opt = std::move(m);
    } else {
        std::ifstream in{ path, std::ios::binary };
        in.seekg(0, std::ios::end);
        const auto file_size = in.tellg();
        in.seekg(0, std::ios::beg);
        const auto buff = std::make_unique_for_overwrite<uint8_t[]>(static_cast<size_t>(file_size));
        in.read(reinterpret_cast<char *>(buff.get()), file_size);
        in.close();
        model_opt.emplace(buff.get(), buff.get() + file_size);
    }
}

void MainWindow::import_pid()
{
    const auto filename = QFileDialog::getOpenFileName(
        this, "Select PID regulator file", QDir::currentPath(),
        "PID regulator (*.pid *.pidt);;PID binary model (*.pid);;PID text model (*.pidt)");
    if (filename.isEmpty())
        return;
    const fs::path path{ filename.toStdU16String() };
    if (filename.back() == u't') {
        std::ifstream in{ path };
        RegulatorPID pid{ 0 };
        in >> pid;
        regulator_opt = std::move(pid);
    } else {
        std::ifstream in{ path, std::ios::binary };
        in.seekg(0, std::ios::end);
        const auto file_size = in.tellg();
        in.seekg(0, std::ios::beg);
        const auto buff = std::make_unique_for_overwrite<uint8_t[]>(static_cast<size_t>(file_size));
        in.read(reinterpret_cast<char *>(buff.get()), file_size);
        in.close();
        regulator_opt.emplace(buff.get(), buff.get() + file_size);
    }
}

void MainWindow::start()
{
    setup_ui();
    setWindowTitle("PO lab 2");
    show();
}
