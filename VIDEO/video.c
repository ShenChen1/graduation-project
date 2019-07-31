#include "video.h"
#include <stdio.h>              /* perror, printf, puts, fprintf, fputs */
#include <unistd.h>             /* read, write, close */
#include <fcntl.h>              /* open */
#include <sys/signal.h>
#include <sys/types.h>
#include <string.h>             /* bzero, memcpy */
#include <stdlib.h>
#include <limits.h>             /* CHAR_MAX */
#include <sys/mman.h>
#include <sys/ioctl.h>

static VIDEO_INFO_S g_video_info[VIDEO_MAX];

//打开设备
int VIDEO_Open(int i)
{
	//1).打开摄像头设备
	g_video_info[i].videofd = open(g_video_info[i].videopath, O_RDWR, 0);
	if(g_video_info[i].videofd < 0)
	{
		fprintf(stderr, "Open %s failed\n", g_video_info[i].videopath);
		return -1;
	}

	VIDEO_Queryinfo(i);// 查询设备信息
	VIDEO_Setparam(i);// 设定宽、高、存储类型
	VIDEO_Scansupport(i);// 打印设备信息
	VIDEO_Requestcnt(i);//请求缓存

	return 0;
}

// 查询设备信息
int VIDEO_Queryinfo(int i)
{
	struct v4l2_capability cap ;//视频设备的功能，对应命令VIDIOC_QUERYCAP

	//2).查询摄像头设备的基本信息及功能
	if(ioctl(g_video_info[i].videofd, VIDIOC_QUERYCAP, &cap) < 0)
	{
		printf("VIDIOC_QUERYCAP error\n");
		return -1;
	}
	printf("driver:           %s\n",cap.driver);
	printf("card:             %s\n",cap.card);
	printf("bus_info:         %s\n",cap.bus_info);
	printf("version:          %d\n",cap.version);
	printf("capabilities:     %#x\n",cap.capabilities);

	return 0;
}

// 设定参数值  宽、高、存储类型
int VIDEO_Setparam(int i)
{
	struct v4l2_format fmt;   //当前驱动的频捕获格式式，对应命令VIDIOC_G_FMT、VIDIOC_S_FMT
	//4).设置当前驱动的频捕获格式
	memset(&fmt, 0 ,sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = VIDEO_FORMAT;
	fmt.fmt.pix.height = VIDEO_HEIGHT;
	fmt.fmt.pix.width = VIDEO_WIDTH;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if(ioctl(g_video_info[i].videofd, VIDIOC_S_FMT, &fmt))
	{
		fprintf(stderr, "VIDIOC_S_FMT error\n");
		return -1;
	}

	return 0;
}

// 打印设备信息
int VIDEO_Scansupport(int i)
{
	struct v4l2_format fmt;   //当前驱动的频捕获格式式，对应命令VIDIOC_G_FMT、VIDIOC_S_FMT
	struct v4l2_fmtdesc stfmt; //当前视频支持的格式，对应命令VIDIOC_ENUM_FMT

	//3).列举摄像头所支持像素格式。
	memset(&stfmt, 0, sizeof(stfmt));
	stfmt.index = 0;
	stfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("device support:\n");
	while(ioctl(g_video_info[i].videofd, VIDIOC_ENUM_FMT, &stfmt) != -1)
	{
		printf("index %d:%s\n", stfmt.index, stfmt.description);
		stfmt.index++;
	}

	if(ioctl(g_video_info[i].videofd, VIDIOC_G_FMT, &fmt) < 0)
	{
		fprintf(stderr, "VIDIOC_G_FMT error\n");
		return -1;
	}

	printf("field:              %d\n",fmt.fmt.pix.field);
	printf("bytesperline:       %d\n",fmt.fmt.pix.bytesperline);
	printf("sizeimage:          %d\n",fmt.fmt.pix.sizeimage);
	printf("colorspace:         %d\n",fmt.fmt.pix.colorspace);
	printf("priv:               %d\n",fmt.fmt.pix.priv);

	return 0;
}

int VIDEO_Requestcnt(int i)
{
	struct v4l2_requestbuffers reqbuf;//申请帧缓存，对应命令VIDIOC_REQBUFS
	int j;

	//1).向驱动申请帧缓存
	reqbuf.count = COUNT;// 缓存数量，也就是说在缓存队列里保持多少张照片
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	if(ioctl(g_video_info[i].videofd, VIDIOC_REQBUFS, &reqbuf) == -1)
	{
		fprintf(stderr, "VIDEO_REQBUFS error \n");
		return -1;
	}

	//2).获取申请的每个缓存的信息，并mmap到用户空间
	for(j = 0; j < reqbuf.count; j++)
	{
		g_video_info[i].videobuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		g_video_info[i].videobuf.memory = V4L2_MEMORY_MMAP;
		g_video_info[i].videobuf.index = j;
		if(ioctl(g_video_info[i].videofd, VIDIOC_QUERYBUF, &g_video_info[i].videobuf) == -1)
		{
			fprintf(stderr, "VIDIOC_QUERYBUF error\n");
			return -1;
		}
		g_video_info[i].videoframe[j].length = g_video_info[i].videobuf.length;
		g_video_info[i].videoframe[j].start = mmap(NULL,
				g_video_info[i].videobuf.length,
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				g_video_info[i].videofd,
				g_video_info[i].videobuf.m.offset);
		if(g_video_info[i].videoframe[j].start == MAP_FAILED)
		{
			fprintf(stderr, "mmap error\n");
			return-1;
		}
		//投放一个空的视频缓冲区到视频缓冲区输入队列中
		if(ioctl(g_video_info[i].videofd, VIDIOC_QBUF, &g_video_info[i].videobuf) < 0)
		{
			fprintf(stderr, "VIDIOC_QBUF error\n");
			return -1;
		}
	}

	return 0;
}

// 开始视频采集
int VIDEO_Streamon(int i)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(g_video_info[i].videofd, VIDIOC_STREAMON, &type) < 0)
	{
		fprintf(stderr, "VIDIOC_STREAMON error \n");
		return -1;
	}
	return 0;
}

// 关闭视频采集
int VIDEO_Streamoff(int i)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(ioctl(g_video_info[i].videofd, VIDIOC_STREAMOFF, &type) < 0)
	{
		fprintf(stderr, "VIDIOC_STREAMOFF error\n");
		return -1;
	}
	return 0;
}

//取出视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区；
int VIDEO_GetStream(int i)
{
	if(ioctl(g_video_info[i].videofd, VIDIOC_DQBUF, &g_video_info[i].videobuf) < 0)
	{
		fprintf(stderr, "VIDIOC_DQBUF error\n");
		return -1;
	}
	printf("VIDEO_GetStream()  g_videobuf.index=%d\n",g_video_info[i].videobuf.index);
	return 0;
}

// 释放缓存流 重新入队列
int VIDEO_ReleaseStream(int i)
{
	if(ioctl(g_video_info[i].videofd,VIDIOC_QBUF,&g_video_info[i].videobuf)< 0)
	{
		fprintf(stderr, "VIDIOC_QBUF error\n");
		return -1;
	}
	printf("VIDEO_ReleaseStream()  g_videobuf.index=%d\n",g_video_info[i].videobuf.index);
	return 0;
}

// 解除映射
int VIDEO_Close(int i)
{
	int j;
	for(j = 0; j < COUNT; j++)
	{
		munmap(g_video_info[i].videoframe[j].start, g_video_info[i].videoframe[j].length);
	}

	close(g_video_info[i].videofd);
	return 0;
}

// 采集一帧图像保存进文件中
int VIDEO_Saveimagetofile(int i, int j, FILE *fp)
{
	int n;
	fseek(fp, SEEK_SET, 0);
	n = fwrite(g_video_info[i].videoframe[j].start, 1, g_video_info[i].videoframe[j].length, fp);
	fseek(fp, SEEK_SET, 0);

	return n;
}


int main()
{
	FILE *fp;
	int i;
	char name[8]={0};
	g_video_info[0].videopath = "/dev/video1";
	g_video_info[1].videopath = "/dev/video0";
	VIDEO_Open(0);
	VIDEO_Open(1);
	for(i = 0; i < 5; i++)
	{
		sprintf(name,"pic%d",i);
		fp=fopen(name,"w+");

		VIDEO_Streamon(0);
		VIDEO_GetStream(0);
		VIDEO_Streamoff(0);

		VIDEO_Saveimagetofile(0, 0, fp);
		VIDEO_ReleaseStream(0);

		fclose(fp);
		getchar();
	}

	VIDEO_Close(0);
	VIDEO_Close(1);
	return 0;
}



