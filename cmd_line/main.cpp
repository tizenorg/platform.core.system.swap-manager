#include <iostream>

#include "CConfig.h"
#include "CInterpreter.h"

#include "client.h"
#include "debug.h"

static CInterpreter *_inter = NULL;

int main(int argc, char *argv[])
{
	FILE *fin = NULL;
	FILE *fout = NULL;

	if (argc >= 1)
		fin = fopen(argv[1],"r");
	if (argc >= 2)
		fout = fopen(argv[2], "w");

	Client *cl = new Client();
	pCConfig config(new CConfig());

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
}
