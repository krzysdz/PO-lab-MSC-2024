#pragma once
#include "../generators.hpp"
#include <QComboBox>
#include <QDialog>
#include <QErrorMessage>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <unordered_map>

struct gen_details {
    QString name;
    std::function<std::string(Generator *)> dialog_add;
};

class GeneratorsConfig : public QWidget {
    Q_OBJECT

private:
    QVBoxLayout *layout_main;
    QHBoxLayout *layout_add;
    QHBoxLayout *layout_edit;
    QHBoxLayout *layout_simulate;
    QComboBox *combo_add;
    QComboBox *combo_edit;
    QSpinBox *spinbox_time;
    QPushButton *button_add;
    QPushButton *button_edit;
    QPushButton *button_delete;
    QPushButton *button_simulate;
    QPushButton *button_clear;
    QLabel *label_time;
    QErrorMessage *popup_error;

    // UI
    void prepare_layout();
    void populate_generators();
    void set_initialized(bool initialized);
    void update_sim_time();

    // Actions
    void action_add();
    void action_edit();
    void action_delete();
    void action_simulate();
    void action_clear();
    void action_current_gen_changed();

    // Logic
    GeneratorBaza *get_base() const;
    Generator *get_nth_generator(int n_back) const;
    std::string edit_base();
    std::string add_sin(Generator *existing);
    std::string add_pwm(Generator *existing);
    std::string add_saw(Generator *existing);
    std::string add_uniform(Generator *existing);
    std::string add_normal(Generator *existing);

    int simulation_time{};
    std::unique_ptr<Generator> generator{};
    QStringList configured_generators{};
    std::vector<gen_details> gen_options{
        { "Sine", std::bind_front(&GeneratorsConfig::add_sin, this) },
        { "Rectangular", std::bind_front(&GeneratorsConfig::add_pwm, this) },
        { "Sawtooth", std::bind_front(&GeneratorsConfig::add_saw, this) },
        { "Uniform noise", std::bind_front(&GeneratorsConfig::add_uniform, this) },
        { "Normal noise", std::bind_front(&GeneratorsConfig::add_normal, this) }
    };

public:
    explicit GeneratorsConfig(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    void reset_sim();

signals:
    void simulated(std::vector<double> outputs);
    void cleared();
};
