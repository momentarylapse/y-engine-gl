<Layout>
	bindings = [[buffer,sampler,sampler]]
	pushsize = 20
	input = [vec3,vec3,vec2]
	topology = triangles
	version = 420
</Layout>
<VertexShader>
#extension GL_ARB_separate_shader_objects : enable

struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};
#ifdef vulkan
layout(push_constant) uniform ParameterData {
	Matrix matrix;
};
#else
uniform Matrix matrix;
#endif

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

//layout(location = 0) out vec4 out_pos;
layout(location = 0) out vec2 out_uv;

void main() {
	gl_Position = matrix.project * vec4(in_position, 1.0);
	out_uv = in_uv;
}
</VertexShader>
<FragmentShader>
#extension GL_ARB_separate_shader_objects : enable

/*struct Matrix {
	mat4 model;
	mat4 view;
	mat4 project;
};*/

#ifdef vulkan


layout(push_constant) uniform ParameterData {
	//Matrix matrix;
	mat4 model;
	mat4 view;
	mat4 project;
	float exposure;
	float bloom_factor;
	float gamma;
	float scale_x;
	float scale_y;
};

#else

//uniform Matrix matrix;
uniform float exposure = 1.0;
uniform float bloom_factor = 0.2;
uniform float gamma = 2.2;
uniform float scale_x = 1.0;
uniform float scale_y = 1.0;

#endif

layout(binding = 1) uniform sampler2D tex0;
layout(binding = 2) uniform sampler2D tex1;

//layout(location = 0) in vec4 outPos;
layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


float brightness(vec3 c) {
	return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// TODO: smoother...
vec3 tone_map(vec3 c) {
	float b = brightness(c);
	if (b < 0.8)
		return c;
	return mix(c / b * 0.8, vec3(1,1,1), 1-exp(0.8-b));
	//return vec3(1.0) - exp(-c * exposure*0.5);
	//return pow(c * exposure, vec3(1,1,1)*0.2);
}

void main() {
	// hmmm, texture() is faster than texelFetch()...???
	vec2 uv = in_uv * vec2(scale_x, scale_y);
	uv.y += 1 - scale_y;
	out_color.rgb = textureLod(tex0, uv, 0).rgb;
	
	vec3 bloom = textureLod(tex1, uv, 0).rgb;
	out_color.rgb += bloom * bloom_factor;
	out_color.rgb = tone_map(out_color.rgb);
	
	out_color.rgb = pow(out_color.rgb, vec3(1.0 / gamma));
//	out_color.rgb = vec3(in_uv,0);
	out_color.a = 1;
}
</FragmentShader>
