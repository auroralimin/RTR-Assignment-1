#include "draw.h"
#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#define VERTICES 0
#define NORMALS 1
#define INDICES 2

void updateGridI(void);
void updateGridV(void);
void updateGridN(void);
void allocGridI(void);
void allocGridV(void);
void allocGridN(void);

static int colours[][3] = {
	{1, 0, 0},
	{0, 1, 0},
	{0, 0, 1}
};

static int axes[][3] = {
	{1, 0, 0},
	{0, 1, 0},
	{0, 0, 1}
};

static color3f white =	{1.0, 1.0, 1.0};
static color3f cyan =	{0.0, 1.0, 1.0};

static vec3f *gridV = NULL;
static vec3f *gridN = NULL;
static int *gridI = NULL;
static GLvoid **gridIoffset = NULL;
GLuint buffers[NUM_BUFFERS];

void drawAxes(float length)
{
	glPushAttrib(GL_CURRENT_BIT);
	glBegin(GL_LINES);

	for (int i = 0; i < 3; i++)
	{
		glColor3f(colours[i][0], colours[i][1], colours[i][2]);
		glVertex3f(-axes[i][0]*length, -axes[i][1]*length, -axes[i][2]*length);
		glVertex3f(axes[i][0]*length, axes[i][1]*length, axes[i][2]*length);
	}
	glEnd();
	glPopAttrib();
}

void drawVector(vec3f *r, vec3f *v, float s, bool normalize, color3f *c)
{
	glPushAttrib(GL_CURRENT_BIT);
	glColor3fv((GLfloat *)c);
	glBegin(GL_LINES);
	if (normalize) {
		float mag = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
		v->x /= mag;
		v->y /= mag;
		v->z /= mag;
	}
	glVertex3fv((GLfloat *)r);
	glVertex3f(r->x + s * v->x, r->y + s * v->y, r->z + s * v->z);
	glEnd();
	glPopAttrib();
}

void updateGridI(void)
{
	int iFlag = 0, i, j;

	for (j = 0; j < g.tess; j++)
	{
		for (i = 0; i < (g.tess + 1); i++)
		{
			gridI[iFlag++] = (j*(g.tess+1)) + i;
			gridI[iFlag++] = ((j+1)*(g.tess+1)) + i;
		}
		j++;

		for (i = g.tess; i >= 0; i--)
		{
			gridI[iFlag++] = ((j+1)*(g.tess+1)) + i;
			gridI[iFlag++] = (j*(g.tess+1)) + i;
		}
	}

	for (i = 0; i < g.tess; i++)
	{
		gridIoffset[i] = (GLvoid *) (i * (g.tess + 1) * 2 * sizeof(GLuint));
	}
}

void updateGridV(void)
{
	int vFlag = 0;
	float t = g.t;
	float rowStep = 2.0f / (float) g.tess;
	float colStep = 2.0f / (float) g.tess;
	const float A2 = 0.25, k2 = 2.0 * M_PI, w2 = 0.25;
	const float A1 = 0.25, k1 = 2.0 * M_PI, w1 = 0.25;


	if (g.waveDim == 2)
	{
		for (int i = 0; i <= g.tess; i++)
			for (int j = 0; j <= g.tess; j++)
			{
				gridV[vFlag].x = -1.0 + i*colStep;
				gridV[vFlag].z = -1.0 + j*rowStep;
				gridV[vFlag].y = A1 * sinf(k1 * gridV[vFlag].x + w1 * t);
				vFlag++;
			}
	}
	else if (g.waveDim == 3)
	{
		for (int i = 0; i <= g.tess; i++)
			for (int j = 0; j <= g.tess; j++)
			{
				gridV[vFlag].x = -1.0 + i*colStep;
				gridV[vFlag].z = -1.0 + j*rowStep;
				gridV[vFlag].y = A1 * sinf(k1 * gridV[vFlag].x + w1 * t) +
					A2 * sinf(k2 * gridV[vFlag].z + w2 * t);
				vFlag++;
			}
	}
}

void updateGridN(void)
{
	int nFlag = 0;
	float t = g.t;
	const float A2 = 0.25, k2 = 2.0 * M_PI, w2 = 0.25;
	const float A1 = 0.25, k1 = 2.0 * M_PI, w1 = 0.25;

	if (g.waveDim == 2)
	{
		for (int i = 0; i <= g.tess; i++)
			for (int j = 0; j <= g.tess; j++)
			{
				gridN[nFlag].x = -A1*k1*cosf(k1 * gridV[nFlag].x + w1 * t);
				gridN[nFlag].y = 1.0;
				gridN[nFlag].z = 0.0;
				nFlag++;
			}
	}
	else if (g.waveDim == 3)
	{
		for (int i = 0; i <= g.tess; i++)
			for (int j = 0; j <= g.tess; j++)
			{
				gridN[nFlag].x = -A1*k1*cosf(k1 * gridV[nFlag].x + w1 * t);
				gridN[nFlag].y = 1.0;
				gridN[nFlag].z = -A2*k2*cosf(k2 * gridV[nFlag].z + w2 * t);
				nFlag++;
			}
	}
}

void updateSineWave(void)
{
	allocGridI();
	allocGridV();
	allocGridN();
	updateGridI();
	updateGridV();

	if (g.lighting)
		updateGridN();

	bufferData();
}

void drawSineWave(void)
{
	int i;

	glPushAttrib(GL_CURRENT_BIT);

	if (g.lighting) {
		glEnable(GL_NORMALIZE);
		for (i = 0; i < NLIGHTS; i++)
		{
			if (i < g.n_lights)
				glEnable(GL_LIGHT0 + i);
			else
				glDisable(GL_LIGHT0 + i);
		}

		glEnable(GL_LIGHTING);
		glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *) &white);
		glMaterialf(GL_FRONT, GL_SHININESS, 20.0);
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	} else {
		glDisable(GL_LIGHTING);
	}

	if (g.polygonMode == line)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glColor3f(white.r, white.g, white.b);

	if (g.renderMode == immediate)
		drawAsImmediate();
	else
		drawAsVBO();

	/* Grid */
	if (g.lighting) {
		glDisable(GL_LIGHTING);
	}

	// Normals
	if (g.drawNormals)
		drawNormals();

	glPopAttrib();
}

void drawNormals(void)
{
	vec3f r, n;
	int i, j, tess = g.tess;
	float stepSize = 2.0 / tess;
	float t = g.t;
	const float A2 = 0.25, k2 = 2.0 * M_PI, w2 = 0.25;
	const float A1 = 0.25, k1 = 2.0 * M_PI, w1 = 0.25;

	for (j = 0; j <= tess; j++) {
		for (i = 0; i <= tess; i++) {
			r.x = -1.0 + i * stepSize;
			r.z = -1.0 + j * stepSize;

			n.y = 1.0;
			n.x = - A1 * k1 * cosf(k1 * r.x + w1 * t);
			if (g.waveDim == 2) {
				r.y = A1 * sinf(k1 * r.x + w1 * t);
				n.z = 0.0;
			} else {
				r.y = A1 * sinf(k1 * r.x + w1 * t) + A2 * sinf(k2 * r.z + w2 * t);
				n.z = - A2 * k2 * cosf(k2 * r.z + w2 * t);
			}

			drawVector(&r, &n, 0.05, true, &cyan);
		}
	}
}

void drawAsImmediate(void)
{
	int i;

	for (i = 0; i < g.tess; i++)
	{
		glBegin(GL_TRIANGLE_STRIP);
		int offset = i * (g.tess + 1) * 2;
		for (int j = 0; j < (g.tess + 1) * 2; j++)
		{
			glNormal3fv((GLfloat *)&gridN[gridI[offset + j]]);
			glVertex3fv((GLfloat *)&gridV[gridI[offset + j]]);

		}
		glEnd();
	}
}

void initVBO(void)
{

	glGenBuffers(NUM_BUFFERS, buffers);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

}

void unBindBuffers()
{
	int buffer;

	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
	if (buffer != 0)
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buffer);
	if (buffer != 0)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void bufferData(void)
{

	glBindBuffer(GL_ARRAY_BUFFER, buffers[VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, (g.tess+1)*(g.tess+1)*sizeof(vec3f), gridV, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[NORMALS]);
	glBufferData(GL_ARRAY_BUFFER, (g.tess+1)*(g.tess+1)*sizeof(vec3f), gridN, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[INDICES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (g.tess+1)*g.tess*2*sizeof(int), gridI, GL_STATIC_DRAW);

	unBindBuffers();

}

void drawAsVBO(void)
{
	int i;

	glBindBuffer(GL_ARRAY_BUFFER, buffers[VERTICES]);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, buffers[NORMALS]);
	glNormalPointer(GL_FLOAT, 0, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[INDICES]);

	if(g.renderMode == singleVBO)
		glDrawElements (GL_TRIANGLE_STRIP, g.tess*(g.tess+1)*2, GL_UNSIGNED_INT, 0);
	else
	{
		for (i = 0; i < g.tess; i++)
			glDrawElements (GL_TRIANGLE_STRIP, (g.tess+1)*2, GL_UNSIGNED_INT, gridIoffset[i]);
	}

	checkForGLerrors(__LINE__, __FILE__);
}

void freeSineWaveArrays(void)
{
	free(gridV);
	free(gridI);
	free(gridIoffset);
	free(gridN);
}

void allocGridI(void)
{
	GLvoid **auxIoffset;
	int *auxI = (int *) realloc(gridI, (g.tess+1)*g.tess*2*sizeof(int));
	if (auxI == NULL)
	{
		freeSineWaveArrays();
		fprintf(stderr, "Realloc failed!\n");
		exit(1);
	}

	gridI = auxI;

	auxIoffset = (GLvoid **) realloc(gridIoffset, g.tess*sizeof(GLvoid *));
	if (auxI == NULL)
	{
		freeSineWaveArrays();
		fprintf(stderr, "Realloc failed!\n");
		exit(1);
	}

	gridIoffset = auxIoffset;

}

void allocGridV(void)
{
	vec3f *auxV = (vec3f *) realloc(gridV, (g.tess+1)*(g.tess+1)*sizeof(vec3f));
	if (auxV == NULL)
	{
		freeSineWaveArrays();
		fprintf(stderr, "Realloc failed!\n");
		exit(1);
	}
	gridV = auxV;
}

void allocGridN(void)
{
	vec3f *auxN = (vec3f *) realloc(gridN, (g.tess+1)*(g.tess+1)*sizeof(vec3f));
	if (auxN == NULL)
	{
		freeSineWaveArrays();
		fprintf(stderr, "Realloc failed!\n");
		exit(1);
	}
	gridN = auxN;
}

