#include "CInterpreter.h"

// FIXME: Architectural hack. This is defined in icl_parser.y++.
// Perhaps this should be CConfig's method ?
extern void PrintConfig(FILE *fd, pCConfig&  pConfig);

void CInterpreter::Interpret()
{
	YYmain(m_pConfig, m_pControl, m_pFD_in, m_pFD_out, m_nMode);
}

int SaveConfig(pCConfig& pConfig, const char *filename)
{
	FILE *f_out = fopen(filename, "w");
	if (f_out == NULL) {
		TRACE("Cannot open %s!", filename);
		return -1;
	}

//	PrintConfig(f_out, pConfig);
	fclose(f_out);

	return 0;
}
