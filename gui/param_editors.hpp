#pragma once
#include "../ModelARX.h"
#include "../ObiektStatyczny.hpp"
#include "../PętlaUAR.hpp"
#include "../RegulatorPID.h"
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

template <typename E, typename T>
concept ParamEditor
    = std::derived_from<E, QWidget> && std::same_as<typename E::ObjectT, T> && requires(T &t, E e) {
          { e.update_from(std::as_const(t)) } -> std::same_as<void>;
          { std::as_const(e).make_obj() } -> std::same_as<std::unique_ptr<T>>;
          { std::as_const(e).update_obj(t) } -> std::same_as<void>;
      };

class PIDParams : public QWidget {
    Q_OBJECT

private:
    QFormLayout *layout_params;
    QDoubleSpinBox *input_pid_k;
    QDoubleSpinBox *input_pid_ti;
    QDoubleSpinBox *input_pid_td;

public:
    using ObjectT = RegulatorPID;

    explicit PIDParams(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    PIDParams(const RegulatorPID &pid, QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());

    void update_from(const RegulatorPID &pid);
    std::unique_ptr<RegulatorPID> make_obj() const;
    void update_obj(RegulatorPID &pid) const;
};
static_assert(ParamEditor<PIDParams, RegulatorPID>);

class ObiektStatycznyParams : public QWidget {
    Q_OBJECT

private:
    using point = ObiektStatyczny::point;

    QFormLayout *layout_params;
    QDoubleSpinBox *input_static_x1;
    QDoubleSpinBox *input_static_y1;
    QDoubleSpinBox *input_static_x2;
    QDoubleSpinBox *input_static_y2;

    std::pair<point, point> get_points() const;

public:
    using ObjectT = ObiektStatyczny;

    explicit ObiektStatycznyParams(QWidget *parent = nullptr,
                                   Qt::WindowFlags f = Qt::WindowFlags());
    ObiektStatycznyParams(const ObiektStatyczny &obj, QWidget *parent = nullptr,
                          Qt::WindowFlags f = Qt::WindowFlags());

    void update_from(const ObiektStatyczny &obj);
    std::unique_ptr<ObiektStatyczny> make_obj() const;
    void update_obj(ObiektStatyczny &obj) const;
};
static_assert(ParamEditor<ObiektStatycznyParams, ObiektStatyczny>);

class ARXParams : public QWidget {
    Q_OBJECT

private:
    QFormLayout *layout_params;
    QSpinBox *input_arx_delay;
    QDoubleSpinBox *input_arx_stddev;
    QLineEdit *input_arx_coeff_a;
    QLineEdit *input_arx_coeff_b;

    static std::vector<double> parse_coefficients(const QString &coeff_text);
    static QString coeff_string(const std::vector<double> &coeff);

public:
    using ObjectT = ModelARX;

    explicit ARXParams(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ARXParams(const ModelARX &arx, QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());

    void update_from(const ModelARX &arx);
    std::unique_ptr<ModelARX> make_obj() const;
    void update_obj(ModelARX &arx) const;
};
static_assert(ParamEditor<ARXParams, ModelARX>);

class UARParams : public QWidget {
    Q_OBJECT

private:
    QFormLayout *layout_params;
    QCheckBox *input_uar_closed;
    QDoubleSpinBox *input_uar_init_val;

public:
    using ObjectT = PętlaUAR;

    explicit UARParams(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    UARParams(const PętlaUAR &uar, QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());

    void update_from(const PętlaUAR &uar);
    std::unique_ptr<PętlaUAR> make_obj() const;
    void update_obj(PętlaUAR &uar) const;
};
static_assert(ParamEditor<UARParams, PętlaUAR>);

template <typename E>
    requires ParamEditor<E, typename E::ObjectT>
class EditorDialog : public QDialog {
private:
    E *editor_widget;
    QDialogButtonBox *button_box;

public:
    explicit EditorDialog(QWidget *parent = nullptr)
        : QDialog{ parent }
    {
        editor_widget = new E{ this };
        button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *main_layout = new QVBoxLayout;
        main_layout->addWidget(editor_widget);
        main_layout->addWidget(button_box);
        setLayout(main_layout);
    }
    std::unique_ptr<typename E::ObjectT> make_obj() const { return editor_widget->make_obj(); }
    static std::unique_ptr<typename E::ObjectT> get_component(QWidget *parent, const QString &title)
    {
        EditorDialog<E> dialog{ parent };
        dialog.setWindowTitle(title);
        const int ret = dialog.exec();
        // Release the parent, so it won't try to free the stack by destroying dialog later
        dialog.setParent(nullptr);
        return ret ? dialog.make_obj() : nullptr;
    }
};
