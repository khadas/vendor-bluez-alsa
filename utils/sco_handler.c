#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include "shared/log.h"
#include <alsa/asoundlib.h>
#include "sco_handler.h"
#include <pthread.h>

/****************************************/
/*	Stream Direction:					*/
/*	1. Income Stream: BT PCM -> SPEAKER	*/
/*	2. Output Stream: MIC -> BT PCM		*/
/****************************************/

#define INFO(fmt, args...) \
	printf("[SCO_HANDLER][%s] " fmt, __func__, ##args)

//#define DEBUG_STREAM 1

static pthread_t sco_rx_thread;
static pthread_t sco_tx_thread;
static int sco_enabled = 0;

#define ALSA_DEVICE_CONF_PATH "/etc/alsa_bt.conf"
#define DEVICE_SIZE 128
static char pcm_device_mic[DEVICE_SIZE]	    = "hw:0,2";
static char pcm_device_btpcm[DEVICE_SIZE]	= "hw:0,0";
static char pcm_device_spk[DEVICE_SIZE]		= "dmixer_auto";
static void get_alsa_device();
static void *sco_rx_cb(void *arg);
static void *sco_tx_cb(void *arg);

#ifdef AVOID_UAC
// Write a number to file path.
bool write_value(const char* path, int enable)
{
	int fd = open(path, O_WRONLY);
	if (fd < 0) {
		printf("Open %s fail: %s.\n", path, strerror(errno));
		return false;
	}

	char cmd[4];
	int ret = snprintf(cmd, sizeof(cmd), "%d", enable);
	if (ret <= 0)
		printf("Convert num %d to string fail: %s.\n", enable, strerror(errno));
	if (write(fd, cmd, strlen(cmd)) != strlen(cmd)) {
		printf("Write %s to %d fail: %s.\n", cmd, fd, strerror(errno));
	}
	close(fd);
	return true;
}

bool uac_audio_enable(int enable) {

	int path_len = 60;
	const char uac_audio_enable[] = "/sys/devices/platform/audiobridge/bridge%d/bridge_isolated";
	char uac_audio_path[path_len];
	if (snprintf(uac_audio_path, path_len, uac_audio_enable, 0) != strlen(uac_audio_path)) {
		printf("Set uac_audio_path path fail.\n");
	}
	if (!write_value(uac_audio_path,enable))
		printf("Set uac_audio fail\n");

	if (snprintf(uac_audio_path, path_len, uac_audio_enable, 1) != strlen(uac_audio_path)) {
		printf("Set uac_audio_path path fail.\n");
	}
	if (!write_value(uac_audio_path,enable))
		printf("Set uac_audio fail\n");

	return true;
}
#endif
int set_sco_enable(int enable)
{
	INFO("%s sco stream\n", enable == 0 ? "Disable" : "Enable");
	if (sco_enabled == enable) {
		INFO("sco stream %s already\n", enable == 0 ? "disabled" : "enabled");
		return 0;
	}

	sco_enabled = enable;
	if (enable) {
		get_alsa_device();
#ifdef AVOID_UAC
		INFO("enable uac virtual sound card\n");
		uac_audio_enable(1);
#endif
		if (pthread_create(&sco_rx_thread, NULL, sco_rx_cb, NULL)) {
			INFO("rx thread create failed: %s\n", strerror(errno));
			sco_enabled = 0;
			return -1;
		} else
			pthread_setname_np(sco_rx_thread, "sco_rx_thread");

		if (pthread_create(&sco_tx_thread, NULL, sco_tx_cb, NULL)) {
			INFO("tx thread create failed: %s\n", strerror(errno));
			sco_enabled = 0;
			return -1;
		} else
			pthread_setname_np(sco_tx_thread, "sco_tx_thread");

	} else {
		if (sco_rx_thread) {
			pthread_detach(sco_rx_thread);
			sco_rx_thread  = 0;
		}
		if (sco_tx_thread) {
			pthread_detach(sco_tx_thread);
			sco_tx_thread  = 0;
		}
	}

	return 0;

}

static void get_alsa_device()
{
	FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int i;

	/* conf format:
		spk=dimxer_avs_auto
		mic=hw:0,2
		pcm=hw:0,0
	*/

	stream = fopen(ALSA_DEVICE_CONF_PATH, "r");
	if (stream == NULL) {
		INFO("open ALSA DEVICE CONF error, use default\n");
		char temp[64] = "spk=dimxer_avs_auto\nmic=hw:0,2\npcm=hw:0,0\n";
		INFO("If soundcard devcie changed, set them at %s like:\n%s", ALSA_DEVICE_CONF_PATH, temp);
		return;
	}

	while ((read = getline(&line, &len, stream)) != -1) {
		INFO("Retrieved line of length %zu :\n", read);
		INFO("%s\n", line);
		/* remove the newline char */
		line[read - 1] = 0;
		if (strstr(line, "spk")) {
			memset(pcm_device_spk, 0, DEVICE_SIZE);
			strncpy(pcm_device_spk, line + 4, DEVICE_SIZE - 1);
			for (i = 0; i < 48; i++)
				printf("%x ", pcm_device_spk[i]);
			printf("\n");
		} else if (strstr(line, "mic")) {
			memset(pcm_device_mic, 0, DEVICE_SIZE);
			strncpy(pcm_device_mic, line + 4, DEVICE_SIZE - 1);
		} else if (strstr(line, "pcm")) {
			memset(pcm_device_btpcm, 0, DEVICE_SIZE) ;
			strncpy(pcm_device_btpcm, line + 4, DEVICE_SIZE - 1);
		}
	}

	free(line);
	fclose(stream);

}

static void *sco_rx_cb(void *arg)
{
	snd_pcm_t *pcm_handle_capture;
	snd_pcm_t *pcm_handle_playback;
	snd_pcm_hw_params_t *snd_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	const int channels = 1, expected_frames = 60;
	int dir, status, buf_size, rate = 8000;
	char *buf;
	snd_pcm_uframes_t frames = 120;

	INFO("setting bt pcm\n");
	/*********BT PCM ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_capture, pcm_device_btpcm,
						  SND_PCM_STREAM_CAPTURE,	/*bt pcm as input device*/
						  0);		 				/*ASYNC MODE*/


	if (status < 0) {
		INFO("bt pcm open fail:%s\n", strerror(errno));
		return NULL;
	}
#if 0
	status = snd_pcm_set_params(pcm_handle_capture, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								0,				/*allow alsa resample*/
								100000);		/*max latence = 100ms*/
#endif
	snd_pcm_hw_params_alloca(&snd_params);
	snd_pcm_hw_params_any(pcm_handle_capture, snd_params);
	snd_pcm_hw_params_set_access(pcm_handle_capture, snd_params,
								 SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle_capture, snd_params, format);
	snd_pcm_hw_params_set_channels(pcm_handle_capture, snd_params, channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle_capture, snd_params, &rate, 0);
	snd_pcm_hw_params_set_period_size_near(pcm_handle_capture, snd_params, &frames, &dir);


	status = snd_pcm_hw_params(pcm_handle_capture, snd_params);

	if (status < 0) {
		INFO("bt pcm set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		return NULL;
	}

	INFO("setting speaker\n");
	/*********SPEAKER ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_playback, pcm_device_spk,
						  SND_PCM_STREAM_PLAYBACK,	/*speaker as output device*/
						  1);						/*NOBLOCK MODE*/
	if (status < 0) {
		INFO("speaker open fail:%s\n", strerror(errno));
		return NULL;
	}

	status = snd_pcm_set_params(pcm_handle_playback, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								1,				/*allow alsa resample*/
								500000);		/*expected max latence = 500ms*/

	if (status < 0) {
		INFO("bt pcm set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		snd_pcm_close(pcm_handle_playback);
		return NULL;
	}

#ifdef DEBUG_STREAM
	FILE *fp;
	fp = fopen("/tmp/rx_cb.wav", "w+");
	if (fp  == NULL)
		INFO("create wav file error");
#endif

	INFO("start streaming\n");
	/*********STREAM HANDLING BEGIN***************************/
	buf_size = expected_frames * 1 * 2;	/*bytes = frames * ch * 16Bit/8 */
	buf = malloc(buf_size);
	frames = 0;
	while (sco_enabled) {
		memset(buf, 0, buf_size);
		frames = snd_pcm_readi(pcm_handle_capture, buf, expected_frames);
		if (frames == -EPIPE) {
			INFO("pcm read underrun\n");
			snd_pcm_prepare(pcm_handle_capture);
			frames = 0;
			continue;
		} else if (frames == -EBADFD) {
			INFO("pcm read EBADFD, retring\n");
			frames = 0;
			continue;
		}

#ifdef DEBUG_STREAM
		if (fp)
			fwrite(buf, frames * 2, 1, fp);
#endif

		frames = snd_pcm_writei(pcm_handle_playback, buf, frames);
		/*if write failed somehow, just ignore, we don't want to wast too much time*/
		if (frames == -EPIPE) {
			INFO("speaker write underrun\n");
			snd_pcm_prepare(pcm_handle_playback);
		} else if (frames == -EBADFD) {
			INFO("speaker write  EBADFD\n");
		}
	}
	INFO("stopping\n");
	/********STOP**********************************************/
	snd_pcm_close(pcm_handle_capture);
	snd_pcm_close(pcm_handle_playback);
	free(buf);
#ifdef DEBUG_STREAM
	fclose(fp);
#endif
#ifdef AVOID_UAC
	INFO("disable uac virtual sound card\n");
	uac_audio_enable(0);
#endif
	return NULL;
}


static void *sco_tx_cb(void *arg)
{
	snd_pcm_t *pcm_handle_capture;
	snd_pcm_t *pcm_handle_playback;
	snd_pcm_hw_params_t *snd_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	const int mic_channels = 1, expected_frames = 60;
	int dir, status, buf_size, i, rate = 8000;
	char *buf;
	snd_pcm_uframes_t frames = 120;

	INFO("setting mic\n");
	/********MIC ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_capture, pcm_device_mic,
						  SND_PCM_STREAM_CAPTURE,	/*mic as input device*/
						  0);						/*ASYNC MODE*/


	if (status < 0) {
		INFO("mic open fail:%s\n", strerror(errno));
		return NULL;
	}
#if 0
	status = snd_pcm_set_params(pcm_handle_capture, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								0,				/*allow alsa resample*/
								100000);		/*max latence = 100ms*/
#else
	snd_pcm_hw_params_alloca(&snd_params);
	snd_pcm_hw_params_any(pcm_handle_capture, snd_params);
	snd_pcm_hw_params_set_access(pcm_handle_capture, snd_params,
								 SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm_handle_capture, snd_params, format);
	snd_pcm_hw_params_set_channels(pcm_handle_capture, snd_params, mic_channels);
	snd_pcm_hw_params_set_rate_near(pcm_handle_capture, snd_params, &rate, 0);
	snd_pcm_hw_params_set_period_size_near(pcm_handle_capture, snd_params, &frames, &dir);


	status = snd_pcm_hw_params(pcm_handle_capture, snd_params);
#endif

	if (status < 0) {
		INFO("mic set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		return NULL;
	}

	INFO("setting bt pcm\n");
	/*********BT PCM ALSA SETTING****************************/
	status = snd_pcm_open(&pcm_handle_playback, pcm_device_btpcm,
						  SND_PCM_STREAM_PLAYBACK,	/*bt pcm as output device*/
						  1);						/*NOBLOCK MODE*/
	if (status < 0) {
		INFO("bt pcm open fail:%s\n", strerror(errno));
		return NULL;
	}

	status = snd_pcm_set_params(pcm_handle_playback, SND_PCM_FORMAT_S16_LE,
								SND_PCM_ACCESS_RW_INTERLEAVED,
								1,				/*1 channel*/
								8000,			/*8k sample rete*/
								1,				/*allow alsa resample*/
								500000);		/*expected max latence = 500ms*/

	if (status < 0) {
		INFO("bt pcm set params fail:%s\n", strerror(errno));
		snd_pcm_close(pcm_handle_capture);
		snd_pcm_close(pcm_handle_playback);
		return NULL;
	}

#ifdef DEBUG_STREAM
	FILE *fp;
	fp = fopen("/tmp/tx_cb.wav", "w+");
	if (fp  == NULL)
		INFO("create wav file error");
#endif

	INFO("start streaming\n");
	/*********STREAM HANDLING BEGIN***************************/
	buf_size  = expected_frames * mic_channels * 2;	/*bytes = frames * ch * 16Bit/8 */
	buf  = malloc(buf_size);

	while (sco_enabled) {

		memset(buf, 0, buf_size);

		frames = snd_pcm_readi(pcm_handle_capture, buf, expected_frames);
		if (frames == -EPIPE) {
			INFO("mic read underrun\n");
			snd_pcm_prepare(pcm_handle_capture);
			frames = 0;
			continue;
		} else if (frames == -EBADFD) {
			INFO("mic read EBADFD, retring\n");
			frames = 0;
			continue;
		}

#ifdef DEBUG_STREAM
		if (fp)
			fwrite(buf, frames * 2, 1, fp);
#endif

		frames = snd_pcm_writei(pcm_handle_playback, buf, frames);
		/*if write failed somehow, just ignore, we don't want to wast too much time*/
		if (frames == -EPIPE) {
			INFO("bt pcm write underrun\n");
			snd_pcm_prepare(pcm_handle_playback);
		} else if (frames == -EBADFD) {
			INFO("bt pcm write  EBADFD\n");
		}
	}
	INFO("stopping\n");
	/********STOP**********************************************/
	snd_pcm_close(pcm_handle_capture);
	snd_pcm_close(pcm_handle_playback);
	free(buf);
#ifdef DEBUG_STREAM
	fclose(fp);
#endif
#ifdef AVOID_UAC
	INFO("disable uac virtual sound card\n");
	uac_audio_enable(0);
#endif
	return NULL;
}

