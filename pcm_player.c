//
//  pcm_player.c
//  ffmpeg_proj
//
//  Created by HorsonChan on 2021/1/30.
//

#include "pcm_player.h"
#include <SDL.h>

#define CHECK_ERROR(cond, inf, code) \
if (cond) { \
    if (code) rc = code; \
    printf("[demo log] %s\n", inf); \
    goto __exit; \
}

#define RETURN_ERROR_CHECK(rc)\
if (rc < 0) {\
    printf("[demo log] error occured, code: %d",\
            rc); \
    return rc;\
}

//4M
#define BLOCK_SIZE 4096000

static uint8_t *audio_buf = NULL;
static uint8_t *audio_pos;
static size_t rlen = 0;
static void audio_callback(void *userdata,
                    Uint8 * stream,
                    int len) {
    if (rlen == 0) {
        return;
    }
    
    SDL_memset(stream, 0, len);
    
    len = len < rlen ? len : (int)rlen;
    
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len;
    
    rlen -= len;
}

int play_pcm(char *in_filename,
             uint8_t channels,
             int sample_rate) {
    int rc = 0;
    FILE *f = NULL;
    uint8_t *audio_end;
    
    CHECK_ERROR((rc = (SDL_Init(SDL_INIT_AUDIO))),
                "failed to init SDL",
                0)
    
    CHECK_ERROR(!(f = fopen(in_filename, "r")),
                "failed to open pcm src file",
                -1)
        
    CHECK_ERROR(!(audio_buf = (uint8_t *)malloc(BLOCK_SIZE)),
                "failed to allocate audio_buf in memory",
                -1)
    
    SDL_AudioSpec spec;
    spec.freq = sample_rate;
    spec.channels = channels;
    spec.format = AUDIO_F32LSB;
    spec.callback = audio_callback;
    spec.userdata = NULL;//call back data(optional)
    CHECK_ERROR((SDL_OpenAudio(&spec, NULL)),
                "failed to open audio device",
                -1)
    
    //play
    SDL_PauseAudio(0);
    
    while(1) {
        rlen = fread(audio_buf, 1, BLOCK_SIZE, f);
        if (rlen <= 0) {
            printf("read audio file EOF\n");
            break;
        }
        audio_pos = audio_buf;
        audio_end = audio_pos + rlen;
        while (audio_pos < audio_end) {
            SDL_Delay(1);
        }
    }
    
    SDL_CloseAudio();
    
__exit:
    SDL_Quit();
    
    if (audio_buf) {
        free(audio_buf);
    }
    
    if (f) {
        fclose(f);
    }
    
    RETURN_ERROR_CHECK(rc);
    return 0;
}

void start_pcm_player(void) {
    play_pcm("/Users/horson/Desktop/ffmpeg_learning_demo/practice/6.ffmpeg_proj/ffmpeg_proj/ffmpeg_proj/8.pcm_player/audio.pcm", 2, 48000);
}

#if 0
int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage command: <in_filename> <channels> <sample rate>\n");
        return -1;
    }
    char *in_filename = argv[1];
    uint8_t channels = atoi(argv[2]);
    int sample_rate = atoi(argv[3]);
    play_pcm(in_filename, channels, sample_rate);
    return 0;
}
#endif
