#pragma once
#include "../lab2/define_fixes.hpp"
#include <vector>

class ObiektSISO {
public:
    virtual void reset() { }
    virtual double symuluj(double u) = 0;
    virtual ~ObiektSISO() = default;

    friend bool operator==(const ObiektSISO &, const ObiektSISO &) = default;
    friend bool operator!=(const ObiektSISO &, const ObiektSISO &) = default;
};

#ifndef NO_LAB_TESTS
class TESTY {
public:
    static void raportBleduSekwencji(std::vector<double> &spodz, std::vector<double> &fakt);
    static bool porownanieSekwencji(std::vector<double> &spodz, std::vector<double> &fakt);
};
#endif
