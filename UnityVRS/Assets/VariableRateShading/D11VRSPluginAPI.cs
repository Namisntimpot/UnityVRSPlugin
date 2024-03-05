using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;

public class D11VRSPluginAPI
{
    const string PLUGIN_NAME = "D11VRSNativePlugin";

    [DllImport(PLUGIN_NAME)]
    public extern static IntPtr GetRenderEventFunc();

    [DllImport(PLUGIN_NAME)]
    public extern static IntPtr GetRenderEventAndDataFunc();

    [DllImport (PLUGIN_NAME)]
    public extern static uint GetVRSTileSize();

    public static void Log(string log)
    {
        string s = PLUGIN_NAME + ": " + log;
        Debug.Log(s);
    }

    public enum VRSPluginEvent
    {
        UpdateVRSImage,
        EnableVRSImage,
        DisableVRSImage,
    };

    public enum VRSPluginShadingRate
    {
        X1_PER_PIXEL,
        X1_PER_2X1_PIXELS,
        X1_PER_1X2_PIXELS,
        X1_PER_2X2_PIXELS,
        X1_PER_4X2_PIXELS,
        X1_PER_2X4_PIXELS,
        X1_PER_4X4_PIXELS,

        X2_PER_PIXEL,
        X4_PER_PIXEL,
        X8_PER_PIXEL,
        X16_PER_PIXEL,
        X0_CULL,
    };
}

