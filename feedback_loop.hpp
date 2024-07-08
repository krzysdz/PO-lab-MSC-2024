#pragma once
#include "ModelARX.h"
#include "RegulatorPID.h"
#include <optional>

double feedback_step(RegulatorPID &regulator, ModelARX &model, const double u,
                     const std::optional<double> &reset = std::nullopt);