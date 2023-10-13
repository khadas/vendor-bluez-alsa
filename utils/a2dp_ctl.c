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
#include <unistd.h>
#include <string.h>

#include <gio/gio.h>
#include <glib.h>
#include <pthread.h>
#include "a2dp_ctl.h"


void connect_call_back(gboolean connected)
{
    if (TRUE == connected) {
        printf("A2dp Connected\n");
        /*works when a2dp connected*/
    } else {
        printf("A2dp Disconnected\n");
        /*works when a2dp disconnected*/
    }
}

void play_call_back(char *status)
{
    /*Possible status: "playing", "stopped", "paused"*/
    if (strcmp("playing", status) == 0) {
        printf("Media_Player is now playing\n");
        /*works when playing*/
    } else if (strcmp("stopped", status) == 0) {
        printf("Media_Player stopped\n");
        /*works when stopped*/
    } else if (strcmp("paused", status) == 0) {
        printf("Media_Player paused\n");
        /*works when paused*/
    }
}

void volume_call_back(guint16 volume)
{
    printf("volume is %d\n", volume);
}

int main(int argc, void **argv)
{
	char device_mode[11] = "central"; //size of 11 bytes for "peripheral"
	int i, ret = 0;
	int timeout = 10;
	char *bddr = NULL;
	if (a2dp_ctl_init(connect_call_back, play_call_back, volume_call_back))
		return 1;


	while (timeout > 0) {
		if (!adapter_ready())
			break;
		sleep(1);
		timeout--;
	}

	if (timeout == 0) {
		printf("adapter not found!!, exit!!");
		return 1;
	}

	if (argc > 1) {

		if (argc > 3)
			if (strstr(argv[3], "peripheral"))
				strcpy(device_mode, "peripheral");

		printf("set device_mode = %s\n", device_mode);
		set_device_mode(device_mode);

		if (strcmp(argv[1], "scan") == 0) {
			sleep(1);

			//remove_dev();

			adapter_scan(1);
			timeout = 5;
			if (argc > 2)
				timeout = atoi(argv[2]);

			timeout = (timeout > 0   ? timeout : 1);
			timeout = (timeout < 180 ? timeout : 180);
			printf("scan timeout = %ds\n", timeout);
			sleep(timeout);

			adapter_scan(0);
			print_scan_results();
		} else if (strcmp(argv[1], "scanoff") == 0) {
			adapter_scan(0);
			print_scan_results();
		} else if (strcmp(argv[1], "connect") == 0) {
			if (argc > 2) {
				bddr = argv[2];
				printf("connec target : %s\n", bddr);
				bddr[2] = bddr[5] = bddr[8] = bddr[11] = bddr[14] = '_';

				ret = connect_dev(bddr);
				if (!ret)
					printf("\n*********************\n* Device Connected! *\n*********************\n");

			} else {
				printf("please set bddr!!");
				ret = 1;
			}
		} else if (strcmp(argv[1], "disconnect") == 0) {
			disconnect_dev();
		} else if (strcmp(argv[1], "status") == 0) {
			print_connect_status();
		}
	}
	a2dp_ctl_delinit();

	return ret;

}
