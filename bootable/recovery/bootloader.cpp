/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include <fs_mgr.h>
#include "bootloader.h"
#include "common.h"
#include "mtdutils/mtdutils.h"
#include "roots.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#if 1 //wschen 2012-07-10
#include <sys/types.h>
#include <fcntl.h>
#endif

static int get_bootloader_message_mtd(struct bootloader_message *out, const Volume* v);
static int set_bootloader_message_mtd(const struct bootloader_message *in, const Volume* v);
static int get_bootloader_message_block(struct bootloader_message *out, const Volume* v);
static int set_bootloader_message_block(const struct bootloader_message *in, const Volume* v);

#ifdef __cplusplus
extern "C" {
#endif
bool is_support_gpt(void) {
    int fd = open("/dev/block/platform/mtk-msdc.0/by-name/para", O_RDONLY);
    if (fd == -1) {
        LOGI("GPT not supported!\n");
        return false;
    } else {
        LOGI("GPT is supported!\n");
        close(fd);
        return true;
    }
}
#ifdef __cplusplus
}
#endif


int get_bootloader_message(struct bootloader_message *out) {
    Volume* v = volume_for_path("/misc");
    if (v == NULL) {
      LOGE("Cannot load volume /misc!\n");
      return -1;
    }
    if (strcmp(v->fs_type, "mtd") == 0) {
        return get_bootloader_message_mtd(out, v);
    } else if (strcmp(v->fs_type, "emmc") == 0) {
        return get_bootloader_message_block(out, v);
    }
    LOGE("unknown misc partition fs_type \"%s\"\n", v->fs_type);
    return -1;
}

int set_bootloader_message(const struct bootloader_message *in) {
    Volume* v = volume_for_path("/misc");
    if (v == NULL) {
      LOGE("Cannot load volume /misc!\n");
      return -1;
    }
    if (strcmp(v->fs_type, "mtd") == 0) {
        return set_bootloader_message_mtd(in, v);
    } else if (strcmp(v->fs_type, "emmc") == 0) {
        return set_bootloader_message_block(in, v);
    }
    LOGE("unknown misc partition fs_type \"%s\"\n", v->fs_type);
    return -1;
}

#if 1 //wschen 2012-04-12

int get_phone_encrypt_state(struct phone_encrypt_state *out) {

    Volume *v = volume_for_path("/misc");

    if (v == NULL) {
      LOGE("Cannot load volume /misc!\n");
      return -1;
    }

    if (strcmp(v->fs_type, "emmc") == 0) {
        int dev = -1;
        char dev_name[256];
        struct phone_encrypt_state temp;
        int count;

        if (is_support_gpt()) {
            snprintf(dev_name, sizeof(dev_name), "%s", v->device);
        } else {
            strcpy(dev_name, "/dev/");
            strcat(dev_name, v->device);
        }

        dev = open(dev_name, O_RDONLY);
        if (dev < 0)  {
            LOGE("Can't open%s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }

        if (lseek(dev, PHONE_ENCRYPT_OFFSET, SEEK_SET) == -1) {
            LOGE("Failed seeking %s\n(%s)\n", dev_name, strerror(errno));
            close(dev);
            return -1;
        }

        count = read(dev, &temp, sizeof(temp));

        if (count != sizeof(temp)) {
            LOGE("Failed reading %s\n(%s)\n", dev_name, strerror(errno));
            close(dev);
            return -1;
        }

        if (close(dev) != 0) {
            LOGE("Failed closing %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }

        memcpy(out, &temp, sizeof(temp));
        return 0;

    } else {
        out->state = 0;
        return 0;
    }
}


int set_phone_encrypt_state(const struct phone_encrypt_state *in) {

    Volume *v = volume_for_path("/misc");
    if (v == NULL) {
      LOGE("Cannot load volume /misc!\n");
      return -1;
    }

    if (strcmp(v->fs_type, "emmc") == 0) {
        int dev = -1;
        char dev_name[256];
        int count;

        if (is_support_gpt()) {
            snprintf(dev_name, sizeof(dev_name), "%s", v->device);
        } else {
            strcpy(dev_name, "/dev/");
            strcat(dev_name, v->device);
        }

        dev = open(dev_name, O_WRONLY | O_SYNC);
        if (dev < 0)  {
            LOGE("Can't open %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }

        if (lseek(dev, PHONE_ENCRYPT_OFFSET, SEEK_SET) == -1) {
            LOGE("Failed seeking %s\n(%s)\n", dev_name, strerror(errno));
            close(dev);
            return -1;
        }

        count = write(dev, in, sizeof(*in));
        if (count != sizeof(*in)) {
            LOGE("Failed writing %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }
        if (close(dev) != 0) {
            LOGE("Failed closing %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }
        return 0;

    } else {
        return 0;
    }
}

int set_ota_result(int result) {

    Volume *v = volume_for_path("/misc");
    if (v == NULL) {
      LOGE("Cannot load volume /misc!\n");
      return -1;
    }

    if (strcmp(v->fs_type, "emmc") == 0) {
        int dev = -1;
        char dev_name[256];
        int count;

        if (is_support_gpt()) {
            snprintf(dev_name, sizeof(dev_name), "%s", v->device);
        } else {
            strcpy(dev_name, "/dev/");
            strcat(dev_name, v->device);
        }

        dev = open(dev_name, O_WRONLY | O_SYNC);
        if (dev < 0)  {
            LOGE("Can't open %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }

        if (lseek(dev, OTA_RESULT_OFFSET, SEEK_SET) == -1) {
            LOGE("Failed seeking %s\n(%s)\n", dev_name, strerror(errno));
            close(dev);
            return -1;
        }

        count = write(dev, &result, sizeof(result));
        if (count != sizeof(result)) {
            LOGE("Failed writing %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }
        if (close(dev) != 0) {
            LOGE("Failed closing %s\n(%s)\n", dev_name, strerror(errno));
            return -1;
        }
        sync();
        return 0;

    } else {
        return 0;
    }

}

#endif

// ------------------------------
// for misc partitions on MTD
// ------------------------------

static const int MISC_PAGES = 3;         // number of pages to save
static const int MISC_COMMAND_PAGE = 1;  // bootloader command is this page

static int get_bootloader_message_mtd(struct bootloader_message *out,
                                      const Volume* v) {
    size_t write_size;
    mtd_scan_partitions();
    const MtdPartition *part = mtd_find_partition_by_name(v->device);
    if (part == NULL || mtd_partition_info(part, NULL, NULL, &write_size)) {
        LOGE("Can't find %s\n", v->device);
        return -1;
    }

    MtdReadContext *read = mtd_read_partition(part);
    if (read == NULL) {
        LOGE("Can't open %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }

    const ssize_t size = write_size * MISC_PAGES;
    char data[size];
    ssize_t r = mtd_read_data(read, data, size);
    if (r != size) LOGE("Can't read %s\n(%s)\n", v->device, strerror(errno));
    mtd_read_close(read);
    if (r != size) return -1;

    memcpy(out, &data[write_size * MISC_COMMAND_PAGE], sizeof(*out));
    return 0;
}
static int set_bootloader_message_mtd(const struct bootloader_message *in,
                                      const Volume* v) {
    size_t write_size;
    mtd_scan_partitions();
    const MtdPartition *part = mtd_find_partition_by_name(v->device);
    if (part == NULL || mtd_partition_info(part, NULL, NULL, &write_size)) {
        LOGE("Can't find %s\n", v->device);
        return -1;
    }

    MtdReadContext *read = mtd_read_partition(part);
    if (read == NULL) {
        LOGE("Can't open %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }

    ssize_t size = write_size * MISC_PAGES;
    char data[size];
    ssize_t r = mtd_read_data(read, data, size);
    if (r != size) LOGE("Can't read %s\n(%s)\n", v->device, strerror(errno));
    mtd_read_close(read);
    if (r != size) return -1;

    memcpy(&data[write_size * MISC_COMMAND_PAGE], in, sizeof(*in));

    MtdWriteContext *write = mtd_write_partition(part);
    if (write == NULL) {
        LOGE("Can't open %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    if (mtd_write_data(write, data, size) != size) {
        LOGE("Can't write %s\n(%s)\n", v->device, strerror(errno));
        mtd_write_close(write);
        return -1;
    }
    if (mtd_write_close(write)) {
        LOGE("Can't finish %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }

    LOGI("Set boot command \"%s\"\n", in->command[0] != 255 ? in->command : "");
    return 0;
}


// ------------------------------------
// for misc partitions on block devices
// ------------------------------------

#if 0 //wschen 2012-07-10
static void wait_for_device(const char* fn) {
    int tries = 0;
    int ret;
    struct stat buf;
    do {
        ++tries;
        ret = stat(fn, &buf);
        if (ret) {
            printf("stat %s try %d: %s\n", fn, tries, strerror(errno));
            sleep(1);
        }
    } while (ret && tries < 10);
    if (ret) {
        printf("failed to stat %s\n", fn);
    }
}
#endif

static int get_bootloader_message_block(struct bootloader_message *out,
                                        const Volume* v) {
#if 0 //wschen 2012-07-10
    wait_for_device(v->device);
    FILE* f = fopen(v->device, "rb");
    if (f == NULL) {
        LOGE("Can't open %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    struct bootloader_message temp;
    int count = fread(&temp, sizeof(temp), 1, f);
    if (count != 1) {
        LOGE("Failed reading %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        LOGE("Failed closing %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    memcpy(out, &temp, sizeof(temp));
    return 0;
#else
    int dev = -1;
    char dev_name[256];

    if (is_support_gpt()) {
        snprintf(dev_name, sizeof(dev_name), "%s", v->device);
    } else {
        strcpy(dev_name, "/dev/");
        strcat(dev_name, v->device);
    }

    dev = open(dev_name, O_RDONLY);
    if (dev < 0)  {
        LOGE("Can't open %s (%s)\n", dev_name, strerror(errno));
        return -1;
    }

    struct bootloader_message temp;
    int count = read(dev, &temp, sizeof(temp));
    if (count != sizeof(temp)) {
        LOGE("Failed reading %s (%s)\n", dev_name, strerror(errno));
        close(dev);
        return -1;
    }
    if (close(dev) != 0) {
        LOGE("Failed closing %s (%s)\n", dev_name, strerror(errno));
        return -1;
    }
    memcpy(out, &temp, sizeof(temp));
    return 0;
#endif
}

static int set_bootloader_message_block(const struct bootloader_message *in,
                                        const Volume* v) {
#if 0 //wschen 2012-07-10
    wait_for_device(v->device);
    FILE* f = fopen(v->device, "wb");
    if (f == NULL) {
        LOGE("Can't open %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    int count = fwrite(in, sizeof(*in), 1, f);
    if (count != 1) {
        LOGE("Failed writing %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    if (fclose(f) != 0) {
        LOGE("Failed closing %s\n(%s)\n", v->device, strerror(errno));
        return -1;
    }
    return 0;
#else
    int dev = -1;
    char dev_name[256];

    if (is_support_gpt()) {
        snprintf(dev_name, sizeof(dev_name), "%s", v->device);
    } else {
        strcpy(dev_name, "/dev/");
        strcat(dev_name, v->device);
    }

    dev = open(dev_name, O_WRONLY | O_SYNC);
    if (dev < 0)  {
        LOGE("Can't open %s (%s)\n", dev_name, strerror(errno));
        return -1;
    }
    int count = write(dev, in, sizeof(*in));
    if (count != sizeof(*in)) {
        LOGE("Failed writing %s (%s)\n", dev_name, strerror(errno));
        close(dev);
        return -1;
    }
    if (close(dev) != 0) {
        LOGE("Failed closing %s (%s)\n", dev_name, strerror(errno));
        return -1;
    }
    return 0;
#endif
}
