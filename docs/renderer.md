# Resources

## IGfxDescriptor

This is used in the back-ends as a base class. For example, in DX12, there is a class `D3D12ShaderResourceView` that is derived from `IGfxDescriptor`. This is used to create a shader resource view.

There are only 4 type of descriptors:

- Shader Resource View
- Unordered Access View
- Constant Buffer View
- Sampler

They all share the following properties:

- A name (for debugging)
- A handle (GetHandle())
- A heap index (GetHeapIndex())
- One or more specific descriptor structure



## IGfxResource

- Name
- GetHandle()
- GetDevice() (IGfxDevice*)
- IsTexture(), IsBuffer()


- IGfxBuffer
- IGfxCommandList
- IGfxDescriptor
- IGfxFence
- IGfxHeap
- IGfxPipelineState
- IGfxRaytracingBLAS
- IGfxRaytracingTLAS
- IGfxShader
- IGfxSwapChain
- IGfxTexture

