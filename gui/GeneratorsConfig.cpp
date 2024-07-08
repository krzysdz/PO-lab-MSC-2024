#include "GeneratorsConfig.hpp"
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QSpinBox>
#include <limits>

GeneratorBaza *GeneratorsConfig::get_base() const
{
    if (!generator)
        return nullptr;

    auto gen = generator.get();
    auto decor = dynamic_cast<GeneratorDecor *>(gen);
    while (decor) {
        gen = decor->m_base.get();
        decor = dynamic_cast<GeneratorDecor *>(gen);
    }
    auto base = dynamic_cast<GeneratorBaza *>(gen);
    Q_ASSERT(base != nullptr);
    return base;
}

Generator *GeneratorsConfig::get_nth_generator(int n_back) const
{
    if (!generator)
        return nullptr;
    auto gen = generator.get();
    for (int i = 0; i < n_back; ++i) {
        auto as_dec = dynamic_cast<GeneratorDecor *>(gen);
        if (as_dec == nullptr)
            return nullptr;
        gen = as_dec->m_base.get();
    }
    return gen;
}

std::string GeneratorsConfig::edit_base()
{
    auto base = get_base();
    QDialog dialog{ this };
    auto layout = new QFormLayout{ &dialog };
    auto input_amplitude = new QDoubleSpinBox{ &dialog };
    auto input_t_start = new QSpinBox{ &dialog };
    auto input_t_end = new QSpinBox{ &dialog };
    auto dialog_buttons
        = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog };

    input_amplitude->setMaximum(std::numeric_limits<double>::max());
    input_t_start->setMaximum(std::numeric_limits<int>::max());
    input_t_end->setMaximum(std::numeric_limits<int>::max());
    if (base) {
        const auto [ts, te] = base->get_activity_time();
        input_amplitude->setValue(base->get_amplitude());
        input_t_start->setValue(ts);
        input_t_end->setValue(te);
    }

    connect(dialog_buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(dialog_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.setLayout(layout);
    layout->addRow("Value", input_amplitude);
    layout->addRow("Start time", input_t_start);
    layout->addRow("End time", input_t_end);
    layout->addRow(dialog_buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto a = input_amplitude->value();
        const auto ts = input_t_start->value();
        const auto te = input_t_end->value();
        if (base) {
            base->set_amplitude(a);
            base->set_activity_time(ts, te);
        } else {
            generator = std::make_unique<GeneratorBaza>(a, ts, te);
            set_initialized(true);
        }
        return std::format("Base [V={}{}]", a,
                           ts == 0 && te == 0 ? "" : std::format(" <{}-{}>", ts, te));
    }
    return {};
}

std::string GeneratorsConfig::add_sin(Generator *_existing)
{
    auto existing = dynamic_cast<GeneratorSinus *>(_existing);
    QDialog dialog{ this };
    auto layout = new QFormLayout{ &dialog };
    auto input_amplitude = new QDoubleSpinBox{ &dialog };
    auto input_period = new QSpinBox{ &dialog };
    auto input_t_start = new QSpinBox{ &dialog };
    auto input_t_end = new QSpinBox{ &dialog };
    auto dialog_buttons
        = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog };

    input_amplitude->setMaximum(std::numeric_limits<double>::max());
    input_period->setRange(
        1,
        std::min<int64_t>(std::numeric_limits<uint32_t>::max(), std::numeric_limits<int>::max()));
    input_t_start->setMaximum(std::numeric_limits<int>::max());
    input_t_end->setMaximum(std::numeric_limits<int>::max());
    if (existing) {
        const auto [ts, te] = existing->get_activity_time();
        input_amplitude->setValue(existing->get_amplitude());
        input_period->setValue(existing->get_period());
        input_t_start->setValue(ts);
        input_t_end->setValue(te);
    }

    connect(dialog_buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(dialog_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.setLayout(layout);
    layout->addRow("Amplitude", input_amplitude);
    layout->addRow("Period", input_period);
    layout->addRow("Start time", input_t_start);
    layout->addRow("End time", input_t_end);
    layout->addRow(dialog_buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto a = input_amplitude->value();
        const auto p = input_period->value();
        const auto ts = input_t_start->value();
        const auto te = input_t_end->value();
        if (existing) {
            existing->set_amplitude(a);
            existing->set_period(p);
            existing->set_activity_time(ts, te);
        } else {
            generator = std::make_unique<GeneratorSinus>(std::move(generator), a, p, ts, te);
        }
        return std::format("Sine [A={} T={}{}]", a, p,
                           ts == 0 && te == 0 ? "" : std::format(" <{}-{}>", ts, te));
    }
    return {};
}

std::string GeneratorsConfig::add_pwm(Generator *_existing)
{
    auto existing = dynamic_cast<GeneratorProstokat *>(_existing);
    QDialog dialog{ this };
    auto layout = new QFormLayout{ &dialog };
    auto input_amplitude = new QDoubleSpinBox{ &dialog };
    auto input_period = new QSpinBox{ &dialog };
    auto input_duty = new QDoubleSpinBox{ &dialog };
    auto input_t_start = new QSpinBox{ &dialog };
    auto input_t_end = new QSpinBox{ &dialog };
    auto dialog_buttons
        = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog };

    input_amplitude->setMaximum(std::numeric_limits<double>::max());
    input_period->setRange(
        1,
        std::min<int64_t>(std::numeric_limits<uint32_t>::max(), std::numeric_limits<int>::max()));
    input_duty->setSingleStep(0.05);
    input_duty->setValue(0.5);
    input_duty->setRange(std::numeric_limits<double>::min(), std::nextafter(1.0, 0.0));
    input_t_start->setMaximum(std::numeric_limits<int>::max());
    input_t_end->setMaximum(std::numeric_limits<int>::max());
    if (existing) {
        const auto [ts, te] = existing->get_activity_time();
        input_amplitude->setValue(existing->get_amplitude());
        input_period->setValue(existing->get_period());
        input_duty->setValue(existing->get_duty_cycle());
        input_t_start->setValue(ts);
        input_t_end->setValue(te);
    }

    connect(dialog_buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(dialog_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.setLayout(layout);
    layout->addRow("Amplitude", input_amplitude);
    layout->addRow("Period", input_period);
    layout->addRow("Duty cycle", input_duty);
    layout->addRow("Start time", input_t_start);
    layout->addRow("End time", input_t_end);
    layout->addRow(dialog_buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto a = input_amplitude->value();
        const auto p = input_period->value();
        const auto d = input_duty->value();
        const auto ts = input_t_start->value();
        const auto te = input_t_end->value();
        if (existing) {
            existing->set_amplitude(a);
            existing->set_period(p);
            existing->set_duty_cycle(d);
            existing->set_activity_time(ts, te);
        } else {
            generator = std::make_unique<GeneratorProstokat>(std::move(generator), a, p, d, ts, te);
        }
        return std::format("Rectangular [A={} T={} D={}% {}]", a, p, d * 100,
                           ts == 0 && te == 0 ? "" : std::format(" <{}-{}>", ts, te));
    }
    return {};
}

std::string GeneratorsConfig::add_saw(Generator *_existing)
{
    auto existing = dynamic_cast<GeneratorSawtooth *>(_existing);
    QDialog dialog{ this };
    auto layout = new QFormLayout{ &dialog };
    auto input_amplitude = new QDoubleSpinBox{ &dialog };
    auto input_period = new QSpinBox{ &dialog };
    auto input_t_start = new QSpinBox{ &dialog };
    auto input_t_end = new QSpinBox{ &dialog };
    auto dialog_buttons
        = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog };

    input_amplitude->setMaximum(std::numeric_limits<double>::max());
    input_period->setRange(
        1,
        std::min<int64_t>(std::numeric_limits<uint32_t>::max(), std::numeric_limits<int>::max()));
    input_t_start->setMaximum(std::numeric_limits<int>::max());
    input_t_end->setMaximum(std::numeric_limits<int>::max());
    if (existing) {
        const auto [ts, te] = existing->get_activity_time();
        input_amplitude->setValue(existing->get_amplitude());
        input_period->setValue(existing->get_period());
        input_t_start->setValue(ts);
        input_t_end->setValue(te);
    }

    connect(dialog_buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(dialog_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.setLayout(layout);
    layout->addRow("Amplitude", input_amplitude);
    layout->addRow("Period", input_period);
    layout->addRow("Start time", input_t_start);
    layout->addRow("End time", input_t_end);
    layout->addRow(dialog_buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto a = input_amplitude->value();
        const auto p = input_period->value();
        const auto ts = input_t_start->value();
        const auto te = input_t_end->value();
        if (existing) {
            existing->set_amplitude(a);
            existing->set_period(p);
            existing->set_activity_time(ts, te);
        } else {
            generator = std::make_unique<GeneratorSawtooth>(std::move(generator), a, p, ts, te);
        }
        return std::format("Sawtooth [A={} T={}{}]", a, p,
                           ts == 0 && te == 0 ? "" : std::format(" <{}-{}>", ts, te));
    }
    return {};
}

std::string GeneratorsConfig::add_uniform(Generator *_existing)
{
    auto existing = dynamic_cast<GeneratorUniformNoise *>(_existing);
    QDialog dialog{ this };
    auto layout = new QFormLayout{ &dialog };
    auto input_amplitude = new QDoubleSpinBox{ &dialog };
    auto input_t_start = new QSpinBox{ &dialog };
    auto input_t_end = new QSpinBox{ &dialog };
    auto dialog_buttons
        = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog };

    input_amplitude->setMaximum(std::numeric_limits<double>::max());
    input_t_start->setMaximum(std::numeric_limits<int>::max());
    input_t_end->setMaximum(std::numeric_limits<int>::max());
    if (existing) {
        const auto [ts, te] = existing->get_activity_time();
        input_amplitude->setValue(existing->get_amplitude());
        input_t_start->setValue(ts);
        input_t_end->setValue(te);
    }

    connect(dialog_buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(dialog_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.setLayout(layout);
    layout->addRow("Amplitude", input_amplitude);
    layout->addRow("Start time", input_t_start);
    layout->addRow("End time", input_t_end);
    layout->addRow(dialog_buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto a = input_amplitude->value();
        const auto ts = input_t_start->value();
        const auto te = input_t_end->value();
        if (existing) {
            existing->set_amplitude(a);
            existing->set_activity_time(ts, te);
        } else {
            generator = std::make_unique<GeneratorUniformNoise>(std::move(generator), a, ts, te);
        }
        return std::format("Uniform noise [A={}{}]", a,
                           ts == 0 && te == 0 ? "" : std::format(" <{}-{}>", ts, te));
    }
    return {};
}

std::string GeneratorsConfig::add_normal(Generator *_existing)
{
    auto existing = dynamic_cast<GeneratorNormalNoise *>(_existing);
    QDialog dialog{ this };
    auto layout = new QFormLayout{ &dialog };
    auto input_mean = new QDoubleSpinBox{ &dialog };
    auto input_stddev = new QDoubleSpinBox{ &dialog };
    auto input_t_start = new QSpinBox{ &dialog };
    auto input_t_end = new QSpinBox{ &dialog };
    auto dialog_buttons
        = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog };

    input_mean->setMaximum(std::numeric_limits<double>::max());
    input_stddev->setMaximum(std::numeric_limits<double>::max());
    input_t_start->setMaximum(std::numeric_limits<int>::max());
    input_t_end->setMaximum(std::numeric_limits<int>::max());
    if (existing) {
        const auto [ts, te] = existing->get_activity_time();
        input_mean->setValue(existing->get_mean());
        input_stddev->setValue(existing->get_stddev());
        input_t_start->setValue(ts);
        input_t_end->setValue(te);
    }

    connect(dialog_buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(dialog_buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    dialog.setLayout(layout);
    layout->addRow("Mean", input_mean);
    layout->addRow("Standard deviation (σ)", input_stddev);
    layout->addRow("Start time", input_t_start);
    layout->addRow("End time", input_t_end);
    layout->addRow(dialog_buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto m = input_mean->value();
        const auto sd = input_stddev->value();
        const auto ts = input_t_start->value();
        const auto te = input_t_end->value();
        if (existing) {
            existing->set_mean(m);
            existing->set_stddev(sd);
            existing->set_activity_time(ts, te);
        } else {
            generator = std::make_unique<GeneratorNormalNoise>(std::move(generator), m, sd, ts, te);
        }
        return std::format("Normal noise [M={} σ={}{}]", m, sd,
                           ts == 0 && te == 0 ? "" : std::format(" <{}-{}>", ts, te));
    }
    return {};
}

void GeneratorsConfig::prepare_layout()
{
    layout_main = new QVBoxLayout{ this };
    setLayout(layout_main);

    // FIRST ROW
    layout_add = new QHBoxLayout{};
    layout_main->addLayout(layout_add);

    combo_add = new QComboBox{ this };
    layout_add->addWidget(combo_add);

    button_add = new QPushButton{ "Add", this };
    connect(button_add, &QPushButton::released, this, &GeneratorsConfig::action_add);
    layout_add->addWidget(button_add);

    // SECOND ROW
    layout_edit = new QHBoxLayout{};
    layout_main->addLayout(layout_edit);

    combo_edit = new QComboBox{ this };
    connect(combo_edit, &QComboBox::currentIndexChanged, this,
            &GeneratorsConfig::action_current_gen_changed);
    layout_edit->addWidget(combo_edit, 1);

    button_edit = new QPushButton{ "Edit", this };
    connect(button_edit, &QPushButton::released, this, &GeneratorsConfig::action_edit);
    layout_edit->addWidget(button_edit);

    button_delete = new QPushButton{ "Delete", this };
    connect(button_delete, &QPushButton::released, this, &GeneratorsConfig::action_delete);
    layout_edit->addWidget(button_delete);

    // THIRD ROW
    layout_simulate = new QHBoxLayout{};
    layout_main->addLayout(layout_simulate);

    label_time = new QLabel{ this };
    update_sim_time();
    layout_simulate->addWidget(label_time);

    spinbox_time = new QSpinBox{ this };
    spinbox_time->setRange(1, std::numeric_limits<int>::max());
    layout_simulate->addWidget(spinbox_time);

    button_simulate = new QPushButton{ "Simulate", this };
    connect(button_simulate, &QPushButton::released, this, &GeneratorsConfig::action_simulate);
    layout_simulate->addWidget(button_simulate);

    // LAST ROW
    button_clear = new QPushButton{ "Clear", this };
    connect(button_clear, &QPushButton::released, this, &GeneratorsConfig::action_clear);
    layout_main->addWidget(button_clear);

    // OTHERS
    popup_error = new QErrorMessage{ this };
    popup_error->setWindowModality(Qt::WindowModality::WindowModal);

    set_initialized(false);
}

void GeneratorsConfig::populate_generators()
{
    if (generator) {
        combo_add->clear();
        for (const auto &[name, _] : gen_options)
            combo_add->addItem(name);
    } else {
        combo_add->clear();
        combo_add->addItem("Base");
    }
}

void GeneratorsConfig::set_initialized(bool initialized)
{
    combo_edit->setEnabled(initialized);
    button_edit->setEnabled(initialized);
    button_delete->setEnabled(initialized);
    button_simulate->setEnabled(initialized);
    populate_generators();
}

void GeneratorsConfig::update_sim_time()
{
    auto label = QString::fromStdString(std::format("Simulation time: {}", simulation_time));
    label_time->setText(label);
}

void GeneratorsConfig::action_add()
{
    try {
        std::string name;
        QString gen_name;
        if (!generator) {
            name = edit_base();
            gen_name = "Base";
        } else {
            const auto idx = combo_add->currentIndex();
            const auto [gn, func] = gen_options[idx];
            name = func(nullptr);
            gen_name = gn;
        }
        if (name.length()) {
            combo_edit->addItem(QString::fromStdString(name), gen_name);
            combo_edit->setCurrentIndex(combo_edit->count() - 1);
        }
    } catch (const std::exception &e) {
        popup_error->showMessage(e.what());
    }
}

void GeneratorsConfig::action_edit()
{
    try {
        const auto idx = combo_edit->currentIndex();
        const auto type = combo_edit->currentData().toString();
        const auto count = combo_edit->count();
        const auto n_back = count - idx - 1;
        auto gen = get_nth_generator(n_back);
        std::string name;
        if (type == "Base") {
            name = edit_base();
        } else {
            for (const auto &[t, f] : gen_options) {
                if (t == type) {
                    name = f(gen);
                    break;
                }
            }
        }
        if (name.length())
            combo_edit->setItemText(idx, QString::fromStdString(name));
    } catch (const std::exception &e) {
        popup_error->showMessage(e.what());
    }
}

void GeneratorsConfig::action_delete()
{
    const auto idx = combo_edit->currentIndex();
    const auto count = combo_edit->count();
    // This should not be possible
    if (idx < 1)
        return;

    if (idx + 1 == count) {
        auto current_gen_raw = dynamic_cast<GeneratorDecor *>(generator.get());
        Q_ASSERT(current_gen_raw != nullptr);
        std::swap(generator, current_gen_raw->m_base);
        // `generator` now contains the previous pointer, but the (now old) generator holds
        // a pointer to itself, which must be released to free the memory
        current_gen_raw->m_base.reset(nullptr);
    } else {
        auto gen_next = dynamic_cast<GeneratorDecor *>(get_nth_generator(count - idx - 2));
        Q_ASSERT(gen_next != nullptr);
        auto gen_delete = dynamic_cast<GeneratorDecor *>(gen_next->m_base.get());
        Q_ASSERT(gen_delete != nullptr);
        std::swap(gen_delete->m_base, gen_next->m_base);
        // gen_delete now has ownership of itself, make sure to release the pointer
        gen_delete->m_base.reset(nullptr);
    }
    combo_edit->removeItem(idx);
    combo_edit->setCurrentIndex(idx - 1);
}

void GeneratorsConfig::action_simulate()
{
    const auto duration = spinbox_time->value();
    const auto end = simulation_time + duration;
    std::vector<double> outputs;
    outputs.reserve(duration);
    for (; simulation_time < end; ++simulation_time)
        outputs.push_back(generator->symuluj(simulation_time));

    update_sim_time();
    emit simulated(outputs);
}

void GeneratorsConfig::action_clear()
{
    combo_edit->clear();
    generator.reset(nullptr);
    simulation_time = 0;
    set_initialized(false);
    update_sim_time();
    emit cleared();
}

void GeneratorsConfig::action_current_gen_changed()
{
    button_delete->setEnabled(combo_edit->currentIndex() > 0);
}

GeneratorsConfig::GeneratorsConfig(QWidget *parent, Qt::WindowFlags f)
    : QWidget{ parent, f }
{
    prepare_layout();
}

void GeneratorsConfig::reset_sim()
{
    simulation_time = 0;
    update_sim_time();
}
