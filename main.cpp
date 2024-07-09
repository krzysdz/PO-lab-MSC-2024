#ifndef LAB_TESTS
#include "gui/MainWindow.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app{ argc, argv };
    MainWindow main_window{};
    main_window.start();
    return app.exec();
}
#else
#include "ModelARX.h"
#include "RegulatorPID.h"
#include "feedback_loop.hpp"
#include "generators.hpp"

int main()
{
    Testy_ModelARX::run_tests();
    Testy_RegulatorPID::run_tests();
    FeedbackTests::run_tests();
    GeneratorTests::run_tests();
    return 0;
}
#endif
