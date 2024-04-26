#include "../lab1/ObiektSISO.h"
#include "../lab1/util.hpp"

class RegulatorPID : public ObiektSISO
{
private:
	double m_k;
	double m_ti;
	double m_td;
	double m_integral{};
	double m_prev_e{};

	constexpr void check_constraints() const;

public:
	constexpr RegulatorPID(double k, double ti = 0.0, double td = 0.0) : m_k{k}, m_ti{ti}, m_td{td} {
		check_constraints();
	}
	constexpr void set_k(double k) { throw_bad_neg(k); m_k = k; }
	constexpr void set_ti(double ti) { throw_bad_neg(ti); m_ti = ti; }
	constexpr void set_td(double td) { throw_bad_neg(td); m_td = td; }
	constexpr double symuluj(double u) override;
};

