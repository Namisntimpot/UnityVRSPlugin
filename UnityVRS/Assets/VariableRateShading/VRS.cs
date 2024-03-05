using System.Collections;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.UI;
using UnityEngine.Events;
using UnityEngine.Experimental.Rendering;

[RequireComponent(typeof(Camera))]
public class VRS : MonoBehaviour
{
    public UnityEvent<CommandBuffer, RenderTexture, Camera> UpdateRatemapEvent;
    public CameraEvent enableVRSStage = CameraEvent.BeforeForwardOpaque;
    public CameraEvent disableVRSStage = CameraEvent.AfterForwardAlpha;

    CommandBuffer m_cmd_enable = null;
    CommandBuffer m_cmd_disable = null;

    RenderTexture m_ratemap = null;
    RenderTextureDescriptor m_ratemap_desc;

    bool populateRatemap = false;

    uint m_Tilesize = 0;

    private void OnEnable()
    {
        m_Tilesize = D11VRSPluginAPI.GetVRSTileSize();
        if (m_Tilesize > 0)
        {
            D11VRSPluginAPI.Log("Variable Rate Shading supported. Tile size: " + m_Tilesize);
        }
        else
        {
            D11VRSPluginAPI.Log("Variable Rate Shading unsupported.");
        }

        m_ratemap_desc = new RenderTextureDescriptor()
        {
            dimension = TextureDimension.Tex2D,
            colorFormat = RenderTextureFormat.R8,
            graphicsFormat = GraphicsFormat.R8_UInt,
            enableRandomWrite = true,
            useMipMap = false,
            depthBufferBits = 0,
            volumeDepth = 1,

            bindMS = false,
            msaaSamples = 1,
        };

        m_cmd_enable = new CommandBuffer() { name = "Enable VRS" };
        m_cmd_disable = new CommandBuffer() { name="Disable VRS"};

        Camera.onPreRender += OnPreRenderCallback;
        Camera.onPostRender += OnPostRenderCallback;
    }

    // 虽然Update也在Render之前调用，但在编辑器中Update和OnPostRender不是“成对”的。
    // 场景没有变化的时候就不会渲染，也就没有OnPostRender，但是会不断Update，所以play后，scene窗口下会反复Update而不OnPostRender
    private void OnPreRenderCallback(Camera cam)
    {
        if(UpdateRatemapEvent.GetPersistentEventCount() > 0 && 
            UpdateRatemapEvent.GetPersistentMethodName(0) != "" && cam == GetComponent<Camera>())
        {
            populateRatemap = true;

            int h = cam.pixelHeight, w = cam.pixelWidth;
            m_ratemap_desc.height = h / (int)m_Tilesize;
            m_ratemap_desc.width = w / (int)m_Tilesize;

            m_ratemap = new RenderTexture(m_ratemap_desc) { depth = 0};
            m_ratemap.Create();

            UpdateRatemapEvent.Invoke(m_cmd_enable, m_ratemap, cam);

            m_cmd_enable.IssuePluginEventAndData(
                D11VRSPluginAPI.GetRenderEventAndDataFunc(),
                (int)D11VRSPluginAPI.VRSPluginEvent.UpdateVRSImage,
                m_ratemap.GetNativeTexturePtr()
            );
            m_cmd_enable.IssuePluginEvent(
                D11VRSPluginAPI.GetRenderEventFunc(),
                (int)D11VRSPluginAPI.VRSPluginEvent.EnableVRSImage
            );

            m_cmd_disable.IssuePluginEvent(
                D11VRSPluginAPI.GetRenderEventFunc(),
                (int)D11VRSPluginAPI.VRSPluginEvent.DisableVRSImage
            );

            cam.AddCommandBuffer(enableVRSStage, m_cmd_enable);
            cam.AddCommandBuffer(disableVRSStage, m_cmd_disable);
        }
    }

    private void OnPostRenderCallback(Camera cam)
    {
        if (populateRatemap && cam == GetComponent<Camera>())
        {
            populateRatemap = false;
            cam.RemoveCommandBuffer(enableVRSStage, m_cmd_enable);
            cam.RemoveCommandBuffer(disableVRSStage, m_cmd_disable);
            m_cmd_enable.Clear();
            m_cmd_disable.Clear();
            
            m_ratemap.Release();
        }
    }

    private void OnDisable()
    {
        m_cmd_enable.Release();
        m_cmd_disable.Release();
        m_cmd_disable = m_cmd_enable = null;
        Camera.onPreRender -= OnPreRenderCallback;
        Camera.onPostRender -= OnPostRenderCallback;

        D11VRSPluginAPI.Log("VRS Disabled");
    }
}
