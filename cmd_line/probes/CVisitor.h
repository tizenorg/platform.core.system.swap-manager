#ifndef __CVISITOR_H__
#define __CVISITOR_H__

#include "debug.h"

class CConfig;
class CFeatures;

class CAppListContainer;
class CLibListContainer;
class CProbeListContainer;

class CAppListElm;
class CProbeListElm;
class CProbeData;
class CProbeDataFBIVarList;
class CProbeDataFBIVar;

class CProbeDataFBIStepList;
class CProbeDataFBIStep;
class CLibListElm;

enum output_mode_t {
	OM_SIMPLE = 0,
	OM_DEBUG = 1
};

/* TODO refactor this defines or remove it */

#define VISITOR_INTERFACES(TYPE, VAR_NAME, func1, func2) \
		virtual CVisitor::code_t access(TYPE VAR_NAME) func1;\
		virtual void entry(TYPE VAR_NAME) func2;\
		virtual void exit(TYPE VAR_NAME) func2;

#define VISITOR_INTERFACE(CLASS, VAR_NAME) \
		VISITOR_INTERFACES(CLASS *const, VAR_NAME, {LOGI_VIS(#CLASS); return ACCESS_GRANTED;} , {LOGI_VIS(#CLASS);});
#define LOGI_VIS LOGI
//#define LOGI_VIS(...)

class CVisitor
{
	public:
		enum code_t {
			ACCESS_GRANTED,
			ACCESS_DENIED
		};

/* CConfig */
		VISITOR_INTERFACE(CConfig, conf);

/* CFeatures */
		VISITOR_INTERFACE(CFeatures, features);

/* CAppListContainer */
		VISITOR_INTERFACE(CAppListContainer, app_list);
/* CLibListContainer */
		VISITOR_INTERFACE(CLibListContainer, lib_list);
/* CProbeListContainer */
		VISITOR_INTERFACE(CProbeListContainer, probe_list);

/* CAppListElm */
		VISITOR_INTERFACE(CAppListElm, app_list_elm);
/* CLibListElm */
		VISITOR_INTERFACE(CLibListElm, var_list_elm);
/* CProbeListElm */
		VISITOR_INTERFACE(CProbeListElm, probe_list_elm);

/* CProbeData */
		VISITOR_INTERFACE(CProbeData, probe_data);
/* CProbeDataFBIVarList */
		VISITOR_INTERFACE(CProbeDataFBIVarList, var_list);
/* CProbeDataFBIVar */
		VISITOR_INTERFACE(CProbeDataFBIVar, data_var);
/* CProbeDataFBIStepList */
		VISITOR_INTERFACE(CProbeDataFBIStepList, step_list);
/* CProbeDataFBIStep */
		VISITOR_INTERFACE(CProbeDataFBIStep, step);

		CVisitor() {};
		virtual ~CVisitor() {};
};

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

class CSerialise: public CVisitor
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

		CSerialise()
		{
		}

		virtual ~CSerialise()
		{
		}
};

class CNode
{
	public:
		virtual void accept(CVisitor& v) = 0;

		CNode(){};
		virtual ~CNode(){};
};
#endif /* __CVISITOR_H__ */
