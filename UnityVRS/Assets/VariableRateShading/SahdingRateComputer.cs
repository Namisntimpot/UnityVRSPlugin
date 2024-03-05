using System.Collections;
using System.Collections.Generic;
using System.Security.Cryptography;
using UnityEngine;
using UnityEngine.Rendering;

[CreateAssetMenu(fileName = "ShadingRateComputer", menuName = "VRS/Shading Rate Computer", order = 0)]
public class SahdingRateComputer : ScriptableObject
{
    public ComputeShader computeShader = null;
    public string kernelName = "CSMain";
    public string resultName = "Result";
    public int num_thread_x = 8;
    public int num_thread_y = 8;

    public void ComputeShadingRate(
        CommandBuffer cmdbuf, RenderTexture rateImage, Camera cam
    )
    {
        if (computeShader == null)
            return;
        int kernelIndex = computeShader.FindKernel(kernelName);
        computeShader.SetTexture(kernelIndex, resultName, rateImage);
        int rx = rateImage.width / num_thread_x, ry = rateImage.height / num_thread_y;
        int n_groupX = rateImage.width % num_thread_x != 0 ? rx + 1 : rx;
        int n_groupY = rateImage.height% num_thread_y != 0 ? ry + 1 : ry;
        cmdbuf.DispatchCompute(computeShader, kernelIndex, n_groupX, n_groupY, 1);
    }
}
