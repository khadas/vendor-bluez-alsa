#ifndef  _HFP_CTL_
#define  _HFP_CTL_

#define MSGKEY 				1357
#define MSGTYPE_TO_HOST 	1
#define MSGTYPE_FROM_HOST	2
#define MSGTEXT_SIZE		64

#define HFP_STATE_DISCONNECT_STRING			"HFP_STATE_DISCONNECT"
#define HFP_STATE_CONNECT_STRING           	"HFP_STATE_CONNECT"
#define HFP_STATE_CALL_IN_STRING            "HFP_STATE_CALL_IN"
#define HFP_STATE_CALL_OUT_STRING          	"HFP_STATE_CALL_OUT"
#define HFP_STATE_CALL_OUT_RINGING_STRING 	"HFP_STATE_CALL_OUT_RINGING"
#define HFP_STATE_CALL_IN_OUT_OVER_STRING  	"HFP_STATE_CALL_IN_OUT_OVER"
#define HFP_STATE_CALL_START_STRING        	"HFP_STATE_CALL_START"
#define HFP_STATE_CALL_ENDED_STRING			"HFP_STATE_CALL_ENDED"

struct msgstru
{
	long msgtype;
	char msgtext[MSGTEXT_SIZE];
};

int hfp_ctl_init(void);
void hfp_ctl_delinit(void);
int answer_call(void);
int reject_call(void);
int VGS_up(void);
int VGS_down(void);
int VGM_up(void);
int VGM_down(void);
int set_VGS(int value);
int set_VGM(int value);
#endif
