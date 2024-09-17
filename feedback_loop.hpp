/// @file feedback_loop.hpp
/// @brief Simple feedback loop and related tests

#pragma once
#include "ModelARX.h"
#include "RegulatorPID.h"
#include <optional>

/// @brief Simulate a single step of a closed feedback loop with a PID regulator and ARX model
/// @param regulator a reference to PID regulator used in the loop
/// @param model a reference to ARX model used in the loop
/// @param u current input
/// @param reset optional reset value to set the saved previous result to
/// @return `model`'s output given `regulator`'s output, which was given `u - last_result`
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
