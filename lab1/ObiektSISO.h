#pragma once

class ObiektSISO {
public:
    virtual double symuluj(double u) = 0;
    virtual ~ObiektSISO() = default;

    friend bool operator==(const ObiektSISO &, const ObiektSISO &) = default;
    friend bool operator!=(const ObiektSISO &, const ObiektSISO &) = default;
};
