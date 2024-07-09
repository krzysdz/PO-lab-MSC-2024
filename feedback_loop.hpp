#pragma once
#include "ModelARX.h"
#include "RegulatorPID.h"
#include <optional>

double feedback_step(RegulatorPID &regulator, ModelARX &model, const double u,
                     const std::optional<double> &reset = std::nullopt);

#ifdef LAB_TESTS
class FeedbackTests {
    static ModelARX get_model();
    static void run_sim(RegulatorPID &pid);
    static void simulate_p_1();
    static void simulate_p_2();
    static void simulate_pi_1();
    static void simulate_pi_2();

public:
    static void run_tests();
};
#endif
