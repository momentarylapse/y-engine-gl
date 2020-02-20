/*----------------------------------------------------------------------------*\
| Nix vertex buffer                                                            |
| -> handling vertex buffers                                                   |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#if HAS_LIB_GL

#include "nix.h"
#include "nix_common.h"


unsigned int VertexArrayID = 0;

namespace nix {

VertexBuffer *vb_temp = NULL;
OldVertexBuffer *vb_2d = NULL;


OldVertexBuffer::OldVertexBuffer(int _num_textures)
{
	num_textures = _num_textures;
	if (num_textures > NIX_MAX_TEXTURELEVELS)
		num_textures = NIX_MAX_TEXTURELEVELS;
	indexed = false;

	num_triangles = 0;

	buffers_created = false;
	dirty = true;

	#ifdef ENABLE_INDEX_BUFFERS
		/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
		if (!OGLVBIndex[index]){
			msg_error("IndexBuffer konnte nicht erstellt werden");
			return -1;
		}*/
	#endif
	buf_n = 0;
	buf_v = 0;
	for (int i=0; i<NIX_MAX_TEXTURELEVELS; i++)
		buf_t[i] = 0;
}

OldVertexBuffer::~OldVertexBuffer()
{
	clear();
}

void OldVertexBuffer::__init__(int _num_textures)
{
	new(this) OldVertexBuffer(_num_textures);
}

void OldVertexBuffer::__delete__()
{
	this->~OldVertexBuffer();
}

void OldVertexBuffer::clear()
{
	vertices.clear();
	normals.clear();
	for (int i=0; i<num_textures; i++)
		tex_coords[i].clear();
	num_triangles = 0;
	dirty = true;
}

void OldVertexBuffer::update()
{
	if (!buffers_created){
		glGenBuffers(1, &buf_v);
		glGenBuffers(1, &buf_n);
		glGenBuffers(num_textures, buf_t);
		buffers_created = true;
	}
	TestGLError("opt0");
	glBindBuffer(GL_ARRAY_BUFFER, buf_v);
	glBufferData(GL_ARRAY_BUFFER, vertices.num * sizeof(vertices[0]), vertices.data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, buf_n);
	glBufferData(GL_ARRAY_BUFFER, normals.num * sizeof(normals[0]), normals.data, GL_STATIC_DRAW);
	TestGLError("opt1");
	for (int i=0; i<num_textures; i++){
		glBindBuffer(GL_ARRAY_BUFFER, buf_t[i]);
		glBufferData(GL_ARRAY_BUFFER, tex_coords[i].num * sizeof(tex_coords[i][0]), tex_coords[i].data, GL_STATIC_DRAW);
	}
	dirty = false;
	TestGLError("opt2");
}

void OldVertexBuffer::addTria(const vector &p1,const vector &n1,float tu1,float tv1,
							const vector &p2,const vector &n2,float tu2,float tv2,
							const vector &p3,const vector &n3,float tu3,float tv3)
{
	vertices.add(p1);
	vertices.add(p2);
	vertices.add(p3);
	normals.add(n1);
	normals.add(n2);
	normals.add(n3);
	tex_coords[0].add(tu1);
	tex_coords[0].add(tv1);
	tex_coords[0].add(tu2);
	tex_coords[0].add(tv2);
	tex_coords[0].add(tu3);
	tex_coords[0].add(tv3);
	indexed = false;
	num_triangles ++;
	dirty = true;
}

void OldVertexBuffer::addTriaM(const vector &p1,const vector &n1,const float *t1,
								const vector &p2,const vector &n2,const float *t2,
								const vector &p3,const vector &n3,const float *t3)
{
	vertices.add(p1);
	vertices.add(p2);
	vertices.add(p3);
	normals.add(n1);
	normals.add(n2);
	normals.add(n3);
	for (int i=0;i<num_textures;i++){
		tex_coords[i].add(t1[i*2  ]);
		tex_coords[i].add(t1[i*2+1]);
		tex_coords[i].add(t2[i*2  ]);
		tex_coords[i].add(t2[i*2+1]);
		tex_coords[i].add(t3[i*2  ]);
		tex_coords[i].add(t3[i*2+1]);
	}
	indexed = false;
	num_triangles ++;
	dirty = true;
}

// for each triangle there have to be 3 vertices (p[i],n[i],t[i*2],t[i*2+1])
void OldVertexBuffer::addTrias(int num_trias, const vector *p, const vector *n, const float *t)
{
	int nv0 = vertices.num;
	vertices.resize(vertices.num + num_trias * 3);
	normals.resize(normals.num + num_trias * 3);
	memcpy(&vertices[nv0], p, sizeof(vector) * num_trias * 3);
	memcpy(&normals[nv0], n, sizeof(vector) * num_trias * 3);
	//memcpy(OGLVBTexCoords[buffer][0],t,sizeof(float)*num_trias*6);
	int nt0 = tex_coords[0].num;
	tex_coords[0].resize(nt0 + 6 * num_trias);
	for (int i=0;i<num_trias*3;i++){
		tex_coords[0][nt0 + i*2  ] = t[i*2];
		tex_coords[0][nt0 + i*2+1] = t[i*2+1];
	}
	num_triangles += num_trias;
	dirty = true;
}

void VBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices)
{
	#ifdef ENABLE_INDEX_BUFFERS
	#endif
}


// so far, map each attribute to another buffer
VertexBuffer::VertexBuffer(const string &f) {
	msg_write("new VertexBuffer " + f);
	auto xx = f.explode(",");
	num_buffers = num_attributes = xx.num;
	if (num_attributes > MAX_VB_ATTRIBUTES) {
		throw Exception("VertexBuffer: too many attributes: " + f);
		num_buffers = num_attributes = 0;
	}
	for (int i=0; i<xx.num; i++) {
		string &x = xx[i];
		auto &b = buf[i];
		b.count = 0;

		auto &a = attr[i];
		glGenBuffers(1, &b.buffer);
		a.buffer = b.buffer;
		a.normalized = false;
		a.stride = 0;
		if (x == "1f") {
			a.type = GL_FLOAT;
			a.num_components = 1;
		} else if (x == "2f") {
			a.type = GL_FLOAT;
			a.num_components = 2;
		} else if (x == "3f") {
			a.type = GL_FLOAT;
			a.num_components = 3;
		} else if (x == "3fn") {
			a.type = GL_FLOAT;
			a.num_components = 3;
			a.normalized = true;
		} else if (x == "4f") {
			a.type = GL_FLOAT;
			a.num_components = 4;
		} else {
			throw Exception("VertexBuffer: unhandled format: " + x);
		}
	}
}


VertexBuffer::~VertexBuffer() {
	for (int i=0; i<num_buffers; i++)
		glDeleteBuffers(1, &buf[i].buffer);
}

void VertexBuffer::__init__(const string &f) {
	new(this) VertexBuffer(f);
}

void VertexBuffer::__delete__() {
	this->~VertexBuffer();
}

void VertexBuffer::update(int index, const DynamicArray &a) {
	if (index < 0 or index >= MAX_VB_BUFFERS)
		throw Exception("VertexBuffer: invalid index " + i2s(index));
	buf[index].count = a.num;
	glBindBuffer(GL_ARRAY_BUFFER, buf[index].buffer);
	glBufferData(GL_ARRAY_BUFFER, a.num * a.element_size, a.data, GL_STATIC_DRAW);
}

int VertexBuffer::count() const {
	return buf[0].count;
}

int _current_vb_attr_ = 0;

void SetVertexBuffer(VertexBuffer *vb) {
	for (int i=0; i<vb->num_attributes; i++) {
		auto &a = vb->attr[i];
		glEnableVertexAttribArray(i);
		TestGLError("set vb 1");
		glBindBuffer(GL_ARRAY_BUFFER, a.buffer);
		TestGLError("set vb 2");
		glVertexAttribPointer(i, a.num_components, a.type, a.normalized, 0, (void*)0);//a.stride, (void*)a.offset);
		TestGLError("set vb 3");
	}

	for (int i=vb->num_attributes; i<_current_vb_attr_; i++)
		glDisableVertexAttribArray(i);

	_current_vb_attr_ = vb->num_attributes;
}

void init_vertex_buffers() {
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	vb_temp = new VertexBuffer("3f,3fn,2f");
	vb_2d = new OldVertexBuffer(1);
}

};

#endif