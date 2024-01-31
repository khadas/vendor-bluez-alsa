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

#ifndef _CT_H_
#define _CT_H_


typedef void (*Connect_Callback)(gboolean connected);
typedef void (*Play_Callback)(char *status);
typedef void (*Volume_Callback)(guint16 volume);
int a2dp_ctl_init(Connect_Callback con_cb, Play_Callback play_cb, Volume_Callback volume_cb);
void a2dp_ctl_delinit(void);
int start_play(void);
int stop_play(void);
int pause_play(void);
int next(void);
int previous(void);
int volume_up();
int volume_down();
int volume_set(int volume);
void set_device_mode(char *dm);
int adapter_ready(void);
int adapter_scan(int onoff);
int connect_dev(const char* bddr);
void disconnect_dev(void);
void print_connect_status(void);
void print_scan_results(void);
const char * get_connected_dev_addr(void);
gint16 get_connected_dev_volume(void);

int pcm_bluealsa_open(char *bddr);
int pcm_bluealsa_close();
int pcm_bluealsa_write(void *buf, size_t bytes);
#endif
