#include <iostream>

#include "CCommonContainer.h"
#include "CConfig.h"

#include "debug.h"

static CInterpreter *_inter = NULL;

CCommonContainer *Global;

int main(int argc, char *argv[])
{
	FILE *fin = NULL;
	FILE *fout = NULL;

	if (argc >= 1)
		fin = fopen(argv[1],"r");
	if (argc >= 2)
		fout = fopen(argv[2], "w");

	Global = new CCommonContainer(fin, fout);

	Global->getInter()->Interpret();
/*
	CConfig *config = new CConfig();
	CClient *cl = new CClient();

	//config->m_bPrompt = true;
	_inter = new CInterpreter(config, ALL, fin, fout);
	_inter->Interpret();

	//cl->config->load_from_file("some_file_name");
	cl->m_Launcher->prepare();
	if (cl->connect() != 0) {
		LOGE("can not connect");
	} else {
		LOGI("connected");
	}
	delete cl;
	delete _inter;
*/
}
