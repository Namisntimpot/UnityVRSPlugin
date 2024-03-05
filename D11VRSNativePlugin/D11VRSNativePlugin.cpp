#include "pch.h"

#include "D11VRSNativePlugin.h"

//----------------暴露给Unity的接口------------------------------
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces * unityInterfaces) {
	// 只在这里面操作UnityXXX.
	s_UnityInterfaces = unityInterfaces;
	s_UnityGraphics = unityInterfaces->Get<IUnityGraphics>();
	s_UnityLog = unityInterfaces->Get<IUnityLog>();
	s_UnityGraphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

	OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);   // 刚加载的时候，不要错过第一次initialize.
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload() {
	SAFE_RELEASE(s_Device);
	s_UnityGraphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
	s_UnityGraphics = nullptr;
	s_UnityLog = nullptr;
	s_UnityInterfaces = nullptr;
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc() {
	return OnRenderEvent;
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventAndDataFunc() {
	return OnRenderEventAndData;
}

extern "C" UNITY_INTERFACE_EXPORT UINT UNITY_INTERFACE_API GetVRSTileSize() {
	if (s_VRSCapable)
		return NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH;    // Nvidia Turing GPU 中，VRS Tile width height 都是16.
	else
		return 0;
}

// --------------------功能与其它--------------------
static void NVInit(ID3D11Device* device) {
	NvAPI_Status NvStatus = NvAPI_Initialize();
	if (NvStatus != NvAPI_Status::NVAPI_OK) {
		LOG("NvAPI init fail");
		s_VRSCapable = false;
		return;
	}

	NvStatus = NvAPI_D3D_RegisterDevice(device);
	if (NvStatus != NvAPI_Status::NVAPI_OK) {
		LOG("NvAPI register device fail");
		s_VRSCapable = false;
		return;
	}

	NV_D3D1x_GRAPHICS_CAPS capabilities = {};
	NvStatus = NvAPI_D3D1x_GetGraphicsCapabilities(device, NV_D3D1x_GRAPHICS_CAPS_VER, &capabilities);
	if (NvStatus != NvAPI_Status::NVAPI_OK) {
		LOG("NvAPI check device capabilities fail");
		s_VRSCapable = false;
		return;
	}
	s_VRSCapable = capabilities.bVariablePixelRateShadingSupported;
	if (!s_VRSCapable) {
		LOG("Plugin loaded. VRS unsupported.");
	}
	else {
		LOG(util::Format("Plugin loaded. VRS supported. TileSize: {0}", NV_VARIABLE_PIXEL_SHADING_TILE_WIDTH));
	}
}

static void BindVRSResources(ID3D11DeviceContext *ctx, ID3D11NvShadingRateResourceView *view) {
	NvAPI_D3D11_RSSetShadingRateResourceView(ctx, view);
}

static void EnableVRS(ID3D11DeviceContext* ctx, bool enable) {
	NV_D3D11_VIEWPORTS_SHADING_RATE_DESC shadingRateDesc;
	NV_D3D11_VIEWPORT_SHADING_RATE_DESC viewportsShadingRateDesc[1];
	shadingRateDesc.version = NV_D3D11_VIEWPORTS_SHADING_RATE_DESC_VER;

	if (enable) {
		shadingRateDesc.numViewports = 1;
		shadingRateDesc.pViewports = viewportsShadingRateDesc;
		viewportsShadingRateDesc[0].enableVariablePixelShadingRate = true;
		for (int i = 0; i < NV_MAX_PIXEL_SHADING_RATES; ++i) {
			viewportsShadingRateDesc[0].shadingRateTable[i] = s_ShadingRateTable[i];
		}
	}
	else {
		shadingRateDesc.numViewports = 0;
		shadingRateDesc.pViewports = NULL;
	}

	NvAPI_D3D11_RSSetViewportsPixelShadingRates(ctx, &shadingRateDesc);
}

static void UpdateVRSResources(ID3D11DeviceContext* ctx, ID3D11Texture2D* tex) {
	if (tex != NULL) {
		NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Format = DXGI_FORMAT_R8_TYPELESS;
		viewDesc.version = NV_D3D11_SHADING_RATE_RESOURCE_VIEW_DESC_VER;
		viewDesc.ViewDimension = NV_SRRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;

		if (NvAPI_D3D11_CreateShadingRateResourceView(s_Device, tex, &viewDesc, &m_ShadingRateImageView) != NVAPI_OK) {
			LOG("Failed to create shading rate resource view from texture created in Unity by VRS plugin");
			m_ShadingRateImageView = NULL;
		}
	}
	else {
		m_ShadingRateImageView = NULL;
	}
}


// ------------------事件分发-----------------------
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
	switch (eventType)
	{
	case kUnityGfxDeviceEventInitialize:
		// 先获取device，然后初始化 nvapi.
		if (s_UnityInterfaces) {
			s_Device = s_UnityInterfaces->Get<IUnityGraphicsD3D11>()->GetDevice();
		}
		NVInit(s_Device);
		break;

	case kUnityGfxDeviceEventShutdown:
		NvAPI_Unload();
		break;
	default:
		break;
	}
}


static void UNITY_INTERFACE_API OnRenderEvent(int eventId) {
	if (!s_VRSCapable)
		return;
	ID3D11DeviceContext* ctx;
	s_Device->GetImmediateContext(&ctx);

	switch (eventId) {
	case EnableVRSImage:
		s_EnableVRS = true;
		if(m_ShadingRateImageView)
		{
			EnableVRS(ctx, true);
			BindVRSResources(ctx, m_ShadingRateImageView);
		}
		else {
			EnableVRS(ctx, false);
			BindVRSResources(ctx, nullptr);
		}
		break;
	case DisableVRSImage:
		s_EnableVRS = false;
		EnableVRS(ctx, false);
		BindVRSResources(ctx, nullptr);
		m_ShadingRateImageView = NULL;
		break;
	default:
		break;
	}
}

static void UNITY_INTERFACE_API OnRenderEventAndData(int eventId, void* data) {
	if (!s_VRSCapable)
		return;
	ID3D11DeviceContext* ctx;
	s_Device->GetImmediateContext(&ctx);

	ID3D11Texture2D* tex;
	
	switch (eventId) {
	case UpdateVRSImage:
		tex = reinterpret_cast<ID3D11Texture2D*>(data);
		UpdateVRSResources(ctx, tex);
		break;
	default:
		break;
	}
}