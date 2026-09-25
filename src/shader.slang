struct VertexInput {
    float2 position;
    float3 color;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 camera;
    float4x4 proj;
};

ConstantBuffer<UniformBuffer> uniform_buffer;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vert_main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(uniform_buffer.proj,mul(uniform_buffer.camera, mul(uniform_buffer.model, float4(input.position, 2.0, 1.0))));
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 frag_main(VertexOutput input_vert) : SV_Target
{
    return float4(input_vert.color, 1.0);
}
