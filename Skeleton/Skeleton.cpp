// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Proh?szka Botond Bendeg?z
// Neptun : DG1647
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================

#include "framework.h"

const char *vertexSource = R"(
	#version 330
	precision highp float;

	uniform vec3 wLookAt, wRight, wUp;
	layout(location = 0) in vec2 cCamWindowVertex;
	out vec3 p;

	void main() {
		gl_Position = vec4(cCamWindowVertex, 0, 1);
		p = wLookAt + wRight * cCamWindowVertex.x + wUp * cCamWindowVertex.y;
	}
)";

const char *fragmentSource = R"(
	#version 330
	precision highp float;
	
	const float A = 0.9f;
	const float B = 0.2f;
	const float C = 0.4f;

	const vec3 La = vec3(0.5f, 0.8f, 0.9f);
	const vec3 Le = vec3(0.6f, 0.9f, 1.0f);
	const vec3 lightPosition = vec3(0.4f, 0.4f, 0.25f);
	const vec3 ka = vec3(0.5f, 0.5f, 0.5f);
	const float shininess = 500.0f;
	const int maxdepth = 5;
	const float epsilon = 0.1f;
	const float rotateTheta = 72.0f;
	const float disFromCorner = 0.1f;
	const float maxObjectSize = 0.3f;
	
	struct Hit {
		float t;
		vec3 position, normal, origo;
		int mat;
	};

	struct Ray {
		vec3 start, dir, weight;
	};

	const int objFaces = 12;
	uniform vec3 wEye, v[20];
	uniform int planes[objFaces * 5];
	uniform vec3 kd, ks, F0;

	void getObjPlane(int i, out vec3 p, out vec3 normal) {
		vec3 p1 = v[planes[5 * i] - 1], p2 = v[planes[5 * i + 1] - 1], p3 = v[planes[5 * i + 2] -1];
		normal = cross(p2 - p1, p3 - p1);
		if(dot(p1, normal) < 0) normal = -normal;
		p = p1;
	}
	
	Hit intersectConvexPolyhedron(Ray ray, Hit hit) {
		for(int i = 0; i < objFaces; i++) {
			vec3 p1, normal;
			getObjPlane(i, p1, normal);
			float ti = abs(dot(normal, ray.dir)) > epsilon ? dot(p1 - ray.start, normal) / dot(normal, ray.dir) : -1;
			if (ti <= epsilon || (ti > hit.t && hit.t > 0)) continue;
			vec3 pintersect = ray.start + ray.dir * ti;
			bool outside = false;
			for(int j = 0; j < objFaces; j++) {
				if (i == j) continue;
				vec3 p11, n;
				getObjPlane(j, p11, n);
				if (dot(n, pintersect - p11) > 0) {
					outside = true;
					break;
				}
			}
			if (!outside) {
				hit.mat = 3;
				
				hit.t = ti;
				hit.position = pintersect;
				hit.normal = normalize(normal);
				
				for(int j = 0; j < 5; j++){

					vec3 x1 = v[planes[5 * i + j] - 1];
					vec3 x2 = v[planes[5 * i +((j + 1) % 5)] - 1];
					vec3 x0 = hit.position;
					float dis = length(cross((x2 - x1), (x1 - x0))) / length(x2 - x1);
					if(dis < disFromCorner){
						hit.mat = 1;
						break;	
					}
				vec3 p = vec3(0,0,0);
				for(int j = 0; j < 5; j++) p += v[planes[5 * i + j] - 1];

				hit.origo = (p / 5.0f);
				}
			}
		}
		return hit;
	}

	Hit solveQuadratic(float a, float b, float c, Ray ray, Hit hit, float normz){
		float discr = b * b - 4.0f * a * c;
		if(discr >= 0){
			float sqrt_discr = sqrt(discr);
			float t1 = (-b + sqrt_discr) / 2.0f / a;
			vec3 p = ray.start + ray.dir * t1;
			if(sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z,2) )> maxObjectSize) t1 = -1.0f;			
			float t2 = (-b - sqrt_discr) / 2.0f / a;
			p = ray.start + ray.dir * t2;
			
			if(sqrt(pow(p.x, 2) + pow(p.y, 2) + pow(p.z,2) )> maxObjectSize) t2 = -1.0f;
			if(t2 > 0 && (t2 < t1 || t1 < 0)) t1 = t2;
			if(t1 > 0 && (t1 < hit.t || hit.t < 0)) {
				hit.t = t1;
				hit.position = ray.start + ray.dir * hit.t;
				hit.normal = normalize(vec3(-hit.position.x, -hit.position.y, normz));
				hit.mat = 2;
			}
		}
		return hit;
	}

	Hit intersectObject(Ray ray, Hit hit) {
		float a = A * pow(ray.dir.x, 2) + B * pow(ray.dir.y, 2);
		float b = 2 * A * ray.start.x * ray.dir.x + 2 * B * ray.start.y * ray.dir.y - C * ray.dir.z;
		float c = A * pow(ray.start.x, 2) + B * pow(ray.start.y, 2) - C * ray.start.z;
		hit = solveQuadratic(a, b, c, ray, hit, 2 * maxObjectSize);
		
		return hit;
	}

	Hit firstIntersect(Ray ray) {
		Hit bestHit;
		bestHit.t = -1;
		bestHit = intersectObject(ray, bestHit);
		bestHit = intersectConvexPolyhedron(ray, bestHit);
		if(dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
		return bestHit;
	}

	mat4 rotationMatrix;
	vec4 inputMatrix;
	vec4 outputMatrix;

void setUpRotationMatrix(float angle, vec3 vector)
{
	float u = vector.x, v = vector.y, w = vector.z;
    float L = (u*u + v * v + w * w);
    angle = angle * 3.1415f / 180.0f; //converting to radian value
    float u2 = u * u;
    float v2 = v * v;
    float w2 = w * w; 
	//followtutorials.com/2012/03/3d-rotation-algorithm-about-arbitrary-axis-with-c-c-code.html
    rotationMatrix[0][0] = (u2 + (v2 + w2) * cos(angle)) / L; rotationMatrix[0][1] = (u * v * (1 - cos(angle)) - w * sqrt(L) * sin(angle)) / L; rotationMatrix[0][2] = (u * w * (1 - cos(angle)) + v * sqrt(L) * sin(angle)) / L; rotationMatrix[0][3] = 0.0; 
    rotationMatrix[1][0] = (u * v * (1 - cos(angle)) + w * sqrt(L) * sin(angle)) / L; rotationMatrix[1][1] = (v2 + (u2 + w2) * cos(angle)) / L; rotationMatrix[1][2] = (v * w * (1 - cos(angle)) - u * sqrt(L) * sin(angle)) / L; rotationMatrix[1][3] = 0.0; 
    rotationMatrix[2][0] = (u * w * (1 - cos(angle)) - v * sqrt(L) * sin(angle)) / L; rotationMatrix[2][1] = (v * w * (1 - cos(angle)) + u * sqrt(L) * sin(angle)) / L; rotationMatrix[2][2] = (w2 + (u2 + v2) * cos(angle)) / L; rotationMatrix[2][3] = 0.0; 
    rotationMatrix[3][0] = 0.0; rotationMatrix[3][1] = 0.0; rotationMatrix[3][2] = 0.0; rotationMatrix[3][3] = 1.0;
}

vec3 Rotate(vec3 points, vec3 v, float s){
	float angle = s * rotateTheta;
   
    inputMatrix.x= points.x;
    inputMatrix.y = points.y;
    inputMatrix.z = points.z;
    inputMatrix.w = 1.0;
    setUpRotationMatrix(angle, v);
    outputMatrix = rotationMatrix * inputMatrix;
	return vec3(outputMatrix.x, outputMatrix.y, outputMatrix.z);

}

	vec3 trace(Ray ray) {
		vec3 outRadiance = vec3(0, 0, 0);
		for(int d = 0; d < maxdepth; d++) {
			Hit hit = firstIntersect(ray);
			if (hit.t < 0) break;
			if (hit.mat < 2) {
				vec3 lightdir = normalize(lightPosition - hit.position);
				float cosTheta = dot(hit.normal, lightdir);
				if (cosTheta > 0) {
					vec3 LeIn = Le / dot(lightPosition - hit.position, lightPosition - hit.position);
					outRadiance += ray.weight * LeIn * kd[hit.mat] * cosTheta;
					vec3 halfway = normalize(-ray.dir + lightdir);
					float cosDelta = dot(hit.normal, halfway);
					if (cosDelta > 0) outRadiance += ray.weight * LeIn * ks[hit.mat] * pow(cosDelta, shininess);
				}
				ray.weight *= ka;
				break;
			}
			else if(hit.mat == 3){
				ray.dir = reflect(ray.dir, hit.normal);
				ray.start = hit.position + hit.normal * epsilon;

				float rotateDirection = 1.0f;

				if(d % 2 == 1) rotateDirection *= -1.0f;
				ray.start = Rotate(ray.start, hit.origo, rotateDirection);
				ray.dir = Rotate(ray.dir, hit.origo, rotateDirection);
			}
			else{
				ray.weight *= F0 + (vec3(1, 1, 1) - F0) * pow(1 - dot(-ray.dir, hit.normal), 5);
				ray.start = hit.position + hit.normal * epsilon;
				ray.dir = reflect(ray.dir, hit.normal);
			}
		}
		outRadiance += ray.weight * La;
		return outRadiance;
	}

	in vec3 p;
	out vec4 fragmentColor;

	void main() {
		Ray ray;
		ray.start = wEye;
		ray.dir = normalize(p - wEye);
		ray.weight = vec3(1, 1, 1);
		fragmentColor = vec4(trace(ray), 1);
	}
)";
GPUProgram gpuProgram;

struct Camera {
	vec3 eye, lookat, right, pvup, rvup;
	float fov = 45 * (float)M_PI / 180;

	Camera() : eye(0, 1, 1), pvup(0, 0, 1), lookat(0, 0, 0) { set(); }
	void set() {
		vec3 w = eye - lookat;
		float f = length(w);
		right = normalize(cross(pvup, w)) * f * tanf(fov / 2);
		rvup = normalize(cross(w, right)) * f * tanf(fov / 2);
	}
	void Animate(float t) {
		float r = sqrtf(eye.x * eye.x + eye.y * eye.y);
		eye = vec3(r * cos(t) + lookat.x, r * sin(t) + lookat.y, eye.z);
		set();
	}
	void Step(float step) {
		eye = normalize(eye + right * step) * length(eye);
		set();
	}
};

GPUProgram shader;
Camera camera;
bool animate = true;
float F(float n, float k) { return ((n - 1) * (n - 1) + k * k) / ((n + 1) * (n + 1) + k * k); }

void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	unsigned int vao, vbo;
	glGenVertexArrays(1, &vao); glBindVertexArray(vao);
	glGenBuffers(1, &vbo);		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	float vertexCoords[] = { -1, -1,	 1, -1,		1, 1,	 -1, 1 };
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	shader.create(vertexSource, fragmentSource, "fragmentColor");

	const float g = 0.618f, G = 1.618f;
	std::vector<vec3> v = {
		vec3(0, g, G), vec3(0, -g, G), vec3(0, -g, -G), vec3(0, g, -G),		vec3(G, 0, g), vec3(-G, 0, g), vec3(-G, 0, -g), vec3(G, 0, -g),	 vec3(g, G , 0), vec3(-g, G, 0),
		vec3(-g, -G, 0), vec3(g, -G, 0), vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, -1, 1), vec3(1, -1, 1), vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1)
	};

	for (int i = 0; i < v.size(); i++) shader.setUniform(v[i], "v[" + std::to_string(i) + "]");

	std::vector<int> planes = {
		1, 2, 16, 5, 13,	1, 13, 9, 10, 14,	1, 14, 6, 15, 2,	2, 15, 11, 12, 16,	3, 4, 18, 8, 17,	3, 17, 12, 11, 20,	3, 20, 7, 19, 4,	19, 10, 9, 18, 4,	16, 12, 17, 8, 5,	5, 8, 18, 9, 13,	14, 10, 19, 7, 6,	6, 7, 20, 11, 15
	};
	for (int i = 0; i < planes.size(); i++) shader.setUniform(planes[i], "planes[" + std::to_string(i) + "]");

	shader.setUniform(vec3(0.4f, 0.4f, 0.4f), "kd");
	shader.setUniform(vec3(1, 1, 1), "ks");
	shader.setUniform(vec3(F(0.17, 3.1), F(0.35, 2.7), F(1.5, 1.9)), "F0");
	printf("Usage:\na: Step left\nd: Step right\ne: Enable animation\n");
}

void onDisplay() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);
	shader.setUniform(camera.eye, "wEye");
	shader.setUniform(camera.lookat, "wLookAt");
	shader.setUniform(camera.right, "wRight");
	shader.setUniform(camera.rvup, "wUp");
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glutSwapBuffers();
}

void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') camera.Step(0.1f);
	if (key == 'a') camera.Step(-0.1f);
	if (key == 'e') animate = !animate;
}

void onKeyboardUp(unsigned char key, int pX, int pY) {}
void onMouseMotion(int pX, int pY) {}
void onMouse(int button, int state, int pX, int pY) {}

void onIdle() {
	if (animate) camera.Animate(glutGet(GLUT_ELAPSED_TIME) / 5000.0f);
	glutPostRedisplay();
}
