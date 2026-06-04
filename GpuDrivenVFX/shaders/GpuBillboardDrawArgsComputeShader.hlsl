// 현재 프레임의 active Particle 개수
StructuredBuffer<uint> aliveCount : register(t0);

// DrawIndexedInstancedIndirect가 사용할 argument buffer
RWByteAddressBuffer drawArgs : register(u0);

// DrawIndexedInstancedIndirect argument layout:
// Byte offset 00 : IndexCountPerInstance
// Byte offset 04 : InstanceCount
// Byte offset 08 : StartIndexLocation
// Byte offset 12 : BaseVertexLocation
// Byte offset 16 : StartInstanceLocation
[numthreads(1, 1, 1)]
void CS_Main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    // Billboard Quad는 인덱스 6개로 구성
    drawArgs.Store(0, 6);

    // Compute Shader에서 계산한 active Particle 개수를 instance count로 사용
    drawArgs.Store(4, aliveCount[0]);

    // Index Buffer 시작 위치
    drawArgs.Store(8, 0);

    // Base Vertex Location
    drawArgs.Store(12, 0);

    // Start Instance Location
    drawArgs.Store(16, 0);
}