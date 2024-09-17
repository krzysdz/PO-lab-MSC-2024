/// @file GeneratorsConfig.hpp
/// @brief Widgets and structures used for configuring @link ::Generator generators@endlink

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

#ifdef IE_TESTS
class ImportExportTest;
#endif

/// Helper struct mapping UI names with appropriate editor functions.
struct gen_details {
    /// Human-readable generator name.
    QString name;
    /// Appropriate editor function.
    std::function<std::string(Generator *)> dialog_add;
};

/// Widget with options for configuring and simulating @link ::Generator generators@endlink.
class GeneratorsConfig : public QWidget {
    Q_OBJECT

private:
    /// Main widget layout.
    QVBoxLayout *layout_main;
    /// Layout of the _add generator_ section.
    QHBoxLayout *layout_add;
    /// Layout with existing generators and edit options.
    QHBoxLayout *layout_edit;
    /// Layout with simulation options.
    QHBoxLayout *layout_simulate;
    /// Combobox with list of generators available to add.
    QComboBox *combo_add;
    /// Combobox with list of existing generators. Items' data is appropriate gen_details::name.
    QComboBox *combo_edit;
    /// Spinbox with number of ticks to simulate.
    QSpinBox *spinbox_time;
    /// Button add generator.
    QPushButton *button_add;
    /// Button edit generator.
    QPushButton *button_edit;
    /// Button delete generator.
    QPushButton *button_delete;
    /// Button launching simulation of generators.
    QPushButton *button_simulate;
    /// Button removing all generators.
    QPushButton *button_clear;
    /// Label with current simulation time.
    QLabel *label_time;
    /// Errror dialog (shown if an error occurs).
    QErrorMessage *popup_error;

    // UI
    /// Initialize and lay out components, connect appropriate signals.
    void prepare_layout();
    /// Update list of available options in #combo_add based on whether any generator is configured.
    void populate_generators();
    /// @brief Enable or disable action buttons and repopulate #combo_add (using
    /// #populate_generators())
    /// @param initialized whether any generator is configured (#generator `!= nullptr`)
    void set_initialized(bool initialized);
    /// Update displayed simulation time (#label_time) with current time (#simulation_time).
    void update_sim_time();

    // Actions
    /// Add a generator created by calling a function from #gen_options based on #combo_add.
    void action_add();
    /// Edit currently selected generator using an appropriate function from #gen_options.
    void action_edit();
    /// Delete currently selected (#combo_edit) generator.
    void action_delete();
    /// Simulate N ticks (#spinbox_time). Result is emitted as #simulated signal.
    void action_simulate();
    /// @brief Remove all generators and reset simulation time.
    /// @details Emits #cleared signal.
    void action_clear();
    /// Handle #combo_edit index changes and update buttons appropriately.
    void action_current_gen_changed();

    // Logic
    /// @brief Get pointer to the base generator.
    /// @return Pointer to base GeneratorBaza generator. `nullptr` if there are no generators.
    GeneratorBaza *get_base() const;
    /// @brief Get pointer to N-th preceding generator starting from currently configured.
    /// @param n_back number of generators to go back from #generator
    /// @return Pointer to n-th generator or `nullptr` if it does not exist.
    Generator *get_nth_generator(int n_back) const;
    /// @brief Create or edit base generator (GeneratorBaza) based on user input in shown dialog.
    /// @return String dectibing the generator type and its properties.
    std::string edit_base();
    /// @brief Create or edit sine wave generator (GeneratorSinus) based on user input in shown
    /// dialog.
    /// @param existing pointer to GeneratorSinus to edit or `nullptr` to create a new one
    /// @return String dectibing the generator type and its properties.
    std::string add_sin(Generator *existing);
    /// @brief Create or edit square wave generator (GeneratorProstokat) based on user input in
    /// shown dialog.
    /// @param existing pointer to GeneratorProstokat to edit or `nullptr` to create a new one
    /// @return String dectibing the generator type and its properties.
    std::string add_pwm(Generator *existing);
    /// @brief Create or edit sawtooth wave generator (GeneratorSawtooth) based on user input in
    /// shown dialog.
    /// @param existing pointer to GeneratorSawtooth to edit or `nullptr` to create a new one
    /// @return String dectibing the generator type and its properties.
    std::string add_saw(Generator *existing);
    /// @brief Create or edit uniform noise generator (GeneratorUniformNoise) based on user input
    /// in shown dialog.
    /// @param existing pointer to GeneratorUniformNoise to edit or `nullptr` to create a new one
    /// @return String dectibing the generator type and its properties.
    std::string add_uniform(Generator *existing);
    /// @brief Create or edit normal-distributed noise generator (GeneratorNormalNoise) based on
    /// user input in shown dialog.
    /// @param existing pointer to GeneratorNormalNoise to edit or `nullptr` to create a new one
    /// @return String dectibing the generator type and its properties.
    std::string add_normal(Generator *existing);
    /// Recreate #combo_edit's list of configured generators.
    void recreate_list();

    /// Current simulation time.
    int simulation_time{};
    /// Currently configured generator (may be a decorator over other generators).
    std::unique_ptr<Generator> generator{};
    /// Vector of available non-base, decorator generators and their respective @e add_ functions.
    std::vector<gen_details> gen_options{
        { "Sine", std::bind_front(&GeneratorsConfig::add_sin, this) },
        { "Rectangular", std::bind_front(&GeneratorsConfig::add_pwm, this) },
        { "Sawtooth", std::bind_front(&GeneratorsConfig::add_saw, this) },
        { "Uniform noise", std::bind_front(&GeneratorsConfig::add_uniform, this) },
        { "Normal noise", std::bind_front(&GeneratorsConfig::add_normal, this) }
    };

public:
    /// @brief Create GeneratorsConfig widget with given parent and window flags.
    /// @see QWidget::QWidget()
    explicit GeneratorsConfig(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    /// Reset simulation time.
    void reset_sim();
    /// Check if there are no configured generators.
    bool empty() const { return generator == nullptr; }
    /// @brief Serialize the underlying topmost generator (all configured generators).
    /// @return A vector of bytes (`uint8_t`) from which the generator can be reconstructed.
    std::vector<uint8_t> dump() const;
    /// @brief Deserialize generator from an input range of bytes and set it as #generator
    /// @tparam T type of the input range of bytes
    /// @param serialized byte representation of a Generator derived class
    /// @see Generator::deserialize()
    /// @see GeneratorsConfig::import(std::unique_ptr<Generator> &&)
    template <std::ranges::input_range T>
        requires ByteRepr<std::ranges::range_value_t<T>>
    void import(const T &serialized)
    {
        import(Generator::deserialize(serialized));
    }
    /// @brief Change root generator, reset simulation and update buttons.
    /// @param gen new generator
    void import(std::unique_ptr<Generator> &&gen);

signals:
    /// Emitted when simulation is performed with results as argument.
    void simulated(std::vector<double> outputs);
    /// Emitted when all existing generators are removed (#action_clear())
    void cleared();

#ifdef IE_TESTS
    friend class ImportExportTest;
#endif
};
