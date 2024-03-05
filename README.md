# UnityVRSPlugin
## Overview
基于 **DX11与NVAPI** 的Unity Variable Rate Shading (VRS) 插件。VRS能够根据一张着色率图，为渲染目标的每个像素块 (在NVAPI的VRS中，是16X16的像素块)指定单独的着色率 (1x1, 1x2, 2x1, 2x2, 2x4, 4x2, 4x4).  

启用UnityVRSPlugin后，它将自动创建用于指定着色率的着色率图，使用者需要自定义向这个着色率图写入值的方法，然后Plugin将完成对底层图形API的调用，以这张着色率图为屏幕空间的着色率。

项目构成为：
+ D11VRSNativePlugin. 用于调用DX11与NVAPI图形接口的原生C++插件
+ UnityVRS，一个使用该插件的样例项目，其中UnityVRS/Asset/VariableRateShading下包含了Unity内调用VRS native plugin所需的.cs文件。

## 支持系统
+ Unity projects using DX11.
+ Nvidia Turing based GPU. (Nvidia 2000 Series or later)
+ Graphic Card Driver：430 and later
+ Only Unity Built-in pipeline is supported currently.

## 使用
1. 使用VS2022打开D11VRSNativePlugin项目，编译为.dll动态库。
2. 将该动态库复制到Unity项目的 Asset/Plugins 文件夹下。
3. 如UnityVRS这一示例场景中所示，将VRS.cs脚本挂载到主相机（或者你希望进行vrs渲染的相机）上。
4. 如UnityVRS/Asset/VariableRateShading/ShadingRateComputer.cs所示，通过ScriptableObject或者其它任何方式（比如使用monobehaviour，然后挂载到游戏物体上）定义计算着色率图像素值的方法。着色率图的每个像素代表一个16X16像素块的着色率。这个方法接收三个参数：`CommandBuffer, RenderTexture, Camera`。RenderTexture为需要求值的着色率图，是一张纹理，长宽为h/16, w/16. **你的任何渲染指令必须通过CommandBuffer记录**，不能直接执行。你也可以直接修改VRS.cs，让它提供更多你需要的参数。  
   着色率图的值与像素着色率的对应关系为：
   ```C#
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
   ```
   0值代表X1_PER_PIXEL, 1代表X1_PER_2X1_PIXELS, 以此类推。**着色率图RenderTexture**的格式为 **R8_UInt**，因此你必须在着色器中以 uint 声明此纹理、向其赋值。  
   UnityVRS/Asset/VariableRateShading/ShadingRateComputer.cs 提供了一个计算着色率图的样例，它通过一个计算着色器计算这张着色率图，这个计算着色器做的只是把着色率图中的每个像素赋值为6（X1_PER_4X4_PIXELS）。
5. 点击挂载 VRS.cs 的相机，将计算着色率图的方法拖入VRS.cs下的UpdateRatemapEvent 事件，VRS.cs将自动调用它以更新着色率图。
6. 打开Play模式，查看效果。**只在Play模式下进行VRS渲染**。

## 直接调用D11VRSNativePlugin中的API
可以不通过 vrs.cs，而是直接调用D11VRSNativePlugin中的API来实现VRS渲染。主要流程为：  
1. 查询系统是否支持VRS
2. 创建着色率图。着色率图的尺寸必须是渲染目标的1/16（长宽皆是），不整除可以直接向下取整。着色率图的TextureFormat应该是 **R8**，而GraphicsFormat应该是 **R8_UInt**。
3. 计算着色率图中每个像素的值。这个值与像素块着色率的对应关系见 `enum VRSPluginShadingRate`.
4. 让Plugin更新着色率图。
5. 在某个drawcall之前（比如渲染不透明物体之前），让Plugin开启VRS。
6. 在某个drawcall之后（比如渲染透明物体之后），让Plugin关闭VRS。（注意一定要关闭，否则Unity Editor的GUI可能也被VRS渲染！）

所涉及的API均在 UnityVRS/Asset/VariableRateShading/D11VRSPluginAPI.cs中链接。其中第 1, 4, 5, 6 部需要使用 D11VRSNativePlugin 的 API。

### 查询系统是否支持NVAPI的VRS
```C#
if(D11VRSPluginAPI.GetVRSTileSize()==0){
    Debug.Log("VRS unsupported");
}
```

### 更新着色率图
```C#
cmd.IssuePluginEventAndData(
    D11VRSPluginAPI.GetRenderEventAndDataFunc(),
    (int)D11VRSPluginAPI.VRSPluginEvent.UpdateVRSImage,
    m_ratemap.GetNativeTexturePtr()
);
```
其中，cmd表示一个 CommandBuffer.

### 开启VRS
```C#
cmd.IssuePluginEvent(
    D11VRSPluginAPI.GetRenderEventFunc(),
    (int)D11VRSPluginAPI.VRSPluginEvent.EnableVRSImage
);
```

### 关闭VRS
```C#
cmd.IssuePluginEvent(
    D11VRSPluginAPI.GetRenderEventFunc(),
    (int)D11VRSPluginAPI.VRSPluginEvent.DisableVRSImage
);
```