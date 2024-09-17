/// @file param_editors.hpp
/// @brief Custom editor widgets, dialog and related utilities.

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

/// @brief Concept describing parameter editor widgets.
/// @tparam E type of the editor widget
/// @tparam T type whose parameters can be edited using this widget
template <typename E, typename T>
concept ParamEditor
    = std::derived_from<E, QWidget> && std::same_as<typename E::ObjectT, T> && requires(T &t, E e) {
          { e.update_from(std::as_const(t)) } -> std::same_as<void>;
          { std::as_const(e).make_obj() } -> std::same_as<std::unique_ptr<T>>;
          { std::as_const(e).update_obj(t) } -> std::same_as<void>;
      };

/// A #ParamEditor widget for #RegulatorPID.
class PIDParams : public QWidget {
    Q_OBJECT

private:
    /// Main widget layout used for inputs.
    QFormLayout *layout_params;
    /// Spinbox with the @link RegulatorPID::m_k k@endlink value.
    QDoubleSpinBox *input_pid_k;
    /// Spinbox with the @link RegulatorPID::m_ti ti@endlink value.
    QDoubleSpinBox *input_pid_ti;
    /// Spinbox with the @link RegulatorPID::m_td td@endlink value.
    QDoubleSpinBox *input_pid_td;

public:
    /// Type of object which can be edited.
    using ObjectT = RegulatorPID;

    /// @brief Construct an editor widget with default input values.
    /// @see QWidget::QWidget()
    explicit PIDParams(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    /// @brief Construct an editor widget with values from `pid`.
    /// @param pid object with initial input values
    /// @see QWidget::QWidget()
    PIDParams(const RegulatorPID &pid, QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());

    /// @brief Update inputs to match parameters of `pid`.
    /// @param pid object with initial input values
    void update_from(const RegulatorPID &pid);
    /// @brief Create a new object with parameters from current inputs.
    /// @return A unique pointer to a new #RegulatorPID.
    std::unique_ptr<RegulatorPID> make_obj() const;
    /// @brief Update `pid` with current input values.
    /// @param pid an object to update
    void update_obj(RegulatorPID &pid) const;
};
static_assert(ParamEditor<PIDParams, RegulatorPID>);

/// A #ParamEditor widget for #ObiektStatyczny.
class ObiektStatycznyParams : public QWidget {
    Q_OBJECT

private:
    /// <x, y> pair describing a 2D point
    using point = ObiektStatyczny::point;

    /// Main widget layout used for inputs.
    QFormLayout *layout_params;
    /// Spinbox with the x parameter of 1st point value.
    QDoubleSpinBox *input_static_x1;
    /// Spinbox with the y parameter of 1st point value.
    QDoubleSpinBox *input_static_y1;
    /// Spinbox with the x parameter of 2nd point value.
    QDoubleSpinBox *input_static_x2;
    /// Spinbox with the y parameter of 2nd point value.
    QDoubleSpinBox *input_static_y2;

    /// @brief Construct a pair of points based on current input values.
    /// @return <p1, p2> pair of <x, y> points
    std::pair<point, point> get_points() const;

public:
    /// Type of object which can be edited.
    using ObjectT = ObiektStatyczny;

    /// @brief Construct an editor widget with default input values.
    /// @see QWidget::QWidget()
    explicit ObiektStatycznyParams(QWidget *parent = nullptr,
                                   Qt::WindowFlags f = Qt::WindowFlags());
    /// @brief Construct an editor widget with values from `obj`.
    /// @param obj object with initial input values
    /// @see QWidget::QWidget()
    ObiektStatycznyParams(const ObiektStatyczny &obj, QWidget *parent = nullptr,
                          Qt::WindowFlags f = Qt::WindowFlags());

    /// @brief Update inputs to match parameters of `obj`.
    /// @param obj object with initial input values
    void update_from(const ObiektStatyczny &obj);
    /// @brief Create a new object with parameters from current inputs.
    /// @return A unique pointer to a new #ObiektStatyczny.
    std::unique_ptr<ObiektStatyczny> make_obj() const;
    /// @brief Update `obj` with current input values.
    /// @param obj an object to update
    void update_obj(ObiektStatyczny &obj) const;
};
static_assert(ParamEditor<ObiektStatycznyParams, ObiektStatyczny>);

/// A #ParamEditor widget for #ModelARX.
class ARXParams : public QWidget {
    Q_OBJECT

private:
    /// Main widget layout used for inputs.
    QFormLayout *layout_params;
    /// Spinbox with the @link ModelARX::m_transport_delay transport delay@endlink value.
    QSpinBox *input_arx_delay;
    /// Spinbox with the standard deviation value.
    QDoubleSpinBox *input_arx_stddev;
    /// LineEdit with the @link ModelARX::m_coeff_a polynomial A coefficients@endlink
    QLineEdit *input_arx_coeff_a;
    /// LineEdit with the @link ModelARX::m_coeff_b polynomial B coefficients@endlink
    QLineEdit *input_arx_coeff_b;

    /// @brief Parse a comma-delimited string of decimal numbers
    /// @param coeff_text string with comma-separated decimal numbers
    /// @return A vector of parsed coefficients.
    static std::vector<double> parse_coefficients(const QString &coeff_text);
    /// @brief Format a vector of doubles into human-readable, comma-delimited string.
    /// @param coeff vector of coefficients to represent as a string
    /// @return A human-readable string that can be parsed by #parse_coefficients().
    static QString coeff_string(const std::vector<double> &coeff);

public:
    /// Type of object which can be edited.
    using ObjectT = ModelARX;

    /// @brief Construct an editor widget with default input values.
    /// @see QWidget::QWidget()
    explicit ARXParams(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    /// @brief Construct an editor widget with values from `arx`.
    /// @param arx object with initial input values
    /// @see QWidget::QWidget()
    ARXParams(const ModelARX &arx, QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());

    /// @brief Update inputs to match parameters of `arx`.
    /// @param arx object with initial input values
    void update_from(const ModelARX &arx);
    /// @brief Create a new object with parameters from current inputs.
    /// @return A unique pointer to a new #ModelARX.
    std::unique_ptr<ModelARX> make_obj() const;
    /// @brief Update `arx` with current input values.
    /// @param arx an object to update
    void update_obj(ModelARX &arx) const;
};
static_assert(ParamEditor<ARXParams, ModelARX>);

/// A #ParamEditor widget for #PętlaUAR.
class UARParams : public QWidget {
    Q_OBJECT

private:
    /// Main widget layout used for inputs.
    QFormLayout *layout_params;
    /// Checkbox representing whether the loop is @link PętlaUAR::m_closed closed@endlink.
    QCheckBox *input_uar_closed;
    /// Spinbox with the @link PętlaUAR::m_prev_result previus result/initial@endlink value.
    QDoubleSpinBox *input_uar_init_val;

public:
    /// Type of object which can be edited.
    using ObjectT = PętlaUAR;

    /// @brief Construct an editor widget with default input values.
    /// @see QWidget::QWidget()
    explicit UARParams(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    /// @brief Construct an editor widget with values from `uar`.
    /// @param uar object with initial input values
    /// @see QWidget::QWidget()
    UARParams(const PętlaUAR &uar, QWidget *parent = nullptr,
              Qt::WindowFlags f = Qt::WindowFlags());

    /// @brief Update inputs to match parameters of `uar`.
    /// @param uar object with initial input values
    void update_from(const PętlaUAR &uar);
    /// @brief Create a new object with parameters from current inputs.
    /// @return A unique pointer to a new #PętlaUAR.
    std::unique_ptr<PętlaUAR> make_obj() const;
    /// @brief Update `uar` with current input values.
    /// @param uar an object to update
    void update_obj(PętlaUAR &uar) const;
};
static_assert(ParamEditor<UARParams, PętlaUAR>);

/// @brief A dialog presenting a #ParamEditor.
/// @tparam E type of #ParamEditor widget
template <typename E>
    requires ParamEditor<E, typename E::ObjectT>
class EditorDialog : public QDialog {
private:
    /// The editor widget presented in dialog.
    E *editor_widget;
    /// Basic button box (Ok, Cancel) with the dialog actions.
    QDialogButtonBox *button_box;

public:
    /// @brief Construct a dialog with parent `parent`.
    /// @see QDialog::QDialog()
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
    /// @brief Create a new object with parameters from current inputs.
    /// @return A unique pointer to a new object (type depends on the template parameter).
    std::unique_ptr<typename E::ObjectT> make_obj() const { return editor_widget->make_obj(); }
    /// @brief Static convenience function to get a complete object from the user.
    /// @param parent pointer to parent widget
    /// @param title text displayed in titlebar of the dailog
    /// @return A unique pointer to the constructed object. Pointer will be `nullptr` if user
    /// cancels the dialog.
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
