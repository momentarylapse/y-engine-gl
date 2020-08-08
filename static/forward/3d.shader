<Layout>
	bindings = [[buffer,dbuffer,buffer,sampler,sampler,sampler,sampler]]
	pushsize = 112
	input = [vec3,vec3,vec2]
	topology = triangles
</Layout>
<VertexShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable


uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

/*layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} mat;*/

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_pos_world;
layout(location = 1) out vec2 out_tex_coord;
layout(location = 2) out vec3 out_normal;

void main() {
	//float a = gl_InstanceID*0.5;
	//float b = gl_InstanceID*1.1;
	out_pos_world = mat_m * vec4(in_position, 1.0);// + vec4(cos(a)*cos(b),sin(a)*cos(b),sin(b),0) * 300 * pow(gl_InstanceID, 0.4);
	gl_Position = mat_p * mat_v * out_pos_world;
	out_normal = (mat_m * vec4(in_normal, 0.0)).xyz;
	out_tex_coord = in_tex_coord;
}
</VertexShader>
<FragmentShader>
#version 420
#extension GL_ARB_separate_shader_objects : enable


const float shininess = 20.0;
const float gloss = 0.2;


uniform mat4 mat_mvp;
uniform mat4 mat_p;
uniform mat4 mat_m;
uniform mat4 mat_v;

uniform vec4 eye_pos;
uniform vec4 emission_factor;

/*layout(push_constant) uniform PushConstants {
	mat4 mat_model;
	vec4 emission_factor;
	vec4 eye_pos;
	vec4 xxx;
	//Light light;
};*/

struct Light {
	mat4 proj;
	vec4 pos;
	vec4 dir;
	vec4 color;
	float radius, theta, harshness;
};
uniform int num_lights;

layout(binding = 1) uniform LightData {
	Light light[32];
};

layout(binding = 2) uniform Fog {
	vec4 color;
	float distance;
} fog;

layout(binding = 3) uniform sampler2D tex0;
layout(binding = 4) uniform sampler2D tex1;//_mat;
layout(binding = 5) uniform sampler2D tex2;//_glow;
layout(binding = 6) uniform sampler2D tex3;//sampler_shadow;
layout(binding = 7) uniform sampler2D tex4;//sampler_shadow2;

layout(location = 0) in vec4 in_pos_world;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec4 out_color;

float rand3d(vec3 p) {
	return fract(sin(dot(p ,vec3(12.9898,78.233,4213.1234))) * 43758.5453);
}


vec3 light_dir(Light l, vec3 p) {
	if (l.radius < 0)
		return l.dir.xyz;
	return normalize(p - l.pos.xyz);
}


float brightness(Light l, vec3 p, vec3 n) {
	// parallel
	if (l.radius < 0)
		return 1.0f;
	
	
	float d = length(p - l.pos.xyz);
	if (d > l.radius)
		return 0.0;
	float b = min(pow(1.0/d, 2), 1.0);
	
	// spherical
	if (l.theta < 0)
		return b;
	
	float t = acos(dot(l.dir.xyz, normalize(p - l.pos.xyz)));
	float tmax = l.theta;
	return b * (1 - smoothstep(tmax*0.8, tmax, t));
}

// blinn phong
float specular(Light l, vec3 p, vec3 n) {
	vec3 view_dir = normalize(p - eye_pos.xyz);
	vec3 half_dir = normalize(light_dir(l, p) + view_dir);

	float spec_angle = max(-dot(half_dir, n), 0.0);
	return pow(spec_angle, shininess);
}

float shadow_pcf_step(vec3 p, vec2 dd, ivec2 ts) {
	vec2 d = dd / ts * 0.8;
	vec2 tp = p.xy + d;
	float shadow_z = texture(tex4, p.xy + d).r + 0.004;
	if (tp.x > 0.38 && tp.y > 0.38 && tp.x < 0.62 && tp.y < 0.62)
		shadow_z = texture(tex3, (p.xy - vec2(0.5,0.5))*4 + vec2(0.5,0.5) + d).r + 0.001;
	if (p.z > shadow_z)
		return 1.0;
	return 0.0;
}

float shadow_pcf(vec3 p) {
	ivec2 ts = textureSize(tex3, 0);
	float value = 0;//shadow_pcf_step(p, vec2(0,0), ts);
	const float R = 1.8;
	const int N = 3;
	for (int i=0; i<N; i++) {
		float phi = rand3d(p + p*i) * 2 * 3.1415;
		float r = R * sqrt(fract(phi * 235.3545));
		value += shadow_pcf_step(p, r * vec2(cos(phi), sin(phi)), ts);
	}
	//value += shadow_pcf_step(p, vec2( 1.0, 1.1), ts);
	//value += shadow_pcf_step(p, vec2(-0.8, 0.9), ts);
	//value += shadow_pcf_step(p, vec2( 0.7,-0.8), ts);
	//value += shadow_pcf_step(p, vec2(-0.5,-1.1), ts);
	return value / N;//(N+1);
}

vec3 light_add(Light l, vec3 p, vec3 n, vec3 t) {
	float shadow = 1.0;
	
	if (l.radius < 0) {
		vec4 proj = l.proj * vec4(p,1);
		proj.xyz /= proj.w;
		proj.x = (proj.x +1)/2;
		proj.y = (proj.y +1)/2;
		proj.z = (proj.z +1)/2;
	
		if (proj.x > 0.01 && proj.x < 0.99 && proj.y > 0.01 && proj.y < 0.99 && proj.z < 1.0) {
			shadow = 1.0 - shadow_pcf(proj.xyz) * l.harshness;
		}
	}
	
	

	float b = brightness(l, p, n) * shadow;
	float lambert = max(-dot(n, light_dir(l, p)), 0);
	
	float bb = l.harshness * lambert + (1 - l.harshness);
	vec3 col = t * l.color.rgb * bb * b;
	
	// specular
	if (lambert > 0) {
		float sp = gloss * specular(l, p, n);
		col += sp * l.color.rgb * b;
	}
	return col;
}

void main() {
	vec3 n = normalize(in_normal);
	vec3 p = in_pos_world.xyz / in_pos_world.w;
	vec3 d = normalize(eye_pos.xyz - p);
	vec4 t = texture(tex0, in_tex_coord);
	//t = texture(sampler_shadow, in_tex_coord);
	//t.r = pow(t.r, 10);

	out_color = texture(tex2/*_glow*/, in_tex_coord) * emission_factor;
//	out_color = vec4(0,0,0,1);
	
///	float reflectivity = 1-((1-xxx.x) * (1-exp(-pow(dot(d, n),2) * 100)));
	float reflectivity = 0.0;



	

	for (int i=0; i<num_lights; i++)
		out_color.rgb.rgb += light_add(light[i], p, n, t.rgb).rgb;
	
/*	float distance = length(p - eye_pos.xyz);
	float f = exp(-distance / fog.distance);
	out_color.rgb = f * out_color.rgb + (1-f) * fog.color.rgb;
	
	*/
	
	out_color.a = t.a;
}

</FragmentShader>
