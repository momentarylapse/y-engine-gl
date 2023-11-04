<Layout>
	version = 420
	name = vertex-instanced
</Layout>
<Module>



#ifdef vulkan
struct Matrices {
	mat4 model;
	mat4 view;
	mat4 project;
};

layout(binding = 0) uniform Parameters {
	Matrices matrix;
};
layout(binding = 7) uniform Multi {
	mat4 multi[1024];
};

#else
struct Matrix { mat4 model, view, project; };
/*layout(binding = 0)*/ uniform Matrix matrix;

uniform Multi {
	mat4 multi[1024];
};

#endif

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_pos; // world space
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;
layout(location = 3) out vec4 out_color; // optional




void main() {
#ifdef vulkan
	mat4 m = multi[gl_InstanceIndex];
#else
	mat4 m = multi[gl_InstanceID];
#endif
	gl_Position = matrix.project * matrix.view * m * vec4(in_position, 1);
	out_normal = (matrix.view * m * vec4(in_normal, 0)).xyz;
	out_uv = in_uv;
	out_pos = matrix.view * m * vec4(in_position,1);
}
</Module>

