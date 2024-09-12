#include "param_editors.hpp"
#include <QMessageBox>

PIDParams::PIDParams(QWidget *parent, Qt::WindowFlags f)
    : QWidget{ parent, f }
{
    layout_params = new QFormLayout{ this };

    input_pid_k = new QDoubleSpinBox{ this };
    input_pid_k->setMinimum(0.0);
    input_pid_k->setSingleStep(0.1);
    layout_params->addRow("k", input_pid_k);

    input_pid_ti = new QDoubleSpinBox{ this };
    input_pid_ti->setMinimum(0.0);
    layout_params->addRow("Ti", input_pid_ti);

    input_pid_td = new QDoubleSpinBox{ this };
    input_pid_td->setMinimum(0.0);
    layout_params->addRow("Td", input_pid_td);
}

PIDParams::PIDParams(const RegulatorPID &pid, QWidget *parent, Qt::WindowFlags f)
    : PIDParams{ parent, f }
{
    update_from(pid);
}

void PIDParams::update_from(const RegulatorPID &pid)
{
    input_pid_k->setValue(pid.get_k());
    input_pid_ti->setValue(pid.get_ti());
    input_pid_td->setValue(pid.get_td());
}

std::unique_ptr<RegulatorPID> PIDParams::make_obj() const
{
    return std::make_unique<RegulatorPID>(input_pid_k->value(), input_pid_ti->value(),
                                          input_pid_td->value());
}

void PIDParams::update_obj(RegulatorPID &pid) const
{
    pid.set_k(input_pid_k->value());
    pid.set_ti(input_pid_ti->value());
    pid.set_td(input_pid_td->value());
}

std::pair<ObiektStatyczny::point, ObiektStatyczny::point> ObiektStatycznyParams::get_points() const
{
    using point = ObiektStatyczny::point;
    double x1 = input_static_x1->value();
    double y1 = input_static_y1->value();
    double x2 = input_static_x2->value();
    double y2 = input_static_y2->value();

    return { point{ x1, y1 }, point{ x2, y2 } };
}

ObiektStatycznyParams::ObiektStatycznyParams(QWidget *parent, Qt::WindowFlags f)
    : QWidget{ parent, f }
{
    layout_params = new QFormLayout{ this };

    input_static_x1 = new QDoubleSpinBox{ this };
    input_static_x1->setSingleStep(0.1);
    input_static_x1->setRange(std::numeric_limits<double>::lowest(),
                              std::numeric_limits<double>::max());
    input_static_x1->setValue(-1.0);
    layout_params->addRow("x1", input_static_x1);
    input_static_y1 = new QDoubleSpinBox{ this };
    input_static_y1->setSingleStep(0.1);
    input_static_y1->setRange(std::numeric_limits<double>::lowest(),
                              std::numeric_limits<double>::max());
    input_static_y1->setValue(-1.0);
    layout_params->addRow("y1", input_static_y1);
    input_static_x2 = new QDoubleSpinBox{ this };
    input_static_x2->setSingleStep(0.1);
    input_static_x2->setRange(std::numeric_limits<double>::lowest(),
                              std::numeric_limits<double>::max());
    input_static_x2->setValue(1.0);
    layout_params->addRow("x2", input_static_x2);
    input_static_y2 = new QDoubleSpinBox{ this };
    input_static_y2->setSingleStep(0.1);
    input_static_y2->setRange(std::numeric_limits<double>::lowest(),
                              std::numeric_limits<double>::max());
    input_static_y2->setValue(1.0);
    layout_params->addRow("y2", input_static_y2);
}

ObiektStatycznyParams::ObiektStatycznyParams(const ObiektStatyczny &obj, QWidget *parent,
                                             Qt::WindowFlags f)
    : ObiektStatycznyParams{ parent, f }
{
    update_from(obj);
}

void ObiektStatycznyParams::update_from(const ObiektStatyczny &obj)
{
    const auto [p1, p2] = obj.get_points();
    const auto [x1, y1] = p1;
    const auto [x2, y2] = p2;

    input_static_x1->setValue(x1);
    input_static_y1->setValue(y1);
    input_static_x2->setValue(x2);
    input_static_y2->setValue(y2);
}

std::unique_ptr<ObiektStatyczny> ObiektStatycznyParams::make_obj() const
{
    const auto [p1, p2] = get_points();
    return std::make_unique<ObiektStatyczny>(p1, p2);
}

void ObiektStatycznyParams::update_obj(ObiektStatyczny &obj) const
{
    const auto [p1, p2] = get_points();
    obj.set_points(p1, p2);
    // Thanks to the wonders of C++ the above 2 lines can be expressed also using this "one-liner"
    // std::apply(&ObiektStatyczny::set_points,
    //            std::tuple_cat(std::forward_as_tuple(obj), get_points()));
}

std::vector<double> ARXParams::parse_coefficients(const QString &coeff_text)
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

QString ARXParams::coeff_string(const std::vector<double> &coeff)
{
    QStringList list;
    for (const auto param : coeff) {
        list.push_back(QString::number(
            param, 'g', QLocale::FloatingPointPrecisionOption::FloatingPointShortest));
    }
    return list.join(',');
}

ARXParams::ARXParams(QWidget *parent, Qt::WindowFlags f)
    : QWidget{ parent, f }
{
    layout_params = new QFormLayout{ this };

    input_arx_delay = new QSpinBox{ this };
    input_arx_delay->setMinimum(1);
    layout_params->addRow("Delay", input_arx_delay);

    input_arx_stddev = new QDoubleSpinBox{ this };
    input_arx_stddev->setMinimum(0.0);
    input_arx_stddev->setSingleStep(0.1);
    layout_params->addRow("stddev", input_arx_stddev);

    QRegularExpression coeff_pattern{ R"(^\s*(?:-?\d+(?:\.\d+)?(?:\s*?,\s*?|\s*?$))*$)" };
    const auto coeff_validator = new QRegularExpressionValidator{ coeff_pattern, this };

    input_arx_coeff_a = new QLineEdit{ this };
    input_arx_coeff_a->setToolTip("Comma separated list of coefficients");
    input_arx_coeff_a->setValidator(coeff_validator);
    layout_params->addRow("<b>A</b> coefficients", input_arx_coeff_a);

    input_arx_coeff_b = new QLineEdit{ this };
    input_arx_coeff_b->setToolTip("Comma separated list of coefficients");
    input_arx_coeff_b->setValidator(coeff_validator);
    layout_params->addRow("<b>B</b> coefficients", input_arx_coeff_b);
}

ARXParams::ARXParams(const ModelARX &arx, QWidget *parent, Qt::WindowFlags f)
    : ARXParams{ parent, f }
{
    update_from(arx);
}

void ARXParams::update_from(const ModelARX &arx)
{
    input_arx_delay->setValue(static_cast<int>(arx.get_transport_delay()));
    input_arx_stddev->setValue(arx.get_stddev());
    input_arx_coeff_a->insert(coeff_string(arx.get_coeff_a()));
    input_arx_coeff_b->insert(coeff_string(arx.get_coeff_b()));
}

std::unique_ptr<ModelARX> ARXParams::make_obj() const
{
    auto coeff_a = parse_coefficients(input_arx_coeff_a->text());
    auto coeff_b = parse_coefficients(input_arx_coeff_b->text());
    const auto delay = input_arx_delay->value();
    const auto stddev = input_arx_stddev->value();
    return std::make_unique<ModelARX>(std::move(coeff_a), std::move(coeff_b), delay, stddev);
}

void ARXParams::update_obj(ModelARX &arx) const
{
    arx.set_coeff_a(parse_coefficients(input_arx_coeff_a->text()));
    arx.set_coeff_b(parse_coefficients(input_arx_coeff_b->text()));
    arx.set_transport_delay(input_arx_delay->value());
    arx.set_stddev(input_arx_stddev->value());
}

UARParams::UARParams(QWidget *parent, Qt::WindowFlags f)
    : QWidget{ parent, f }
{
    layout_params = new QFormLayout{ this };

    input_uar_closed = new QCheckBox{ this };
    input_uar_closed->setChecked(true);
    layout_params->addRow("Closed", input_uar_closed);

    input_uar_init_val = new QDoubleSpinBox{ this };
    input_uar_init_val->setMinimum(0.0);
    input_uar_init_val->setSingleStep(0.1);
    input_uar_init_val->setRange(std::numeric_limits<double>::lowest(),
                                 std::numeric_limits<double>::max());
    layout_params->addRow("Initial value", input_uar_init_val);
}

UARParams::UARParams(const PętlaUAR &uar, QWidget *parent, Qt::WindowFlags f)
    : UARParams{ parent, f }
{
    update_from(uar);
}

void UARParams::update_from(const PętlaUAR &uar)
{
    input_uar_closed->setChecked(uar.get_closed());
    input_uar_init_val->setValue(uar.get_last_result());
}

std::unique_ptr<PętlaUAR> UARParams::make_obj() const
{
    return std::make_unique<PętlaUAR>(input_uar_closed->isChecked(), input_uar_init_val->value());
}

void UARParams::update_obj(PętlaUAR &uar) const
{
    uar.set_closed(input_uar_closed->isChecked());
    uar.set_init(input_uar_init_val->value());
}
