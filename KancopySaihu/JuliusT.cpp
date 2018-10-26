#pragma execution_character_set("utf-8")

#include "JuliusT.h"
#include<julius/juliuslib.h>
#include<iostream>


QString JuliusT::mResult = "";


JuliusT::JuliusT(QObject *parent) :mJconf(0), mRecog(0)
{

}

void JuliusT::init() {
	// ログを出力しない
	jlog_set_output(NULL);
	FILE *fp; fopen_s(&fp, "log.txt", "w"); jlog_set_output(fp);

	char list[5][255] = {
		"KankopySaihu.exe",
		"-C",
		"julius/main.jconf",
		"-input",
		"mic"
	};

	char **argv = (char **)malloc(sizeof(char) * 5);
	for (int i = 0; i < 5; i++) {
		*(argv + i) = (char *)malloc(sizeof(char) * (strlen(list[i]) + 1));
		strcpy_s(argv[i], (strlen(list[i]) + 1), list[i]);
	}
	//strcpy_s(argv[0], 255, "KankopySaihu.exe");
	//strcpy_s(argv[1], 255, "-C");
	//strcpy_s(argv[2], 255, "julius/main.jconf");
	//strcpy_s(argv[3], 255, "-input");
	//strcpy_s(argv[4], 255, "mic");

	mJconf = j_config_load_args_new(5, argv);
	if (mJconf == NULL) {
		qDebug("Try '-setting' for built-in engine configuration.");
		return;
	}
	mRecog = j_create_instance_from_jconf(mJconf);
	if (mRecog == NULL) {
		qDebug("Error in startup");
		return;
	}

	callback_add(mRecog, CALLBACK_EVENT_SPEECH_READY, [](Recog *recog, void*) {
		qDebug("<<< PLEASE SPEAK! >>>");
	}, NULL);

	callback_add(mRecog, CALLBACK_EVENT_SPEECH_START, [](Recog *recog, void*) {
		qDebug("...SPEECH START...");
	}, NULL);
	int e = callback_add(mRecog, CALLBACK_RESULT, JuliusT::output_result, NULL);


	qDebug("Success Julius startup");

	// if (argv) free(argv);
}

void JuliusT::init(std::string filelist) {
	// ログを出力しない
	//jlog_set_output(NULL);

	//char **argv;
	//argv[0] = "KankopySaihu.exe";
	//argv[1] = "-C";
	//argv[2] = "julius/main.jconf";
	//argv[3] = "-input";
	//argv[4] = "rawfile";
	//argv[5] = "-filelist";
	//char file[256];
	//strcpy_s(file, 256, filelist.c_str());
	//argv[6] = file;

	//mJconf = j_config_load_args_new(7, argv);
}

JuliusT::~JuliusT()
{
	j_close_stream(mRecog);
	j_recog_free(mRecog);
	j_jconf_free(mJconf);
}

void JuliusT::output_result(struct __Recog__ *recog, void *dummy) {
	mResult = "";

	for (const RecogProcess *r = recog->process_list; r; r = r->next) {
		WORD_INFO *winfo = r->lm->winfo;
		for (int n = 0; n < r->result.sentnum; ++n) {
			Sentence *s = &(r->result.sent[n]);
			WORD_ID *seq = s->word;
			int seqnum = s->word_num;
			for (int i = 0; i < seqnum; ++i) {
				mResult += winfo->woutput[seq[i]];
			}
		}
	}

	qDebug() << mResult;
}

void JuliusT::startRecog() {
	if (j_adin_init(mRecog) == FALSE) { qDebug("adin init failed."); return; }			// マイクの準備
	j_recog_info(mRecog);

		
	// ストリームを開く
	switch (j_open_stream(mRecog, NULL)) {
	case 0:			/* succeeded */
		break;
	case -1:      		/* error */
		qDebug("error in input stream");
		return;
	case -2:			/* end of recognition process */
		qDebug("failed to begin input stream");
		return;
	}

	int ret = j_recognize_stream(mRecog);
	if (ret == -1)
		return;	/* error */
}

QString JuliusT::getResult() {
	return mResult;
}