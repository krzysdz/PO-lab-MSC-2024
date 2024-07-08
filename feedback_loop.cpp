#include "feedback_loop.hpp"

double feedback_step(RegulatorPID &regulator, ModelARX &model, const double u,
                     const std::optional<double> &reset)
{
    static double last_output = 0.0;
    if (reset.has_value())
        last_output = reset.value();
    const auto e = u - last_output;
    const auto input = regulator.symuluj(e);
    last_output = model.symuluj(input);
    return last_output;
}
