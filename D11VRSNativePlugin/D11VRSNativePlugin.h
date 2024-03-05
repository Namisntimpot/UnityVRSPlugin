#pragma once

#include <d3d11.h>
#include "Nvidia/nvapi/developer/nvapi.h"

#include "Unity/IUnityInterface.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"
#include "Unity/IUnityLog.h"
//#include "Unity/IUnityRenderingExtensions.h" // NVAPI��VRS��������Drawcall�Ľ�����ˢ�£����Բ�������.

#include "Format.h"

using std::string;

//----------------�궨����ö��-----------------------
static const string PLUGIN_NAME = "UnityVRS";

// ʹ�� LOG("some infomation"); ��Unity����̨����ַ���.
#define LOG(x) \
	do{\
		string log = util::Format("{0}: {1}", PLUGIN_NAME, x); \
		s_UnityLog->Log(kUnityLogTypeLog, log.c_str(), __FILE__, __LINE__);  \
	}while(0)


// �ͷ�ָ��.
#define SAFE_RELEASE(a) if (a) { a->Release(); a = nullptr; }

// �¼�
typedef enum {
	UpdateVRSImage,
	EnableVRSImage,
	DisableVRSImage,
}VRSPluginEvent;

//----------------��ʹ�õĵı�����ȫ�Ǿ�̬�������ݲ�������----------
static IUnityInterfaces* s_UnityInterfaces = nullptr;
static IUnityGraphics* s_UnityGraphics = nullptr;
static IUnityLog* s_UnityLog = nullptr;

static ID3D11Device* s_Device = nullptr;
static bool s_VRSCapable = false, s_EnableVRS = false;

//static ID3D11Texture2D* m_ShadingRateImage = nullptr;
static ID3D11NvShadingRateResourceView* m_ShadingRateImageView = nullptr;

//----------------��¶��Unity�Ľӿ�------------------------------
extern "C" {
	// Unity Naive Plugin�������Load and Unload.
	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
	void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload();

	// ��Unity������Ⱦ�¼�
	UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc();
	UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventAndDataFunc();

	// VRS���
	UNITY_INTERFACE_EXPORT UINT UNITY_INTERFACE_API GetVRSTileSize(); // ��ȡVRS Tile��С(Ӣΰ��Turing GPU��16
	
	// ����ʹ�ô�Unity C#�д���Plugin�Ķ�̬������Texture2D��Ϊshading rate image���������� setTargetWH.
	//void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTargetWH(UINT64 width, UINT64 height); // ���ó����������ˢ��resource and view.
}

// -----------------����ʵ��--------------------------
// �¼��ַ�.
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);
static void UNITY_INTERFACE_API OnRenderEvent(int eventId);
static void UNITY_INTERFACE_API OnRenderEventAndData(int eventId, void* data);

static void NVInit(ID3D11Device* device);
static void BindVRSResources(ID3D11DeviceContext* context, ID3D11NvShadingRateResourceView* view);              // ��VRS resource and VRS look-up table
static void UpdateVRSResources(ID3D11DeviceContext* context, ID3D11Texture2D* tex);                    // ͨ�����Ƹ���shading rate image.

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