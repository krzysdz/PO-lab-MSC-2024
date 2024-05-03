#pragma once
#include <optional>
#include "../lab1/ModelARX.h"
#include "RegulatorPID.h"

double feedback_step(RegulatorPID &regulator, ModelARX &model, const double u,
                     const std::optional<double> &reset = std::nullopt);