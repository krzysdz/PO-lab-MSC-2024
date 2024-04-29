#include "ObiektSISO.h"
#include <cmath>
#include <iomanip>
#include <iostream>

void TESTY::raportBleduSekwencji(std::vector<double> &spodz, std::vector<double> &fakt)
{
    constexpr size_t PREC = 3;
    std::cerr << std::fixed << std::setprecision(PREC);
    std::cerr << "  Spodziewany:\t";
    for (auto &el : spodz)
        std::cerr << el << ", ";
    std::cerr << "\n  Faktyczny:\t";
    for (auto &el : fakt)
        std::cerr << el << ", ";
    std::cerr << std::endl << std::endl;
}

bool TESTY::porownanieSekwencji(std::vector<double> &spodz, std::vector<double> &fakt)
{
    constexpr double TOL = 1e-3; // tolerancja dla porównań zmiennoprzecinkowych
    bool result = fakt.size() == spodz.size();
    for (int i = 0; result && i < fakt.size(); i++)
        result = fabs(fakt[i] - spodz[i]) < TOL;
    return result;
}
