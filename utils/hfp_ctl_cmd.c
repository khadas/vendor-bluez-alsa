// Copyright (C) 2023 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/un.h>
#include "hfp_ctl.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

static const struct option long_options[] = {
    {"answer_call", no_argument, NULL, 'a'},
    {"reject_call", no_argument, NULL, 'r'},
    {"vol_spk_up", no_argument, NULL, 'u'},
    {"vol_spk_down", no_argument, NULL, 'd'},
    {"vol_mic_up", no_argument, NULL, 'm'},
    {"vol_mic_down", no_argument, NULL, 'n'},
	{"get_state", no_argument, NULL, 'g'},
    {NULL, 0, NULL, 0}
};

static const char *cmd_useage = {
	"Usage:\n"
	"\t [--answer_call]: answer call \n"
	"\t [--reject_call]: reject call \n"
	"\t [--vol_spk_up]: adjust speaker vol+ \n"
	"\t [--vol_spk_down]: adjust speaker vol- \n"
	"\t [--vol_mic_up]: adjust mic vol+ \n"
	"\t [--vol_mic_down]: adjust mic vol- \n"
	"\t [--get_state]: get the hfp state \n"
};

int main(int argc, char **argv)
{
	struct msgstru msgs;
	int ret;
	int msqid;
	int retry = 10;
	bool need_feedback = false;

	if (argc != 2 || !argv[1]) {
		printf("%s", cmd_useage);
		return -1;
	}

	while ((ret = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch (ret) {
        case 'a':
            strcpy(msgs.msgtext, "answer_call");
            break;
        case 'r':
			strcpy(msgs.msgtext, "reject_call");
            break;
        case 'u':
			strcpy(msgs.msgtext, "vol_spk_up");
            break;
        case 'd':
			strcpy(msgs.msgtext, "vol_spk_down");
            break;
        case 'm':
			strcpy(msgs.msgtext, "vol_mic_up");
            break;
        case 'n':
			strcpy(msgs.msgtext, "vol_mic_down");
            break;
		case 'g':
			strcpy(msgs.msgtext, "get_state");
			need_feedback = true;
            break;
        case '?':
            printf("%s", cmd_useage);
            exit(EXIT_FAILURE);
        }
	}

	msqid = msgget(MSGKEY, IPC_EXCL);
	if (msqid < 0)
	{
		printf("%s", "The message queue does not exist\n");
		return -1;
	}

	msgs.msgtype=MSGTYPE_TO_HOST;
	ret = msgsnd(msqid, &msgs, sizeof(msgs.msgtext), IPC_NOWAIT);
	if (ret < 0)
	{
		printf("msgsnd() write msg failed,errno=%d[%s]\n", errno, strerror(errno));
		return -1;
	}

	if (need_feedback) {
		while (retry > 0) {
			ret = msgrcv(msqid, &msgs, sizeof(msgs.msgtext), MSGTYPE_FROM_HOST, IPC_NOWAIT);
			if (ret > 0) {
				if (strncmp(msgs.msgtext, "hfp_state=", strlen("hfp_state=")) == 0) {
					printf("%s", msgs.msgtext + strlen("hfp_state="));
					break;
				}
			}
			usleep(10000);
			retry--;
		}
		if (retry == 0)
			printf("%s", "Message receive timeout\n");
	}
	return 0;
}
