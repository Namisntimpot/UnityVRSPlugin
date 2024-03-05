#pragma once

#include <d3d11.h>
#include "Nvidia/nvapi/developer/nvapi.h"

#include "Unity/IUnityInterface.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"
#include "Unity/IUnityLog.h"
//#include "Unity/IUnityRenderingExtensions.h" // NVAPI的VRS不会随着Drawcall的结束而刷新，所以不用上它.

#include "Format.h"

using std::string;

//----------------宏定义与枚举-----------------------
static const string PLUGIN_NAME = "UnityVRS";

// 使用 LOG("some infomation"); 向Unity控制台输出字符串.
#define LOG(x) \
	do{\
		string log = util::Format("{0}: {1}", PLUGIN_NAME, x); \
		s_UnityLog->Log(kUnityLogTypeLog, log.c_str(), __FILE__, __LINE__);  \
	}while(0)


// 释放指针.
#define SAFE_RELEASE(a) if (a) { a->Release(); a = nullptr; }

// 事件
typedef enum {
	UpdateVRSImage,
	EnableVRSImage,
	DisableVRSImage,
}VRSPluginEvent;

//----------------所使用的的变量，全是静态变量，暂不做封类----------
static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphics* s_UnityGraphics = nullptr;
static IUnityLog* s_UnityLog = nullptr;

static ID3D11Device* s_Device = nullptr;
static bool s_VRSCapable = false, s_EnableVRS = false;

//static ID3D11Texture2D* m_ShadingRateImage = nullptr;
static ID3D11NvShadingRateResourceView* m_ShadingRateImageView = nullptr;

//----------------暴露给Unity的接口------------------------------
extern "C" {
	// Unity Naive Plugin所必须的Load and Unload.
	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload();

	// 从Unity发出渲染事件
	UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc();
	UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventAndDataFunc();

	// VRS相关
	UNITY_INTERFACE_EXPORT UINT UNITY_INTERFACE_API GetVRSTileSize(); // 获取VRS Tile大小(英伟达Turing GPU是16
	
	// 由于使用从Unity C#中传入Plugin的动态创建的Texture2D作为shading rate image，所以无需 setTargetWH.
	//void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTargetWH(UINT64 width, UINT64 height); // 设置长宽，这伴随着刷新resource and view.
}

// -----------------具体实现--------------------------
// 事件分发.
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);
static void UNITY_INTERFACE_API OnRenderEvent(int eventId);
static void UNITY_INTERFACE_API OnRenderEventAndData(int eventId, void* data);

static void NVInit(ID3D11Device* device);
static void BindVRSResources(ID3D11DeviceContext* context, ID3D11NvShadingRateResourceView* view);              // 绑定VRS resource and VRS look-up table
static void UpdateVRSResources(ID3D11DeviceContext* context, ID3D11Texture2D* tex);                    // 通过复制更新shading rate image.

static void EnableVRS(ID3D11DeviceContext* context, bool enable);

// VRS look-up table
static NV_PIXEL_SHADING_RATE s_ShadingRateTable[NV_MAX_PIXEL_SHADING_RATES] = {
	NV_PIXEL_X1_PER_RASTER_PIXEL,           //  Per-pixel shading
	NV_PIXEL_X1_PER_2X1_RASTER_PIXELS,      //  1 shading pass per  2 raster pixels
	NV_PIXEL_X1_PER_1X2_RASTER_PIXELS,      //  1 shading pass per  2 raster pixels
	NV_PIXEL_X1_PER_2X2_RASTER_PIXELS,      //  1 shading pass per  4 raster pixels
	NV_PIXEL_X1_PER_4X2_RASTER_PIXELS,      //  1 shading pass per  8 raster pixels
	NV_PIXEL_X1_PER_2X4_RASTER_PIXELS,      //  1 shading pass per  8 raster pixels
	NV_PIXEL_X1_PER_4X4_RASTER_PIXELS,       //  1 shading pass per 16 raster pixels

	NV_PIXEL_X2_PER_RASTER_PIXEL,          // 2 shading passes per 1 raster pixel
	NV_PIXEL_X4_PER_RASTER_PIXEL,           //  4 shading passes per 1 raster pixel
	NV_PIXEL_X8_PER_RASTER_PIXEL,           //  8 shading passes per 1 raster pixel
	NV_PIXEL_X16_PER_RASTER_PIXEL,
	NV_PIXEL_X0_CULL_RASTER_PIXELS,         // No shading, tiles are culled
};