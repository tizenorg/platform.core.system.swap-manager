#ifndef __CSAVEPF_H__
#define __CSAVEPF_H__

#include "CVisitor.h"
#include "debug.h"

#define SERIALISER_INTERFACES(CLASS, VAR_NAME) \
		virtual void entry(CLASS *const VAR_NAME);\
		virtual void exit(CLASS *const VAR_NAME)

class OutPut
{
	public:
		friend OutPut &operator<<(OutPut &out,const char *st)
		{
			printf("%s", st);
			return out;
		}

		friend OutPut &operator<<(OutPut &out,std::string st)
		{
			out << st.c_str();
			return out;
		}

		OutPut() {};
		virtual ~OutPut() {};
};

class CSavepf: public CVisitor
{
	private:
		OutPut m_Out;
	public:

/* CFeatures */
		SERIALISER_INTERFACES(CFeatures, features);
/* CProbeData */
		SERIALISER_INTERFACES(CProbeData, probe_data);
/* CProbeListElm */
		SERIALISER_INTERFACES(CProbeListElm, probe_list);

		CSavepf()
		{
		}

		virtual ~CSavepf()
		{
		}
};

#endif /* __CSAVEPF_H__ */
