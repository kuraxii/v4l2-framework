#include <fcntl.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>

#define REQBUFS_COUNT 4
typedef struct cam_buf {
    void *start;
    int length;
} CAMBUF;

CAMBUF cambuf[REQBUFS_COUNT];
struct v4l2_requestbuffers reqbufs;

int main(int argc, char **argv)
{
    int ret;
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

    struct v4l2_buffer vbuf;
    struct v4l2_format format;
    struct v4l2_capability capability;

    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "camera->init: device can not support V4L2_CAP_VIDEO_CAPTURE\n");
        close(fd);
        return -1;
    }
    if (!(capability.capabilities & V4L2_CAP_STREAMING))
    {
        fprintf(stderr, "camera->init: device can not support V4L2_CAP_STREAMING\n");
        close(fd);
        return -1;
    }

    /*设置捕获的视频格式 MJPEG*/
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.width = 600;
    format.fmt.pix.height = 600;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    ret = ioctl(fd, VIDIOC_S_FMT, &format);
    if (!ret)
    {
        perror("ioctl VIDIOC_S_FMT");
        return -1;
    }

    // 获取当前设置的格式
    ret = ioctl(fd, VIDIOC_G_FMT, &format);
    if (!ret)
    {
        perror("ioctl VIDIOC_G_FMT");
        return -1;
    }

    /*向驱动申请缓存*/
    memset(&reqbufs, 0, sizeof(struct v4l2_requestbuffers));
    reqbufs.count = REQBUFS_COUNT; // 缓存区个数
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP; // 设置操作申请缓存的方式:映射 MMAP
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    if (!ret)
    {
        perror("ioctl VIDIOC_REQBUFS");
        return -1;
    }

    for (int i = 0; i < REQBUFS_COUNT; i++)
    {
        // 查询申请到的缓存
        memset(&vbuf, 0, sizeof(vbuf));
        vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vbuf.memory = V4L2_MEMORY_MMAP;
        vbuf.index = i;
        ret = ioctl(fd, VIDIOC_QUERYBUF, &vbuf);
        if (!ret)
        {
            perror("ioctl VIDIOC_QUERYBUF");
            close(fd);
            return -1;
        }

        cambuf[i].length = vbuf.length;
        cambuf[i].start = mmap(0, cambuf[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, vbuf.m.offset);
        if (cambuf[i].start == MAP_FAILED)
        {
            perror("mmap error");
            exit(0);
        }

        ret = ioctl(fd, VIDIOC_QBUF, &vbuf);
        if (!ret)
        {
            perror("ioctl VIDIOC_QBUF");
            return -1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /*ioctl控制摄像头开始采集*/
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret == -1)
    {
        perror("ioctl VIDIOC_STREAMON");
        return -1;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval timeout;
    timeout.tv_sec = 2;
    while (1)
    {
        ret = select(fd + 1, &fds, NULL, NULL, &timeout);
        if (ret < 0)
        {
            perror("camera->dbytesusedqbuf");
            return -1;
        }
        else if (ret == 0)
        {
            fprintf(stderr, "camera->dqbuf: dequeue buffer timeout\n");
            return -1;
        }
        else
        {
            ret = ioctl(fd, VIDIOC_DQBUF, &vbuf);
            if (!ret)
            {
                perror("ioctl VIDIOC_DQBUF");
            }
            printf("get data: %d", vbuf.length);
            ret = ioctl(fd, VIDIOC_QBUF, &vbuf);
            if (!ret)
            {
                perror("ioctl VIDIOC_DQBUF");
            }
        }
    }

    return 0;
}