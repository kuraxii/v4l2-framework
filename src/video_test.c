#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s </dev/videoX>, print format detail for video device\n", argv[0]);
        exit(0);
    }
    int fd;
    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        printf("can't open %s\n", argv[1]);
        exit(0);
    }

    int fmt_index = 0;
    int frame_index = 0;
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum fsenum;

    while (1)
    {
        /* 枚举格式 */
        fmtdesc.index = fmt_index;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (0 != ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc))
            break;
        frame_index = 0;
        while (1)
        {
            /* 枚举格式所支持的帧大小 */
            bzero(&fsenum, sizeof(fsenum));
            fsenum.pixel_format = fmtdesc.pixelformat;
            fsenum.index = frame_index;
            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsenum))
                break;
            printf("format %s, %d, framesize %d: %d x %d\n", fmtdesc.description, fmtdesc.pixelformat, frame_index,
                   fsenum.discrete.width, fsenum.discrete.height);
            ++frame_index;
        }

        ++fmt_index;
    }

    return 0;
}