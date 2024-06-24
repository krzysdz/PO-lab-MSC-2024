#pragma once
#include "../../lab1/ModelARX.h"
#include "../../lab3/generators.hpp"
#include "../RegulatorPID.h"
#include "GeneratorsConfig.hpp"
#include <QAction>
#include <QChart>
#include <QChartView>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSessionManager>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <optional>
#include <vector>
#include "../../lab4/PętlaUAR.hpp"

class MainWindow : public QMainWindow {
private:
    QMenu *menu_file;
    QMenu *submenu_export;
    QMenu *submenu_import;
    QAction *action_exit;
    QAction *action_export_model;
    QAction *action_export_pid;
    QAction *action_import_model;
    QAction *action_import_pid;
    QWidget *widget_right;
    QWidget *widget_parameters;
    QWidget *widget_inputs;
    QTabWidget *tabs_input;
    QSplitter *main_columns_splitter;
    QFormLayout *layout_parameters;
    QVBoxLayout *layout_right_col;
    QChart *plot;
    QChartView *chart_view;
    QDoubleSpinBox *input_pid_k;
    QDoubleSpinBox *input_pid_ti;
    QDoubleSpinBox *input_pid_td;
    QDoubleSpinBox *input_static_x1;
    QDoubleSpinBox *input_static_y1;
    QDoubleSpinBox *input_static_x2;
    QDoubleSpinBox *input_static_y2;
    QSpinBox *input_arx_delay;
    QDoubleSpinBox *input_arx_stddev;
    QLineEdit *input_arx_coeff_a;
    QLineEdit *input_arx_coeff_b;
    QPushButton *button_apply_params;
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
    void configure_loop();
    void simulate(const std::vector<double> &out_inputs);
    void plot_results();
    void reset_sim(bool incl_generators = false);

    void simulate_manual();
    void simulate_gen(std::vector<double> inputs);

    // void export_model();
    // void export_pid();
    // void import_model();
    // void import_pid();

public:
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : QMainWindow{ parent, flags }
    {
    }
    void start();
};
