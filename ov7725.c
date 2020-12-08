/*
 * Copyright (c) 2015 iWave Systems Technologies Pvt. Ltd. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <media/v4l2-chip-ident.h>
#include "v4l2-int-device.h"
#include "mxc_v4l2_capture.h"

#define MIN_FPS 15
#define MAX_FPS 30
#define DEFAULT_FPS 30

#define OV7725_XCLK_MIN 6000000
#define OV7725_XCLK_MAX 24000000

#define OV7725_CHIP_ID_HIGH_BYTE        0x0A
#define OV7725_CHIP_ID_LOW_BYTE         0x0B

enum ov7725_mode {
       ov7725_mode_MIN = 0,
       ov7725_mode_VGA_640_480 = 0,
       ov7725_mode_MAX = 0
};

enum ov7725_frame_rate {
       ov7725_30_fps
};

struct reg_value {
       u8 u8RegAddr;
       u8 u8Val;
       u8 u8Mask;
       u8 u8Delay_ms;
};

struct ov7725_mode_info {
       enum ov7725_mode mode;
       u32 width;
       u32 height;
       struct reg_value *init_data_ptr;
       u32 init_data_size;
};

/*!
 * Maintains the information on the current state of the sesor.
 */
static struct sensor_data ov7725_data;
static int pwn_gpio, rst_gpio;

static struct reg_value ov7725_setting_30fps_VGA_640_480[] = {
        {0x12, 0x80, 0, 30},
        {0x3d, 0x03, 0, 0},
        {0x17, 0x22, 0, 0},
        {0x18, 0xa4, 0, 0},
        {0x19, 0x07, 0, 0},
        {0x1a, 0xf0, 0, 0},
        {0x32, 0x00, 0, 0},
        {0x29, 0xa0, 0, 0},
        {0x2c, 0xf0, 0, 0},
        {0x2a, 0x00, 0, 0},
        {0x11, 0x03, 0, 0},
        {0x42, 0x7f, 0, 0},
        {0x4d, 0x09, 0, 0},
        {0x63, 0xe0, 0, 0},
        {0x64, 0xff, 0, 0},
        {0x65, 0x20, 0, 0},
        {0x66, 0x00, 0, 0},
        {0x67, 0x48, 0, 0},
        {0x13, 0xf0, 0, 0},
        {0x0d, 0x41, 0, 0},
        {0x0f, 0xc5, 0, 0},
        {0x14, 0x11, 0, 0},
        {0x22, 0x3f, 0, 0},
        {0x23, 0x07, 0, 0},
        {0x24, 0x40, 0, 0},
        {0x25, 0x30, 0, 0},
        {0x26, 0xa1, 0, 0},
        {0x2b, 0x00, 0, 0},
        {0x6b, 0xaa, 0, 0},
        {0x13, 0xff, 0, 0},
        {0x90, 0x05, 0, 0},
        {0x91, 0x01, 0, 0},
        {0x92, 0x03, 0, 0},
        {0x93, 0x00, 0, 0},
        {0x94, 0xb0, 0, 0},
        {0x95, 0x9d, 0, 0},
        {0x96, 0x13, 0, 0},
        {0x97, 0x16, 0, 0},
        {0x98, 0x7b, 0, 0},
        {0x99, 0x91, 0, 0},
        {0x9a, 0x1e, 0, 0},
        {0x9b, 0x08, 0, 0},
        {0x9c, 0x20, 0, 0},
        {0x9e, 0x81, 0, 0},
        {0xa6, 0x04, 0, 0},
        {0x7e, 0x0c, 0, 0},
        {0x7f, 0x16, 0, 0},
        {0x80, 0x2a, 0, 0},
        {0x81, 0x4e, 0, 0},
        {0x82, 0x61, 0, 0},
        {0x83, 0x6f, 0, 0},
        {0x84, 0x7b, 0, 0},
        {0x85, 0x86, 0, 0},
        {0x86, 0x8e, 0, 0},
        {0x87, 0x97, 0, 0},
        {0x88, 0xa4, 0, 0},
        {0x89, 0xaf, 0, 0},
        {0x8a, 0xc5, 0, 0},
        {0x8b, 0xd7, 0, 0},
        {0x8c, 0xe8, 0, 0},
        {0x8d, 0x20, 0, 0},
        {0x11, 0x01, 0, 0},
        {0x22, 0x7f, 0, 0}, /*0x99*/
        {0x23, 0x03, 0, 0},
};

static struct ov7725_mode_info ov7725_mode_info_data[1][ov7725_mode_MAX + 1] = {
       {
               {ov7725_mode_VGA_640_480,      640,  480,
               ov7725_setting_30fps_VGA_640_480,
               ARRAY_SIZE(ov7725_setting_30fps_VGA_640_480)},
       },
};

static int ov7725_probe(struct i2c_client *adapter,
                               const struct i2c_device_id *device_id);
static int ov7725_remove(struct i2c_client *client);

static s32 ov7725_read_reg(u8 reg, u8 *val);
static s32 ov7725_write_reg(u8 reg, u8 val);

static const struct i2c_device_id ov7725_id[] = {
       {"ov7725", 0},
       {},
};

MODULE_DEVICE_TABLE(i2c, ov7725_id);

static struct i2c_driver ov7725_i2c_driver = {
       .driver = {
                 .owner = THIS_MODULE,
                 .name  = "ov7725",
                 },
       .probe  = ov7725_probe,
       .remove = ov7725_remove,
       .id_table = ov7725_id,
};

static inline void ov7725_power_down(int enable)
{
       if (gpio_is_valid(pwn_gpio)) {
               gpio_set_value(pwn_gpio, enable);
               msleep(2);
       }+static inline void ov7725_reset(void)
{
       if (gpio_is_valid(rst_gpio) &&  gpio_is_valid(pwn_gpio)) 
       {
               /* camera reset */
               gpio_set_value(rst_gpio, 1);

               /* camera power down */
               gpio_set_value(pwn_gpio, 1);
               msleep(5);
               gpio_set_value(pwn_gpio, 0);
               msleep(5);
               gpio_set_value(rst_gpio, 0);
               msleep(1);
               gpio_set_value(rst_gpio, 1);
               msleep(5);
               gpio_set_value(pwn_gpio, 1);
       }
}

static s32 ov7725_write_reg(u8 reg, u8 val)
{
       u8 au8Buf[2] = {0};

       au8Buf[0] = reg & 0xff;
       au8Buf[1] = val;

       if (i2c_master_send(ov7725_data.i2c_client, au8Buf, 2) < 0) {
               pr_err("%s:write reg error:reg=%x,val=%x\n",
                       __func__, reg, val);
               return -1;
       }

       return 0;
}

static s32 ov7725_read_reg(u8 reg, u8 *val)
{
       u8 au8RegBuf[1] = {0};
       u8 u8RdVal = 0;

       au8RegBuf[0] = reg & 0xff;

       if (1 != i2c_master_send(ov7725_data.i2c_client, au8RegBuf, 1)) {
               pr_err("%s:write reg error:reg=%x\n",
                               __func__, reg);
               return -1;
       }

       if (1 != i2c_master_recv(ov7725_data.i2c_client, &u8RdVal, 1)) {
               pr_err("%s:read reg error:reg=%x,val=%x\n",
                               __func__, reg, u8RdVal);
               return -1;
       }

       *val = u8RdVal;

       return u8RdVal;
}

static int ov7725_init_mode(enum ov7725_frame_rate frame_rate,
                                       enum ov7725_mode mode)
{
        struct reg_value *pModeSetting = NULL;
        s32 iModeSettingArySize = 0;
        s32 i = 0;
        register u32 Delay_ms = 0;
        register u8 RegAddr, Mask, Val = 0;
        u8 RegVal = 0;
        int retval = 0;

        if (mode > ov7725_mode_MAX || mode < ov7725_mode_MIN) {
                pr_err("Unsupported ov7725 mode detected!\n");
                return -1;
        }

        pModeSetting = ov7725_mode_info_data[frame_rate][mode].init_data_ptr;
        iModeSettingArySize =
                ov7725_mode_info_data[frame_rate][mode].init_data_size;

        ov7725_data.pix.width = ov7725_mode_info_data[frame_rate][mode].width;
        ov7725_data.pix.height = ov7725_mode_info_data[frame_rate][mode].height;

        if (ov7725_data.pix.width == 0 || ov7725_data.pix.height == 0 ||
            pModeSetting == NULL || iModeSettingArySize == 0)
                return -EINVAL;

        for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
                Delay_ms = pModeSetting->u8Delay_ms;
                RegAddr = pModeSetting->u8RegAddr;
                Val = pModeSetting->u8Val;
                Mask = pModeSetting->u8Mask;

                if (Mask) {
                        retval = ov7725_read_reg(RegAddr, &RegVal);
                        if (retval < 0)
                                goto err;

                        RegVal &= ~(u8)Mask;
                        Val &= Mask;
                        Val |= RegVal;
                }

                retval = ov7725_write_reg(RegAddr, Val);
                if (retval < 0)
                        goto err;

                if (Delay_ms)
                        msleep(Delay_ms);
        }
err:
        return retval;

}
/* --------------- IOCTL functions from v4l2_int_ioctl_desc --------------- */

static int ioctl_g_ifparm(struct v4l2_int_device *s, struct v4l2_ifparm *p)
{
       if (s == NULL) {
               pr_err("   ERROR!! no slave device set!\n");
               return -1;
       }

       memset(p, 0, sizeof(*p));
       p->u.bt656.clock_curr = ov7725_data.mclk;
       pr_debug("   clock_curr=mclk=%d\n", ov7725_data.mclk);
       p->if_type = V4L2_IF_TYPE_BT656;
       p->u.bt656.mode = V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT;
       p->u.bt656.clock_min = OV7725_XCLK_MIN;
       p->u.bt656.clock_max = OV7725_XCLK_MAX;
       p->u.bt656.bt_sync_correct = 1;  /* Indicate external vsync */

       return 0;
}

/*!
 * ioctl_s_power - V4L2 sensor interface handler for VIDIOC_S_POWER ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: indicates power mode (on or off)
 *
 * Turns the power on or off, depending on the value of on and returns the
 * appropriate error code.
 */
static int ioctl_s_power(struct v4l2_int_device *s, int on)
{
       struct sensor_data *sensor = s->priv;

       if (on && !sensor->on) {
               /* Make sure power on */
               ov7725_power_down(0);
       } else if (!on && sensor->on) {

               ov7725_power_down(1);
       }

       sensor->on = on;

       return 0;
}

/*!
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
       struct sensor_data *sensor = s->priv;
       struct v4l2_captureparm *cparm = &a->parm.capture;
       int ret = 0;

       switch (a->type) {
       /* This is the only case currently handled. */
       case V4L2_BUF_TYPE_VIDEO_CAPTURE:
               memset(a, 0, sizeof(*a));
               a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
               cparm->capability = sensor->streamcap.capability;
               cparm->timeperframe = sensor->streamcap.timeperframe;
               cparm->capturemode = sensor->streamcap.capturemode;
               ret = 0;
               break;

       /* These are all the possible cases. */
       case V4L2_BUF_TYPE_VIDEO_OUTPUT:
       case V4L2_BUF_TYPE_VIDEO_OVERLAY:
       case V4L2_BUF_TYPE_VBI_CAPTURE:
       case V4L2_BUF_TYPE_VBI_OUTPUT:
       case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
       case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
               ret = -EINVAL;
               break;

       default:
               pr_debug("   type is unknown - %d\n", a->type);
               ret = -EINVAL;
               break;
       }

       return ret;
}
/*!
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
       struct sensor_data *sensor = s->priv;
       struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
       u32 tgt_fps;    /* target frames per secound */
       enum ov7725_frame_rate frame_rate;
       int ret = 0;

       /* Make sure power on */
       ov7725_power_down(0);

       switch (a->type) {
       /* This is the only case currently handled. */
       case V4L2_BUF_TYPE_VIDEO_CAPTURE:
               /* Check that the new frame rate is allowed. */
               if ((timeperframe->numerator == 0) ||
                   (timeperframe->denominator == 0)) {
                       timeperframe->denominator = DEFAULT_FPS;
                       timeperframe->numerator = 1;
               }

               tgt_fps = timeperframe->denominator /
                         timeperframe->numerator;

               if (tgt_fps != DEFAULT_FPS) {
                       timeperframe->denominator = MAX_FPS;
                       timeperframe->numerator = 1;
               }

               /* Actual frame rate we use */
               tgt_fps = timeperframe->denominator /
                         timeperframe->numerator;

               if (tgt_fps == 30)
                       frame_rate = ov7725_30_fps;
               else {
                       pr_err(" The camera driver supports only 30 fps!\n");
                       return -EINVAL;
               }

               ret = ov7725_init_mode(frame_rate,
                               a->parm.capture.capturemode);
               if (ret < 0)
                       return ret;

               sensor->streamcap.timeperframe = *timeperframe;
               sensor->streamcap.capturemode = a->parm.capture.capturemode;

               break;

       /* These are all the possible cases. */
       case V4L2_BUF_TYPE_VIDEO_OUTPUT:
       case V4L2_BUF_TYPE_VIDEO_OVERLAY:
       case V4L2_BUF_TYPE_VBI_CAPTURE:
       case V4L2_BUF_TYPE_VBI_OUTPUT:
       case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
       case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
               pr_debug("   type is not " \
                       "V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",
                       a->type);
               ret = -EINVAL;
               break;

       default:
               pr_debug("   type is unknown - %d\n", a->type);
               ret = -EINVAL;
               break;
       }

       return ret;
}

/*!
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
       struct sensor_data *sensor = s->priv;

       f->fmt.pix = sensor->pix;

       return 0;
}

/*!
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the video_control[] array.  Otherwise, returns -EINVAL
 * if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
       int ret = 0;

       switch (vc->id) {
       case V4L2_CID_BRIGHTNESS:
               vc->value = ov7725_data.brightness;
               break;
       case V4L2_CID_HUE:
               vc->value = ov7725_data.hue;
               break;
       case V4L2_CID_CONTRAST:
               vc->value = ov7725_data.contrast;
               break;
       case V4L2_CID_SATURATION:
               vc->value = ov7725_data.saturation;
               break;
       case V4L2_CID_RED_BALANCE:
               vc->value = ov7725_data.red;
               break;
       case V4L2_CID_BLUE_BALANCE:
               vc->value = ov7725_data.blue;
               break;
       case V4L2_CID_EXPOSURE:
               vc->value = ov7725_data.ae_mode;
               break;
       default:
               ret = -EINVAL;
       }

       return ret;
}

/*!
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the video_control[] array).  Otherwise,
 * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
       int retval = 0;

       pr_debug("In ov7725:ioctl_s_ctrl %d\n",
                vc->id);

       switch (vc->id) {
       case V4L2_CID_BRIGHTNESS:
               break;
       case V4L2_CID_CONTRAST:
               break;
       case V4L2_CID_SATURATION:
               break;
       case V4L2_CID_HUE:
               break;
       case V4L2_CID_AUTO_WHITE_BALANCE:
               break;
       case V4L2_CID_DO_WHITE_BALANCE:
               break;
       case V4L2_CID_RED_BALANCE:
               break;
       case V4L2_CID_BLUE_BALANCE:
               break;
       case V4L2_CID_GAMMA:
               break;
       case V4L2_CID_EXPOSURE:
               break;
       case V4L2_CID_AUTOGAIN:
               break;
       case V4L2_CID_GAIN:
       case V4L2_CID_HFLIP:
               break;
       case V4L2_CID_VFLIP:
               break;
       default:
               retval = -EPERM;
               break;
       }

       return retval;
}

/*!
 * ioctl_enum_framesizes - V4L2 sensor interface handler for
 *                        VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
                                struct v4l2_frmsizeenum *fsize)
{
       if (fsize->index > ov7725_mode_MAX)
               return -EINVAL;

       fsize->pixel_format = ov7725_data.pix.pixelformat;
       fsize->discrete.width = ov7725_mode_info_data[0][fsize->index].width;
       fsize->discrete.height = ov7725_mode_info_data[0][fsize->index].height;
       return 0;
}

/*!
 * ioctl_g_chip_ident - V4L2 sensor interface handler for
 *                     VIDIOC_DBG_G_CHIP_IDENT ioctl
 * @s: pointer to standard V4L2 device structure
 * @id: pointer to int
 *
 * Return 0.
 */
static int ioctl_g_chip_ident(struct v4l2_int_device *s, int *id)
{
       ((struct v4l2_dbg_chip_ident *)id)->match.type =
                                       V4L2_CHIP_MATCH_I2C_DRIVER;
       strcpy(((struct v4l2_dbg_chip_ident *)id)->match.name, "ov7725_camera");

       return 0;
}

/*!
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 */
static int ioctl_init(struct v4l2_int_device *s)
{

       return 0;
}

/*!
 * ioctl_enum_fmt_cap - V4L2 sensor interface handler for VIDIOC_ENUM_FMT
 * @s: pointer to standard V4L2 device structure
 * @fmt: pointer to standard V4L2 fmt description structure
 *
 * Return 0.
 */
static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
                             struct v4l2_fmtdesc *fmt)
{
       if (fmt->index > ov7725_mode_MAX)
               return -EINVAL;

       fmt->pixelformat = ov7725_data.pix.pixelformat;

       return 0;
}

/*!
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
       struct sensor_data *sensor = s->priv;
       u32 tgt_xclk;   /* target xclk */
       u32 tgt_fps;    /* target frames per secound */
       enum ov7725_frame_rate frame_rate;
       int ret;

       ov7725_data.on = true;

       /* mclk */
       tgt_xclk = ov7725_data.mclk;
       tgt_xclk = min(tgt_xclk, (u32)OV7725_XCLK_MAX);
       tgt_xclk = max(tgt_xclk, (u32)OV7725_XCLK_MIN);
       ov7725_data.mclk = tgt_xclk;

       pr_debug("   Setting mclk to %d MHz\n", tgt_xclk / 1000000);
       clk_set_rate(ov7725_data.sensor_clk, ov7725_data.mclk);

       /* Default camera frame rate is set in probe */
       tgt_fps = sensor->streamcap.timeperframe.denominator /
                 sensor->streamcap.timeperframe.numerator;

       if (tgt_fps == 30)
               frame_rate = ov7725_30_fps;
       else
               return -EINVAL; /* Only support 30fps now. */

       ret = ov7725_init_mode(frame_rate,
                                sensor->streamcap.capturemode);
       return ret;
}

/*!
 * ioctl_dev_exit - V4L2 sensor interface handler for vidioc_int_dev_exit_num
 * @s: pointer to standard V4L2 device structure
 *
 * Delinitialise the device when slave detaches to the master.
 */
static int ioctl_dev_exit(struct v4l2_int_device *s)
{
       return 0;
}

/*!
 * This structure defines all the ioctls for this module and links them to the
 * enumeration.
 */
static struct v4l2_int_ioctl_desc ov7725_ioctl_desc[] = {
       { vidioc_int_dev_init_num,
         (v4l2_int_ioctl_func *)ioctl_dev_init },
       { vidioc_int_dev_exit_num,
         ioctl_dev_exit},
       { vidioc_int_s_power_num,
         (v4l2_int_ioctl_func *)ioctl_s_power },
       { vidioc_int_g_ifparm_num,
         (v4l2_int_ioctl_func *)ioctl_g_ifparm },
       { vidioc_int_init_num,
         (v4l2_int_ioctl_func *)ioctl_init },
       { vidioc_int_enum_fmt_cap_num,
         (v4l2_int_ioctl_func *)ioctl_enum_fmt_cap },
       { vidioc_int_g_fmt_cap_num,
         (v4l2_int_ioctl_func *)ioctl_g_fmt_cap },
       { vidioc_int_g_parm_num,
         (v4l2_int_ioctl_func *)ioctl_g_parm },
       { vidioc_int_s_parm_num,
         (v4l2_int_ioctl_func *)ioctl_s_parm },
       { vidioc_int_g_ctrl_num,
         (v4l2_int_ioctl_func *)ioctl_g_ctrl },
       { vidioc_int_s_ctrl_num,
         (v4l2_int_ioctl_func *)ioctl_s_ctrl },
       { vidioc_int_enum_framesizes_num,
         (v4l2_int_ioctl_func *)ioctl_enum_framesizes },
       { vidioc_int_g_chip_ident_num,
         (v4l2_int_ioctl_func *)ioctl_g_chip_ident },
};

static struct v4l2_int_slave ov7725_slave = {
       .ioctls = ov7725_ioctl_desc,
       .num_ioctls = ARRAY_SIZE(ov7725_ioctl_desc),
};
static struct v4l2_int_device ov7725_int_device = {
       .module = THIS_MODULE,
       .name = "ov7725",
       .type = v4l2_int_type_slave,
       .u = {
               .slave = &ov7725_slave,
       },
};

/*!
 * ov7725 I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int ov7725_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
       struct pinctrl *pinctrl;
       struct device *dev = &client->dev;
       int retval;
       u8 chip_id_high, chip_id_low;

       /* ov7725 pinctrl */
       pinctrl = devm_pinctrl_get_select_default(dev);
       if (IS_ERR(pinctrl)) {
               dev_err(dev, "setup pinctrl failed\n");
               return PTR_ERR(pinctrl);
       }

       /* request power down pin */
       pwn_gpio = of_get_named_gpio(dev->of_node, "pwn-gpios", 0);
       if (gpio_is_valid(pwn_gpio)) {
               retval = devm_gpio_request_one(dev, pwn_gpio, GPIOF_OUT_INIT_HIGH,
                               "ov7725_pwdn");
               if (retval < 0)
                       return retval;
       }

       /* request reset pin */
       rst_gpio = of_get_named_gpio(dev->of_node, "rst-gpios", 0);
       if (gpio_is_valid(rst_gpio)) {
               retval = devm_gpio_request_one(dev, rst_gpio, GPIOF_OUT_INIT_HIGH,
                               "ov7725_reset");
               if (retval < 0)
                       return retval;
       }

       /* Set initial values for the sensor struct. */
       memset(&ov7725_data, 0, sizeof(ov7725_data));
       ov7725_data.sensor_clk = devm_clk_get(dev, "csi_mclk");
       if (IS_ERR(ov7725_data.sensor_clk)) {
               dev_err(dev, "get mclk failed\n");
               return PTR_ERR(ov7725_data.sensor_clk);
       }

       retval = of_property_read_u32(dev->of_node, "mclk",
                                       &ov7725_data.mclk);
       if (retval) {
               dev_err(dev, "mclk frequency is invalid\n");
               return retval;
       }

       retval = of_property_read_u32(dev->of_node, "mclk_source",
                                       (u32 *) &(ov7725_data.mclk_source));
       if (retval) {
               dev_err(dev, "mclk_source invalid\n");
               return retval;
       }

       retval = of_property_read_u32(dev->of_node, "csi_id",
                                       &(ov7725_data.csi));
       if (retval) {
               dev_err(dev, "csi_id invalid\n");
               return retval;
       }

       clk_prepare_enable(ov7725_data.sensor_clk);

       ov7725_data.io_init = ov7725_reset;
       ov7725_data.i2c_client = client;
       ov7725_data.pix.pixelformat = V4L2_PIX_FMT_YUYV;
       ov7725_data.pix.width = 640;
       ov7725_data.pix.height = 480;
       ov7725_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
                                          V4L2_CAP_TIMEPERFRAME;
       ov7725_data.streamcap.capturemode = 0;
       ov7725_data.streamcap.timeperframe.denominator = DEFAULT_FPS;
       ov7725_data.streamcap.timeperframe.numerator = 1;
       ov7725_reset();

       ov7725_power_down(0);

       retval = ov7725_read_reg(OV7725_CHIP_ID_HIGH_BYTE, &chip_id_high);
       if (retval < 0 || chip_id_high != 0x77) {
               clk_disable_unprepare(ov7725_data.sensor_clk);
               pr_warning("camera ov7725 is not found\n");
               return -ENODEV;
       }
       retval = ov7725_read_reg(OV7725_CHIP_ID_LOW_BYTE, &chip_id_low);
       if (retval < 0 || chip_id_low != 0x21) {
               clk_disable_unprepare(ov7725_data.sensor_clk);
               pr_warning("camera ov7725 is not found\n");
               return -ENODEV;
       }

       ov7725_power_down(1);

       clk_disable_unprepare(ov7725_data.sensor_clk);

       ov7725_int_device.priv = &ov7725_data;
       retval = v4l2_int_device_register(&ov7725_int_device);

       pr_info("camera ov7725 is found\n");
       return retval;
}

/*!
 * ov7725 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int ov7725_remove(struct i2c_client *client)
{
       v4l2_int_device_unregister(&ov7725_int_device);

       return 0;
}

module_i2c_driver(ov7725_i2c_driver);

MODULE_AUTHOR("iWave Systems Technologies Pvt. Ltd.");
MODULE_DESCRIPTION("OV7725 Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");



