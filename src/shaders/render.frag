#version 330
#extension GL_ARB_gpu_shader_fp64 : enable
#define EPS 1e-6
#define INF 1e10
#define DIFFUSE 0
#define SPECULAR 1
#define REFRACTION 2
#define DEPTH_MAX 60
#define DEPTH_MIN 20
precision highp float;
const float PI = acos(-1.0);
const uint UINT_MAX = 4294967295U;

uniform vec2 resolution;
uniform vec2 mouse;
uniform int frame;
uniform int numSamples;

out vec4 out_color;

float pixSize = 1.0;
float gamma = 2.2;

struct Sphere {
	float radius;
	vec3 center;
	vec3 color;
	vec3 emission;
	int reflectType;
};

struct Scene{
	Sphere s[10];
};

struct Ray{
	vec3 org;
	vec3 dir;
};

struct Hitpoint{
	vec3 pos;
	vec3 normal;
	float dist;
	int objectId;
};

uint seed_[4];

void initSeeds(uint seed){
	seed_[0] = 1812433253U * (seed ^ (seed >> 30U)) + 1U;
	seed_[1] = 1812433253U * (seed_[0] ^ (seed_[0] >> 30U)) + 2U;
	seed_[2] = 1812433253U * (seed_[1] ^ (seed_[1] >> 30U)) + 3U;
	seed_[3] = 1812433253U * (seed_[2] ^ (seed_[2] >> 30U)) + 4U; 
}
uint rand(){
	uint t = seed_[0] ^ (seed_[0] << 11U);
	seed_[0] = seed_[1]; 
	seed_[1] = seed_[2];
	seed_[2] = seed_[3];
	seed_[3] = (seed_[3] ^ (seed_[3] >> 19U)) ^ (t ^ (t >> 8U)); 
	return seed_[3];
}

float rand01(){
	//return noise1(float(rand()) / float(UINT_MAX)) / 2.0 + 0.5;
	return float(rand()) / float(UINT_MAX);
}


vec3 gammaCorrection(vec3 color){
	return pow(color, vec3(1.0/gamma));
}

bool intersectSphere2(Ray r, Sphere s, inout Hitpoint hp){
	//float a = dot(r.dir, r.dir);
	float b = dot(r.dir, r.org - s.center);
	float c = dot(r.org - s.center, r.org - s.center) - s.radius * s.radius;
	float D = b * b - c;
	//float D = b * b - dot(r.org - s.center, r.org - s.center) + s.radius * s.radius;
	if(D < 0.0){
		return false;
	}
	float sD = sqrt(D);
	float t1, t2;
	if(b > 0.0){
		t1 = float(-b -sD);
		t2 = float(c) / t1;
	}else{
		t2 = float(-b + sD);
		t1 = float(c) / t2;
	}
//	if(t1 > t2) {
//		float tmp = t1;
//		t1 = t2;
//		t2 = tmp;
//	}
	//t1 = float(-b - sD), t2 = float(-b + sD);
	if(t1 < EPS && t2 < EPS) return false;
	if(t1 > EPS){
		hp.dist = float(t1);
	}
	else{
		hp.dist = float(t2);
	}
	hp.pos = r.org + hp.dist * r.dir;
	hp.normal = normalize(hp.pos - s.center);
	return true;
}


bool intersectSphere(Ray r, Sphere s, inout Hitpoint hp){
	float a = dot(r.dir, r.dir);
	float b = 2.0 * dot(r.dir, r.org - s.center);
	float c = dot(r.org - s.center, r.org - s.center) - s.radius * s.radius;
	float D = b * b - 4.0 * a * c;
	if(D < 0.0){
		return false;
	}
	float x = (-b + sqrt(D)) / (2.0 * a);
	float y = (-b - sqrt(D)) / (2.0 * a);
	//if(x < EPS && y < EPS) return false;
	if(y > EPS){
		hp.pos = r.org + y * r.dir;
		hp.normal = normalize(hp.pos - s.center);
		hp.dist = y;
		return true;
	}
	else{
		if(x > EPS){
			hp.pos = r.org + x * r.dir;
			hp.normal = -normalize(hp.pos - s.center);
			hp.dist = x;
			return true;
		}
		return false;
	}
}


bool intersectScene(Ray r, Scene scene, inout Hitpoint hp){
	hp.dist = INF;
	hp.objectId = -1;
	for(int i = 0;i < 10; i++){
		Hitpoint tmp;
		tmp.objectId = i;
		if(intersectSphere2(r, scene.s[i], tmp)){
			if(tmp.dist < hp.dist){
				hp.pos = tmp.pos;
				hp.normal = tmp.normal;
				hp.dist = tmp.dist;
				hp.objectId = tmp.objectId;
			}
		}
	}
	return (hp.objectId != -1);
}



#define HASHSCALE3 vec3(.1031, .1030, .0973)
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
	p3 += dot(p3, p3.yzx+19.19);
	return fract((p3.xx+p3.yz)*p3.zy);
}

vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
	p3 += dot(p3, p3.yxz+19.19);
	return fract((p3.xxy + p3.yxx)*p3.zyx);

}


vec3 radiance(Ray ray, Scene scene){
	vec3 accumulatedColor = vec3(0.0);
	vec3 accumulatedReflectance = vec3(1.0);
	int depth = 0;
	Ray nowRay = ray;
	for(;;depth++){
		Hitpoint hp;
		//intersectScene(nowRay, scene, hp);
		if(!intersectScene(nowRay, scene, hp)){
			//return vec3(0.0);
			return accumulatedColor;
		}
		Sphere obj = scene.s[hp.objectId];
		//vec3 orientingNormal = hp.normal;
		vec3 orientingNormal = (dot(hp.normal , nowRay.dir) < 0.0 ? hp.normal: (-1.0 * hp.normal));
		vec3 seed = vec3( gl_FragCoord.xy, float(frame) * 0.3 ) + float( depth ) * 500.0 + 50.0;
		vec3 Xi = hash33( seed );
		//if(hp.objectId == 6)return vec3(10.0);
		accumulatedColor += accumulatedReflectance * obj.emission;

		float rrp = max(obj.color.x, max(obj.color.y, obj.color.z));//roussian roulette probability
		if(depth > DEPTH_MAX){
			rrp *= pow(0.5, float(depth - DEPTH_MAX));
		}

		if(depth > DEPTH_MIN){
			float rnd = rand01();
			rnd = Xi.z;
			if(rnd >= rrp){
				//return vec3(0.0);
				return accumulatedColor;
			}
		}else{
			rrp = 1.0;
		}

		switch(obj.reflectType){
			case DIFFUSE: {
				vec3 w, u, v;
				w = orientingNormal;
				if(abs(w.x) > 0.0) u = normalize(cross(vec3(0.0, 1.0, 0.0), w));
				else u = normalize(cross(vec3(1.0, 0.0, 0.0), w));
				v = normalize(cross(w, u));
				float r1 = 2.0 * PI * Xi.x; //rand01();
				//r1 = 2.0 * PI * rand01();
				float r2 = Xi.y; //rand01();
				//r2 = rand01();
				float r2s = sqrt(r2);
				vec3 dir = normalize(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2));
				nowRay = Ray(hp.pos, dir);
				accumulatedReflectance *= obj.color / rrp;
				continue;
			}break;

			case SPECULAR:{
				nowRay = Ray(hp.pos, nowRay.dir - hp.normal * 2.0 * dot(hp.normal, nowRay.dir));
				accumulatedReflectance = accumulatedReflectance * obj.color / rrp;
				continue;
			}break;
		}
	}
	return accumulatedColor;
}
float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

void main(){
	uint seed = uint(gl_FragCoord.x * gl_FragCoord.y * gl_FragCoord.y * frame + 1.0);
	//float hoge = noise1(0);
	//uint seed = uint(random(gl_FragCoord.xy) * float(UINT_MAX));
	initSeeds(seed);
	Scene scene;
	scene.s[0] = Sphere(1e6, vec3( 1e6+1.0, 40.8, 81.6),  vec3(0.75, 0.25, 0.25), vec3(0.0), DIFFUSE); // 左
	scene.s[1] = Sphere(1e6, vec3(-1e6+99.0, 40.8, 81.6), vec3(0.25, 0.25, 0.75), vec3(0.0), DIFFUSE); // 右
	scene.s[2] = Sphere(1e6, vec3(50, 40.8 , 1e6),         vec3(0.75, 0.75, 0.75), vec3(0.0), DIFFUSE); // 奥
	scene.s[3] = Sphere(1e6, vec3(50, 40.8, -1e6+250.0),  vec3(0.0)             , vec3(0.0), DIFFUSE); // 手前
	scene.s[4] = Sphere(1e6, vec3(50, 1e6, 81.6),         vec3(0.75, 0.75, 0.75), vec3(0.0), DIFFUSE); // 床
	scene.s[5] = Sphere(1e6, vec3(50, -1e6+81.6, 81.6),   vec3(0.75, 0.75, 0.75), vec3(0.0), DIFFUSE); // 天井
	scene.s[6] = Sphere(15.0, vec3(50.0, 90.0, 81.6),     vec3(0.0)             , vec3(36.0, 36.0, 36.0),  DIFFUSE); //照明
	scene.s[7] = Sphere(20.0, vec3(65.0, 20.0, 20.0),     vec3(0.25, 0.75, 0.25), vec3(0.0), DIFFUSE); // 緑球
	scene.s[8] = Sphere(16.5, vec3(27.0, 16.5, 47.0),     vec3(0.99, 0.99, 0.99), vec3(0.0), SPECULAR), // 鏡
	scene.s[9] = Sphere(16.5, vec3(77.0, 16.5, 78.0),     vec3(0.99, 0.99, 0.99), vec3(0.0), SPECULAR); //ガラス
	const vec3 cameraPosition = vec3(50.0, 52.0, 220.0);
	const vec3 cameraDir      = normalize(vec3(0.0, -0.04, -1.0));
	const vec3 cameraUp = vec3(0.0, 1.0, 0.0);
	float screenDist = 40.0;
	float screenHeight = 30.0;
	float screenWidth = screenHeight * resolution.x / resolution.y;
	vec3 screenX = normalize(cross(cameraDir, cameraUp));
	vec3 screenY = normalize(cross(screenX, cameraDir));
	vec3 screenCenter = cameraPosition + cameraDir * screenDist;
	float pixSize = screenHeight / resolution.y;

	//vec3 pixPos = vec3((gl_FragCoord.xy - resolution / 2.0) * pixSize, 0.0) + screenCenter;
	vec3 pixPos = screenX * (gl_FragCoord.x / resolution.x - 0.5) * screenWidth + screenY * (gl_FragCoord.y / resolution.y - 0.5) * screenHeight + screenCenter;
	vec3 sumRadiance = vec3(0.0);
	for(int sx = 1; sx <= 4; sx++){
		for(int sy = 1; sy <= 4; sy++){
			vec3 spixPos = pixPos + pixSize / 4.0 * (screenX + screenY);
			Ray ray = Ray(cameraPosition, normalize(spixPos - cameraPosition));
			sumRadiance += radiance(ray, scene) / float(numSamples * 4 * 4);
		}
	}
//	int n = 1;
//	for(int i=0;i < n;i++){
//		vec3 pos = pixPos + pixSize / 2.0 * vec3(1.0, 1.0, 0.0);
//		Ray ray = Ray(cameraPosition, normalize(pos - cameraPosition));
//		sumRadiance += radiance(ray, scene) / float(numSamples);
//		//sumRadiance = max(sumRadiance, radiance(ray, scene));
//	}
	out_color = vec4(sumRadiance, 1.0);
	return;
	out_color = vec4(gammaCorrection(clamp(sumRadiance, 0.0, 1.0)), 1.0);
	return;
	//Hitpoint hp;
	//Ray ray = Ray(cameraPosition, normalize(pixPos - cameraPosition));
//	if(!intersectScene(ray, scene, hp)){
//		out_color = vec4(0.0,0.0,0.0, 0.0);
//	}else{
//		out_color = vec4(vec3(hp.dist / 1000.0), 1.0);
//	}
}
	