//
// sueda - geometry edits Z. Wood
// 3/16
//

#include <iostream>
#include "Particle.h"
#include "GLSL.h"
#include "MatrixStack.h"
#include "Program.h"
#include "Texture.h"


float randFloat(float l, float h)
{
	float r = rand() / (float) RAND_MAX;
	return (1.0f - r) * l + r * h;
}

Particle::Particle(vec3 start) :
	charge(1.0f),
	m(1.0f),
	d(0.0f),
	x(start),
	v(0.0f, 0.0f, 0.0f),
	lifespan(1.0f),
	tEnd(0.0f),
	scale(2.0f),
	color(1.0f, 1.0f, 1.0f, 1.0f)
{
}

Particle::~Particle()
{
}

void Particle::load(vec3 start)
{
	// Random initialization
	rebirth(0.0f, start);
}

/* all particles born at the origin */
void Particle::rebirth(float t, vec3 start)
{
	charge = randFloat(0.0f, 1.0f) < 0.5 ? -1.0f : 1.0f;	
	m = 1.0f;
  	d = randFloat(0.0f, 0.02f);
	x = start;
	// v.x = randFloat(-0.27f, 0.3f);
	// v.y = randFloat(-0.1f, 0.9f);
	// v.z = randFloat(-0.3f, 0.27f);
	// Give each particle a random upward-ish direction
	vec3 dir = normalize(vec3(
		randFloat(-1, 1),
		randFloat(0.2, 1.0),   // upward bias
		randFloat(-1, 1)
	));

	// Speed range (adjust visually)
	float speed = randFloat(0.5f, 2.0f);

	v = dir * speed;
	lifespan = randFloat(100.0f, 200.0f); 
	tEnd = t + lifespan;
	scale = randFloat(0.2, 1.0f);
   	// color.r = randFloat(0.3f, 1.0f);
	// color.g = randFloat(0.3f, 1.0f);
	// color.b = randFloat(0.3f, 1.0f);
	// color.a = 1.0f;
    color = vec4(
    randFloat(0.6f, 0.9f),    // brown/yellow
    randFloat(0.4f, 0.7f),
    randFloat(0.1f, 0.3f),
    1.0f);
	//color = glm::vec4(0.40f, 0.22f, 0.12f, 1.0f);
}

// void Particle::update(float t, float h, const vec3 &g, const vec3 start)
// {
// 	if(t > tEnd) {
// 		rebirth(t, start);
// 	}

// 	//very simple update
// 	x += h*v;
// 	//To do - how do you want to update the forces?
// 	color.a = (tEnd-t)/lifespan;
// }

void Particle::update(float t, float h, const vec3 &g, const vec3 start)
{
    if(t > tEnd) {
        rebirth(t, start);
    }

    // --- Improved physics ---
    vec3 a = g;                // gravity
    float damping = 0.995f;     // mild drag
    v *= damping;

    vec3 noise = vec3(
        (randFloat(-1,1)) * 0.05,
        (randFloat(-1,1)) * 0.05,
        (randFloat(-1,1)) * 0.05
    );
    a += noise;

    v += h * a;   // integrate acceleration → velocity
    x += h * v;   // integrate velocity → position

    // alpha fade
    color.a = (tEnd - t) / lifespan;
}
