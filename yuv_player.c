//
//  yuv_player.c
//  ffmpeg_proj
//
//  Created by HorsonChan on 2021/1/30.
//

#include "yuv_player.h"
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

#define BLOCK_SIZE 4096000
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define QUIT_EVENT  (SDL_USEREVENT + 2)

int is_renderer_begin = 1;
uint32_t sleeptime = 40;//25 fps

int renderer_period(void *data) {
    
    while (is_renderer_begin) {
        SDL_Event event;
        event.type = REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(sleeptime);
    }
    
    SDL_Event event;
    event.type = QUIT_EVENT;
    SDL_PushEvent(&event);
    
    return 0;
}

int play_yuv(char *in_filename,
             int v_width,
             int v_height) {
    int rc = 0;
    int yuv_frame_len;
    
    size_t len = 0;
    size_t remain_len = 0;
    size_t blank_space_len = 0;
    
    FILE *f = NULL;
    
    uint8_t *buffer_pos = NULL;
    uint8_t *buffer_end = NULL;
    
    uint32_t pixformat = 0;
    
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Rect rect;
    SDL_Thread *sdl_thread;
    
    yuv_frame_len = v_width * v_height * 12 / 8;//yuv420
    
    uint8_t video_buf[BLOCK_SIZE];
    //init SDL
    CHECK_ERROR(((rc = SDL_Init(SDL_INIT_VIDEO)) < 0),
                "failed to init SDL",
                0);
    
    //create SDL window
    CHECK_ERROR(!(window =
                   SDL_CreateWindow("yuv render",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    v_width,
                                    v_height,
                                    SDL_WINDOW_OPENGL
                                    |SDL_WINDOW_RESIZABLE)),
                "failed to create SDL window", -1);
    
    //create SDL renderer
    CHECK_ERROR(!(renderer =
                  SDL_CreateRenderer(window, -1, 0)),
                "failed to create SDL renderer",
                -1);
    
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    pixformat = SDL_PIXELFORMAT_IYUV;
    CHECK_ERROR(!(texture =
                  SDL_CreateTexture(renderer, pixformat,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    v_width, v_height)),
                "failed to create sdl texture", -1);
    
    CHECK_ERROR(((f = fopen(in_filename, "r")) < 0),
                "failed to open yuv src file",
                0);
    
    CHECK_ERROR(((len = fread(video_buf, 1, BLOCK_SIZE, f)) < 0),
                "failed to read data from src file",
                0);
    
    buffer_pos = video_buf;
    buffer_end = video_buf + len;
    blank_space_len = BLOCK_SIZE - (int32_t)len;
    
    is_renderer_begin = 1;
    sdl_thread = SDL_CreateThread(renderer_period, "renderer_time_thread", NULL);
    
    while (1) {
        SDL_Event event;
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT) {
            if (buffer_pos + yuv_frame_len > buffer_end) {
                //memcpy remain data, including situation remain 0
                remain_len = buffer_end - buffer_pos;
                memcpy(video_buf, buffer_pos, remain_len);
                buffer_pos = video_buf;
                buffer_end = buffer_pos + remain_len;
                
                blank_space_len = BLOCK_SIZE - remain_len;
                
                printf("not enough yuv data, remain len: %zu, excute read data\n", remain_len);
                
                //read data
                CHECK_ERROR(((len =
                             fread(buffer_end,
                                   1,
                                   blank_space_len,
                                   f)) < 0),
                            "failed to read data from src file", 0);
                printf("expect read data size: %zu\n", blank_space_len);
                
                if (len == 0) {
                    printf("read data empty\n");
                    break;
                }
                
                printf("read data from yuv src file, read length: %zu, current data buffer size: %zu\n", len, len + remain_len);
                
                buffer_end += len;
                blank_space_len = BLOCK_SIZE - remain_len - len;
            }
            
            //设置想要渲染的长宽、位置，自动拉伸
            rect.x = 0;
            rect.y = 0;
            rect.w = v_width;
            rect.h = v_height;
            
            //render frame
            SDL_UpdateTexture(texture, NULL, buffer_pos, v_width);
            SDL_RenderClear(renderer);
            //最后一个可以穿NULL，整屏渲染
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_RenderPresent(renderer);
            
            buffer_pos += yuv_frame_len;
            printf("finished one period of render yuv\n");
            
        } else if (event.type == QUIT_EVENT) {
            break;
        } else if (event.type == SDL_QUIT) {
            is_renderer_begin = 0;
        }
    }
    
    is_renderer_begin = 0;
    
__exit:
    if (f) {
        fclose(f);
    }
    
    RETURN_ERROR_CHECK(rc)
    return rc;
}

#if 0
int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage command: <src_filename> <video_width> <video_height>\n");
    }
    
    char *in_filename = argv[1];
    int v_width = atoi(argv[2]);
    int v_height = atoi(argv[3]);
    play_yuv(in_filename, v_width, v_height);
}
#endif

void start_yuv_player(void) {
    play_yuv("", 640, 480);
}
