#include "CVisitor.h"
#include "CSerialise.h"

#include "CProbeData.h"
#include "CProbe.h"
#include "CFeatures.h"

/* CFeatures */
void CSerialise::entry(CFeatures *const features)
{
	m_Out << "# Features list\n";

	int i;
	std::string res;

	for (i = 0; i < FEATURE_MAX_SIZE; i++) {
		if (features->test(i)) {
			/* TODO make a constant or define */
			m_Out << "setv feature enable " << feature_to_str_arr[i] << "\n";
		}
	}
};

void CSerialise::exit(CFeatures* const features)
{
	m_Out << "# CFeatures<<\n";
};

/* CProbeData */
void CSerialise::entry(CProbeData* const probe_data)
{
	m_Out << "# CProbeData>>\n";
};

void CSerialise::exit(CProbeData* const probe_data)
{
	m_Out << "# CProbeData<<\n";
};

/* CProbeListElm */
void CSerialise::entry(CProbeListElm* const probe_list)
{
	m_Out << "# CProbeListElm>>\n";
};

void CSerialise::exit(CProbeListElm* const probe_list)
{
	m_Out << "# CProbeListElm<<\n";
};


