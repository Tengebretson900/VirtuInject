#include <grrlib.h>
#include <fat.h>
#include <wiiuse/wpad.h>
#include <ogcsys.h>
#include <dirent.h>
#include <pngu.h>
#include <gccore.h>
#include <tpl.h>

typedef unsigned char PNGU_u8;
typedef unsigned short PNGU_u16;
typedef unsigned int PNGU_u32;
typedef unsigned long long PNGU_u64;

int PNGU_DecodeToRGBA8 (IMGCTX ctx, PNGU_u32 width, PNGU_u32 height, void *buffer, PNGU_u32 stride, PNGU_u8 default_alpha);

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PNGU_OK 0

#define MAX_FILES 1024

typedef enum {
    CONSOLE_N64,
    CONSOLE_SNES
} ConsoleType;

ConsoleType console_type = CONSOLE_N64;  // default, change as needed

typedef struct {
    int active;
    int frame;
    int total_frames;
    int x_start, y_start;
    int x_end, y_end;
    int x, y;
} BRLAN_Preview;

BRLAN_Preview preview = {0};

typedef struct {
    char name[256];
    int is_dir;
} FileEntry;

FileEntry files[MAX_FILES];
int file_count = 0;
char current_path[1024] = "/";

GRRLIB_texImg *pointerTex = NULL;
GRRLIB_ttfFont *font = NULL;
GRRLIB_texImg *backTex = NULL;
GRRLIB_texImg *logoTex = NULL;

void list_dir(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    file_count = 0;

    if (strcmp(path, "/") != 0) {
        strcpy(files[file_count].name, "..");
        files[file_count].is_dir = 1;
        file_count++;
    }

    if (dir) {
        while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(files[file_count].name, 256, "%s", entry->d_name);
            files[file_count].is_dir = entry->d_type == DT_DIR;
            file_count++;
        }
        closedir(dir);
    }
}

int ends_with(const char *str, const char *suffix) {
    size_t len = strlen(str);
    size_t slen = strlen(suffix);
    return len >= slen && strcmp(str + len - slen, suffix) == 0;
}

void convert_png_to_tpl(const char *full_path) {
    mkdir("meta", 0777);
    mkdir("meta/arc", 0777);
    mkdir("meta/arc/timg", 0777);
    mkdir("meta/arc/anim", 0777);

    // SNES: copy static LogoSNES.tpl instead of converting PNG
    if (console_type == CONSOLE_SNES) {
        FILE *src = fopen("LogoSNES.tpl", "rb");
        if (src) {
            FILE *dst = fopen("meta/arc/timg/LogoSNES.tpl", "wb");
            if (dst) {
                char buf[4096];
                size_t bytes;
                while ((bytes = fread(buf, 1, sizeof(buf), src)) > 0) {
                    fwrite(buf, 1, bytes, dst);
                }
                fclose(dst);
            }
            fclose(src);
        }
        return;  // Skip PNG conversion
    }

    // N64 or others: convert PNG to TPL RGB565
    PNGUPROP prop;
    IMGCTX ctx = PNGU_SelectImageFromDevice(full_path);
    if (!ctx || PNGU_GetImageProperties(ctx, &prop) != PNGU_OK) {
        PNGU_CloseImage(ctx);
        return;
    }

    u32 *img_data = memalign(32, prop.imgWidth * prop.imgHeight * 4);
    if (!img_data) {
        PNGU_CloseImage(ctx);
        return;
    }

    if (PNGU_DecodeToRGBA8(ctx, prop.imgWidth, prop.imgHeight, img_data, 0, 0) == PNGU_OK) {
        char output_path[1024];
        const char *filename = strrchr(full_path, '/');
        snprintf(output_path, sizeof(output_path), "meta/arc/timg/%s.tpl", filename ? filename + 1 : full_path);

        size_t num_pixels = prop.imgWidth * prop.imgHeight;
        u16 *rgb565_data = (u16 *)malloc(num_pixels * sizeof(u16));
        if (!rgb565_data) {
            free(img_data);
            PNGU_CloseImage(ctx);
            return;
        }

        for (size_t i = 0; i < num_pixels; i++) {
            u8 r = (img_data[i] >> 24) & 0xFF;
            u8 g = (img_data[i] >> 16) & 0xFF;
            u8 b = (img_data[i] >> 8) & 0xFF;

            rgb565_data[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        FILE *out = fopen(output_path, "wb");
        if (out) {
            fwrite(rgb565_data, 2, num_pixels, out);
            fclose(out);
        }

        free(rgb565_data);
    }

    free(img_data);
    PNGU_CloseImage(ctx);
}

GRRLIB_texImg* LoadTPLFromFile(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    u8 *buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    fread(buffer, 1, size, f);
    fclose(f);

    TPLFile tpl;
    GXTexObj tex;
    if (TPL_OpenTPLFromMemory(&tpl, buffer, size) != 0) {
        free(buffer);
        return NULL;
    }
    if (TPL_GetTexture(&tpl, 0, &tex) != 0) {
        free(buffer);
        return NULL;
    }

    GRRLIB_texImg *grr_tex = (GRRLIB_texImg *)malloc(sizeof(GRRLIB_texImg));
    memset(grr_tex, 0, sizeof(GRRLIB_texImg));

    grr_tex->data   = GX_GetTexObjData(&tex);
    grr_tex->w      = GX_GetTexObjWidth(&tex);
    grr_tex->h      = GX_GetTexObjHeight(&tex);
    grr_tex->tilew  = grr_tex->w;
    grr_tex->tileh  = grr_tex->h;

    free(buffer);
    return grr_tex;
}

int main() {
    VIDEO_Init();
    WPAD_Init();
    fatInitDefault();
    GRRLIB_Init();
    GRRLIB_InitTTF();

    pointerTex = LoadTPLFromFile("pointer.png.tpl");  // Load from file instead of embedded
    if (pointerTex)
        GRRLIB_SetHandle(pointerTex, pointerTex->w / 2, pointerTex->h / 2);

    font = GRRLIB_LoadTTF_File("font.ttf");  // You'll need to implement GRRLIB_LoadTTF_File if not existing

    backTex = LoadTPLFromFile("Back_a.tpl");
    logoTex = LoadTPLFromFile("LogoN64.tpl");
    int banner_played = 0;

    list_dir(current_path);

    while (1) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        if (pressed & WPAD_BUTTON_HOME) break;

        ir_t ir;
        WPAD_IR(0, &ir);

        GRRLIB_FillScreen(0x303030FF);
        if (backTex)
            GRRLIB_DrawImg(0, 0, backTex, 0, 1, 1, 0xFFFFFFFF);

        for (int i = 0; i < file_count; i++) {
            int y = 40 + i * 24;
            GRRLIB_PrintfTTF(40, y, font, files[i].name, 18, 0xFFFFFFFF);

            if (ir.y > y && ir.y < y + 22 && ir.x > 20 && ir.x < 600) {
                GRRLIB_Rectangle(30, y - 2, 580, 22, 0xFFFFFF80, true);
                if (pressed & WPAD_BUTTON_A) {
                    char new_path[1024];
                    snprintf(new_path, 1024, "%s/%s", current_path, files[i].name);

                    if (files[i].is_dir) {
                        if (strcmp(files[i].name, "..") == 0) {
                            char *last_slash = strrchr(current_path, '/');
                            if (last_slash && last_slash != current_path) {
                                *last_slash = '\0';
                            } else {
                                strcpy(current_path, "/");
                            }
                        } else {
                            if (strcmp(current_path, "/") == 0)
                                snprintf(current_path, sizeof(current_path), "/%s", files[i].name);
                            else
                                snprintf(current_path, sizeof(current_path), "%s/%s", current_path, files[i].name);
                        }
                        list_dir(current_path);
                    } else if (ends_with(files[i].name, ".png")) {
                        convert_png_to_tpl(new_path);
                    }
                }
            }
        }

        if (logoTex)
            GRRLIB_DrawImg(300, 380, logoTex, 0, 0.5, 0.5, 0xFFFFFFFF);

        if (pointerTex)
            GRRLIB_DrawImg(ir.x, ir.y, pointerTex, 0, 1, 1, 0xFFFFFFFF);

        if (preview.active) {
            float t = (float)preview.frame / preview.total_frames;
            preview.x = preview.x_start + (int)((preview.x_end - preview.x_start) * t);
            preview.y = preview.y_start + (int)((preview.y_end - preview.y_start) * t);
            GRRLIB_Rectangle(preview.x, preview.y, 240, 80, 0xFF9900FF, true);
            preview.frame++;
            if (preview.frame >= preview.total_frames) preview.active = 0;
        }

        if (!banner_played) {
            FILE *f = fopen("arc/anim/banner.brlan", "rb");
            if (f) {
                char magic[4];
                fread(magic, 1, 4, f);
                fclose(f);
                if (memcmp(magic, "RLAN", 4) == 0) {
                    preview.active = 1;
                    preview.frame = 0;
                    preview.total_frames = 60;
                    preview.x_start = 80;
                    preview.y_start = 20;
                    preview.x_end = 320;
                    preview.y_end = 120;
                    banner_played = 1;
                }
            }
        }

        GRRLIB_Render();
    }

    GRRLIB_FreeTTF(font);
    if (pointerTex) GRRLIB_FreeTexture(pointerTex);
    if (backTex) free(backTex);
    if (logoTex) free(logoTex);
    GRRLIB_Exit();
    return 0;
}
