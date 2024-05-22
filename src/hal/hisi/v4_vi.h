#pragma once

#include "v4_common.h"

typedef enum {
    V4_VI_INPUT_VUVU,
    V4_VI_INPUT_UVUV,
    V4_VI_INPUT_UYVY,
    V4_VI_INPUT_VYUY,
    V4_VI_INPUT_YUYV,
    V4_VI_INPUT_YVYU,
    V4_VI_INPUT_END
} v4_vi_input;

typedef enum {
	V4_VI_INTF_BT656,
	V4_VI_INTF_BT656_PACKED_YUV,
	V4_VI_INTF_BT601,
	V4_VI_INTF_DIGITAL_CAMERA,
	V4_VI_INTF_BT1120_STANDARD,
	V4_VI_INTF_BT1120_INTERLEAVED,
	V4_VI_INTF_MIPI,
	V4_VI_INTF_MIPI_YUV420_NORMAL,
	V4_VI_INTF_MIPI_YUV420_LEGACY,
	V4_VI_INTF_MIPI_YUV422,
	V4_VI_INTF_LVDS,
	V4_VI_INTF_HISPI,
	V4_VI_INTF_SLVS,
    V4_VI_INTF_END
} v4_vi_intf;

typedef enum {
    V4_VI_REPHASE_NONE,
    V4_VI_REPHASE_SKIP1_2,
    V4_VI_REPHASE_SKIP1_3,
    V4_VI_REPHASE_BINNING1_2,
    V4_VI_REPHASE_BINNING1_3,
    V4_VI_REPHASE_END
} v4_vi_rephase;

typedef enum {
    V4_VI_WORK_1MULTIPLEX,
    V4_VI_WORK_2MULTIPLEX,
    V4_VI_WORK_4MULTIPLEX
} v4_vi_work;

typedef struct {
    int hsyncFront;
    int hsyncWidth;
    int hsyncBack;
    int vsyncFront;
    int vsyncWidth;
    int vsyncBack;
    // Next three are valid on interlace mode
    // and define even-frame timings
    int vsyncIntrlFront;
    int vsyncIntrlWidth;
    int vsyncIntrlBack;
} v4_vi_timing;

typedef struct {
    int vsyncPulse;
    int vsyncInv;
    int hsyncPulse;
    int hsyncInv;
    int vsyncValid;
    int vsyncValidInv;
    v4_vi_timing timing;
} v4_vi_sync;

typedef struct {
    v4_common_rect capt;
    v4_common_dim dest;
    // Values 0-3 correspond to Top, Bottom, Both
    int field;
    v4_common_pixfmt pixFmt;
    v4_common_compr compress;
    int mirror;
    int flip;
    int srcFps;
    int dstFps;
} v4_vi_chn;

typedef struct {
    v4_vi_intf intf;
    v4_vi_work work;
    int separateOn;
    int doubleCompOn;
    int clkDownOn;
    unsigned int cmpntMask[2];
    int progressiveOn;
    int adChn[4];
    v4_vi_input input;
    v4_vi_sync sync;
    int codeZeroOn;
    int polarNstdOn;
    // Accepts values between 0-2
    // (0: bypass ISP, 1: enable ISP, 2: raw)
    int dataPath;
    int rgbModeOn;
    int dataRev;
    v4_common_rect capt;
    v4_vi_rephase hsyncBayerReph;
    v4_vi_rephase vsyncBayerReph;
    v4_common_dim bayerSize;
    int bayerComprOn;
} v4_vi_dev;

typedef struct {
    v4_common_wdr mode;
    unsigned int cacheLine;
} v4_vi_wdr;

typedef struct {
    void *handle, *handleGoke;

    int (*fnDisableDevice)(int device);
    int (*fnEnableDevice)(int device);
    int (*fnSetDeviceConfig)(int device, v4_vi_dev *config);

    int (*fnDisableChannel)(int channel);
    int (*fnEnableChannel)(int channel);
    int (*fnSetChannelConfig)(int channel, v4_vi_chn *config);
} v4_vi_impl;

static int v4_vi_load(v4_vi_impl *vi_lib) {
    if ( !(vi_lib->handle = dlopen("libmpi.so", RTLD_LAZY | RTLD_GLOBAL)) &&

        (!(vi_lib->handleGoke = dlopen("libgk_api.so", RTLD_LAZY | RTLD_GLOBAL)) ||
         !(vi_lib->handle = dlopen("libhi_mpi.so", RTLD_LAZY | RTLD_GLOBAL)))) {
        fprintf(stderr, "[v4_vi] Failed to load library!\nError: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    if (!(vi_lib->fnDisableDevice = (int(*)(int device))
        dlsym(vi_lib->handle, "HI_MPI_VI_DisableDev"))) {
        fprintf(stderr, "[v4_vi] Failed to acquire symbol HI_MPI_VI_DisableDev!\n");
        return EXIT_FAILURE;
    }

    if (!(vi_lib->fnEnableDevice = (int(*)(int device))
        dlsym(vi_lib->handle, "HI_MPI_VI_EnableDev"))) {
        fprintf(stderr, "[v4_vi] Failed to acquire symbol HI_MPI_VI_EnableDev!\n");
        return EXIT_FAILURE;
    }

    if (!(vi_lib->fnSetDeviceConfig = (int(*)(int device, v4_vi_dev *config))
        dlsym(vi_lib->handle, "HI_MPI_VI_SetDevAttr"))) {
        fprintf(stderr, "[v4_vi] Failed to acquire symbol HI_MPI_VI_SetDevAttr!\n");
        return EXIT_FAILURE;
    }

    if (!(vi_lib->fnDisableChannel = (int(*)(int device))
        dlsym(vi_lib->handle, "HI_MPI_VI_DisableChn"))) {
        fprintf(stderr, "[v4_vi] Failed to acquire symbol HI_MPI_VI_DisableChn!\n");
        return EXIT_FAILURE;
    }

    if (!(vi_lib->fnEnableChannel = (int(*)(int device))
        dlsym(vi_lib->handle, "HI_MPI_VI_EnableChn"))) {
        fprintf(stderr, "[v4_vi] Failed to acquire symbol HI_MPI_VI_EnableChn!\n");
        return EXIT_FAILURE;
    }

    if (!(vi_lib->fnSetChannelConfig = (int(*)(int device, v4_vi_chn *config))
        dlsym(vi_lib->handle, "HI_MPI_VI_SetChnAttr"))) {
        fprintf(stderr, "[v4_vi] Failed to acquire symbol HI_MPI_VI_SetChnAttr!\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static void v4_vi_unload(v4_vi_impl *vi_lib) {
    if (vi_lib->handle) dlclose(vi_lib->handle);
    vi_lib->handle = NULL;
    if (vi_lib->handleGoke) dlclose(vi_lib->handleGoke);
    vi_lib->handleGoke = NULL;
    memset(vi_lib, 0, sizeof(*vi_lib));
}