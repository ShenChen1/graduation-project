#ifndef VIDEO_H_
#define VIDEO_H_

#include <linux/videodev2.h>
#include <stdio.h>

#define COUNT 1
typedef struct _video_img//一帧图像缓冲区
{
	unsigned int length;
	void *start;
}VIDEO_IMG_S;

typedef struct _video_info//摄像头
{
	int videofd;//videofd
	char videopath[32];
	struct v4l2_buffer videobuf;////驱动中的一帧图像缓存，对应命令VIDIOC_QUERYBUF
	VIDEO_IMG_S videoframe[COUNT];
}VIDEO_INFO_S;


#define VIDEO_MAX   	8
#define VIDEO_WIDTH  	320
#define VIDEO_HEIGHT 	240
#define VIDEO_FORMAT 	V4L2_PIX_FMT_RGB32

//打开设备
int VIDEO_Open(int i);
// 查询设备信息
int VIDEO_Queryinfo(int i);
// 设定参数值
int VIDEO_Setparam(int i);
// 打印设备信息
int VIDEO_Scansupport(int i);
// 请求分配空间
int VIDEO_Requestcnt(int i);
// 开始视频采集
int VIDEO_Streamon(int i);
// 关闭视频采集
int VIDEO_Streamoff(int i);
// 获取缓存流数据
int VIDEO_GetStream(int i);
// 释放缓存流
int VIDEO_ReleaseStream(int i);
// 解除映射
int VIDEO_Close(int i);
// 采集一帧图像保存进文件中
int VIDEO_Saveimagetofile(int i, int j, FILE *fp);

#endif /* VIDEO_H_ */
