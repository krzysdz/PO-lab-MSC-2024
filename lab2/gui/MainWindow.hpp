#pragma once
#include "../../lab1/ModelARX.h"
#include "../RegulatorPID.h"
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
#include <QVBoxLayout>
#include <optional>

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
    QSplitter *main_columns_splitter;
    QFormLayout *layout_parameters;
    QVBoxLayout *layout_right_col;
    QChart *plot;
    QChartView *chart_view;
    QDoubleSpinBox *input_pid_k;
    QDoubleSpinBox *input_pid_ti;
    QDoubleSpinBox *input_pid_td;
    QSpinBox *input_arx_delay;
    QDoubleSpinBox *input_arx_stddev;
    QLineEdit *input_arx_coeff_a;
    QLineEdit *input_arx_coeff_b;
    QPushButton *button_apply_params;
    QGridLayout *layout_inputs;
    QLineEdit *input_inputs;
    QSpinBox *input_repetitions;
    QPushButton *button_simulate;

    std::optional<RegulatorPID> regulator_opt;
    std::optional<ModelARX> model_opt;
    bool should_reset;
    std::vector<double> real_outputs;
    std::vector<double> given_inputs;

    void setup_ui();
    void prepare_menu_bar();
    void prepare_layout();
    std::vector<double> parse_coefficients(const QString &coeff_text);
    void configure_loop();
    void simulate();
    void plot_results();

    void export_model();
    void export_pid();
    void import_model();
    void import_pid();

public:
    explicit MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : QMainWindow{ parent, flags }
    {
    }
    void start();
};
