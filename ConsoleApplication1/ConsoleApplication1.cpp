#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

//----- Constants -----//
#define gMAX_PARTICLES 100
const int32_t gMAX_PARTICULE_CHARGE = 3;
const int32_t gMIN_PARTICULE_CHARGE = -3;
const double gMAX_SPEED = 1;
const double gMAX_ACCELERATION = 0.005;
const int32_t gPARTICLE_MASS = 10;
const double gMIN_DISTANCE = 5;

//----- Variables -----//
int32_t gHandCharge = 1;
uint32_t gLastUpdateTick = 0;
uint32_t gShiftPickedParticleX = 0;
uint32_t gShiftPickedParticleY = 0;
uint32_t gHandonParticle = 0;
uint32_t gDeltaTime = 0;
uint32_t gIsRunning = 1;

//----- SDL surfaces -----//
SDL_Surface * gPositiveParticule1 = NULL;
SDL_Surface * gPositiveParticule2 = NULL;
SDL_Surface * gPositiveParticule3 = NULL;
SDL_Surface * gNegativeParticule1 = NULL;
SDL_Surface * gNegativeParticule2 = NULL;
SDL_Surface * gNegativeParticule3 = NULL;

SDL_Surface * gPositiveHand1 = NULL;
SDL_Surface * gPositiveHand2 = NULL;
SDL_Surface * gPositiveHand3 = NULL;
SDL_Surface * gNegativeHand1 = NULL;
SDL_Surface * gNegativeHand2 = NULL;
SDL_Surface * gNegativeHand3 = NULL;
SDL_Surface * gOpenHand = NULL;
SDL_Surface * gCloseHand = NULL;

SDL_Surface * gMovingParticle1 = NULL;

SDL_Surface * gScreen = NULL;

SDL_Surface * gBackgroundImage = NULL;
SDL_Rect gPositionBackground;

SDL_Surface * gHand = NULL;
SDL_Rect gPositionHand;

//----- Creation of a Structure Particle -----//
typedef struct Particle Particle;
struct Particle
{
	bool moving;
	double xCoord;
	double yCoord;
	int32_t charge;
	double xSpeed;
	double ySpeed;
};

//----- Array of Particles -----//
Particle *gParticles[gMAX_PARTICLES];
int32_t gParticlesSize = 0;
int32_t gNumberOfMovingParticle = 0;

//----- Pointers -----//
Particle* gP = NULL;

//----- Functions -----//
void createParticle(bool, int32_t, int, int, double, double);
void rendering();
void init();
void destroy();
void eventManager();
void changeBackgroundDimension(int, int);
Particle* handOnParticle(const int, const int);
void destroyParticle(Particle*);
void movementCalculation();
double getSumForcesX(int32_t, int32_t, int32_t);
double getSumForcesY(int32_t, int32_t, int32_t);
void organizeParticleBlitering();
bool particleHitbox(Particle*, int, int);
void changeParticleCharge(Particle*, int);
void moveParticle();
void getShifting(Particle*);
void changeHandCharge(int);
double getAccelerationX(Particle*);
double getAccelerationY(Particle*);
double getDistance(Particle*, Particle*);
void ticks60fps();
void box(Particle*);
void clamp(double*, double);

int main(int argc, char *argv[])
{
	freopen("CON", "w", stderr);
	freopen("CON", "w", stdout);

	init();

	while (gIsRunning)
	{
		eventManager();
		movementCalculation();
		rendering();
		ticks60fps();
	}

	destroy();

	return EXIT_SUCCESS;
}

void createParticle(bool moving, int32_t charge, int x, int y, double vx, double vy)
{
	Particle *p = NULL;

	if (gParticlesSize < gMAX_PARTICLES)
	{
		p = (Particle*)malloc(sizeof(Particle));
		if (!p) return;

		p->moving = moving;
		p->charge = charge;
		p->xCoord = (double)x;
		p->yCoord = (double)y;
		p->xSpeed = vx;
		p->ySpeed = vy;

		gParticles[gParticlesSize] = p;
		gParticlesSize++;

		if (moving) gNumberOfMovingParticle++;
	}
}

void rendering()
{
	organizeParticleBlitering();

	SDL_FillRect(gScreen, NULL, SDL_MapRGB(gScreen->format, 255, 255, 255));
	SDL_BlitSurface(gBackgroundImage, NULL, gScreen, &gPositionBackground);

	//----- rendering particles -----//
	int i;
	SDL_Surface *particle = NULL;
	SDL_Rect positionParticle;

	for (i = 0; i<gParticlesSize; ++i)
	{
		positionParticle.x = (Sint16)((int)gParticles[i]->xCoord - gPositiveParticule1->w / 2);
		positionParticle.y = (Sint16)((int)gParticles[i]->yCoord - gPositiveParticule1->h / 2);
		if (gParticles[i]->moving)
		{
			switch (gParticles[i]->charge)
			{
			case 1:
				particle = gMovingParticle1;
				break;
			default:;
			}
		}
		else
		{
			switch (gParticles[i]->charge)
			{
			case -3:
				particle = gNegativeParticule3;
				break;
			case -2:
				particle = gNegativeParticule2;
				break;
			case -1:
				particle = gNegativeParticule1;
				break;
			case 1:
				particle = gPositiveParticule1;
				break;
			case 2:
				particle = gPositiveParticule2;
				break;
			case 3:
				particle = gPositiveParticule3;
				break;
			}
		}

		SDL_SetColorKey(particle, SDL_SRCCOLORKEY, SDL_MapRGB(particle->format, 255, 255, 255));
		SDL_BlitSurface(particle, NULL, gScreen, &positionParticle);
	}

	//----- rendering hand -----//

	if (!gHandonParticle)
	{
		switch (gHandCharge)
		{
		case -3:
			gHand = gNegativeHand3;
			break;
		case -2:
			gHand = gNegativeHand2;
			break;
		case -1:
			gHand = gNegativeHand1;
			break;
		case 1:
			gHand = gPositiveHand1;
			break;
		case 2:
			gHand = gPositiveHand2;
			break;
		case 3:
			gHand = gPositiveHand3;
			break;
		default:;
		}
	}
	else
	{
		if (!gP) gHand = gOpenHand;
		else gHand = gCloseHand;
	}
	SDL_SetColorKey(gHand, SDL_SRCCOLORKEY, SDL_MapRGB(gHand->format, 255, 255, 255));
	SDL_BlitSurface(gHand, NULL, gScreen, &gPositionHand);

	SDL_Flip(gScreen);
}

void init()
{
	//initialization of the particles array
	memset(gParticles, 0, gMAX_PARTICLES * sizeof(Particle*));

	//loading images
	gPositiveParticule1 = SDL_LoadBMP("positiveParticle1.bmp");
	if (gPositiveParticule1 == NULL)
	{
		printf("ERROR cant find positivite particle image\n");
	}
	gPositiveParticule2 = SDL_LoadBMP("positiveParticle2.bmp");
	gPositiveParticule3 = SDL_LoadBMP("positiveParticle3.bmp");
	gNegativeParticule1 = SDL_LoadBMP("negativeParticle1.bmp");
	gNegativeParticule2 = SDL_LoadBMP("negativeParticle2.bmp");
	gNegativeParticule3 = SDL_LoadBMP("negativeParticle3.bmp");

	gPositiveHand1 = SDL_LoadBMP("positiveHand1.bmp");
	gPositiveHand2 = SDL_LoadBMP("positiveHand2.bmp");
	gPositiveHand3 = SDL_LoadBMP("positiveHand3.bmp");
	gNegativeHand1 = SDL_LoadBMP("negativeHand1.bmp");
	gNegativeHand2 = SDL_LoadBMP("negativeHand2.bmp");
	gNegativeHand3 = SDL_LoadBMP("negativeHand3.bmp");
	gOpenHand = SDL_LoadBMP("OpenHand.bmp");
	gCloseHand = SDL_LoadBMP("CloseHand.bmp");

	gMovingParticle1 = SDL_LoadBMP("movingParticle.bmp");
	gBackgroundImage = SDL_LoadBMP("background.bmp");

	SDL_Init(SDL_INIT_VIDEO);   //load SDL video mode
	gScreen = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE); //choice of the video mode
	if (gScreen == NULL) //test of an error at the initialization of the SDL
	{
		fprintf(stderr, "can't charge SDL video mode : %s\n", SDL_GetError());  //writing the error
		exit(EXIT_FAILURE); //quit program
	}

	SDL_WM_SetCaption("Charge Game", NULL); //title of the window

	SDL_ShowCursor(SDL_DISABLE);
	SDL_WarpMouse((Uint16)(gScreen->w / 2), (Uint16)(gScreen->h / 2));

	SDL_EnableKeyRepeat(10, 10);

	gPositionBackground.x = 0;
	gPositionBackground.y = 0;

	gHand = gPositiveParticule1;

	createParticle(true, 1, 300, 200, 0, 0);
}

void destroy()
{
	int i;
	for (i = 0; i<gParticlesSize; ++i) free(gParticles[i]);

	SDL_FreeSurface(gBackgroundImage);
	SDL_FreeSurface(gPositiveParticule1);
	SDL_FreeSurface(gPositiveParticule2);
	SDL_FreeSurface(gPositiveParticule3);
	SDL_FreeSurface(gNegativeParticule1);
	SDL_FreeSurface(gNegativeParticule2);
	SDL_FreeSurface(gNegativeParticule3);
	SDL_FreeSurface(gMovingParticle1);
	SDL_FreeSurface(gPositiveHand1);
	SDL_FreeSurface(gPositiveHand2);
	SDL_FreeSurface(gPositiveHand3);
	SDL_FreeSurface(gNegativeHand1);
	SDL_FreeSurface(gNegativeHand2);
	SDL_FreeSurface(gNegativeHand3);
	SDL_FreeSurface(gOpenHand);
	SDL_FreeSurface(gCloseHand);
	SDL_Quit();
}

void eventManager()
{
	SDL_Event event;    //variable for the events
	SDL_PollEvent(&event);

	Particle* p = NULL;

	switch (event.type)
	{
		case SDL_QUIT:
		printf("sdl quit \n");
		printf("last error = %s \n", SDL_GetError());
		gIsRunning = 0;
		break;

	case SDL_KEYDOWN:
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			printf("escape \n");
			gIsRunning = 0;
			break;

		default:;
		}
		break;

	case SDL_MOUSEBUTTONDOWN:
		switch (event.button.button)
		{
		case SDL_BUTTON_LEFT:
			if (!((p = handOnParticle(event.button.x, event.button.y)))) createParticle(false, gHandCharge, event.button.x, event.button.y, 0, 0);
			else
			{
				getShifting(p);
				gP = p;
			}
			break;

		case SDL_BUTTON_RIGHT:
			if ((p = handOnParticle(event.button.x, event.button.y))) destroyParticle(p);
			break;

		case SDL_BUTTON_WHEELUP:
			if ((p = handOnParticle(event.button.x, event.button.y))) changeParticleCharge(p, 1);
			else changeHandCharge(1);
			break;

		case SDL_BUTTON_WHEELDOWN:
			if ((p = handOnParticle(event.button.x, event.button.y))) changeParticleCharge(p, -1);
			else changeHandCharge(-1);
			break;
		default:;
		}
		break;

	case SDL_MOUSEBUTTONUP:
		switch (event.button.button)
		{
		case SDL_BUTTON_LEFT:
			gP = NULL;
			break;
		default:;
		}
		break;

	case SDL_MOUSEMOTION:
		gPositionHand.x = (Sint16)event.motion.x - gHand->w / 2;
		gPositionHand.y = (Sint16)event.motion.y - gHand->h / 2;
		if (handOnParticle(event.motion.x, event.motion.y)) gHandonParticle = 1;
		else gHandonParticle = 0;
		break;

	case SDL_VIDEORESIZE:
		//changeBackgroundDimension(event.resize.w, event.resize.h);
		//changeParticlesCoordinates();
		break;
	default:;
	}
}

void changeBackgroundDimension(int w, int h)
{
	gPositionBackground.w = w;
	gPositionBackground.h = h;
}

Particle* handOnParticle(const int x, const int y)
{
	int i;

	for (i = 0; i<gParticlesSize; ++i)
	{
		if (gParticles[i] && particleHitbox(gParticles[i], x, y)) return gParticles[i];
	}
	return NULL;
}

void destroyParticle(Particle* p)
{
	int i;
	int j;

	for (i = 0; i<gParticlesSize; ++i)
	{
		if (gParticles[i] && p && !(gParticles[i]->moving) && gParticles[i] == p)
		{
			free(gParticles[i]);
			gParticles[i] = NULL;
			for (j = i; j<gParticlesSize; ++j)
			{
				gParticles[j] = gParticles[j + 1];
			}
			gParticlesSize--;
		}
	}
}

void organizeParticleBlitering()
{
	int i;
	int counterMovingParticles = 0;
	Particle* ptemp = NULL;

	for (i = 0; i<gParticlesSize - counterMovingParticles; ++i)
	{
		if (gParticles[i] && gParticles[i]->moving)
		{
			ptemp = gParticles[gParticlesSize - 1 - counterMovingParticles];
			gParticles[gParticlesSize - 1 - counterMovingParticles] = gParticles[i];
			gParticles[i] = ptemp;
			counterMovingParticles++;
		}
	}
}

bool particleHitbox(Particle* p, int x, int y)
{
	return(p && x > ((int)p->xCoord - gPositiveParticule1->w*0.5f)
		&& x < ((int)p->xCoord + gPositiveParticule1->w*0.5f)
		&& y >((int)p->yCoord - gPositiveParticule1->h*0.5f)
		&& y < ((int)p->yCoord + gPositiveParticule1->h*0.5f));
}

void changeParticleCharge(Particle* p, int c)
{
	if (p && !(p->moving))
	{
		if (c == 1 && p->charge < gMAX_PARTICULE_CHARGE)
		{
			if (p->charge == -1) p->charge = 1;
			else p->charge++;
		}
		else if (c == -1 && p->charge > gMIN_PARTICULE_CHARGE)
		{
			if (p->charge == 1) p->charge = -1;
			else p->charge--;
		}
	}
}

void changeHandCharge(int c)
{
	if (c == -1 && gHandCharge > gMIN_PARTICULE_CHARGE)
	{
		if (gHandCharge == 1) gHandCharge = -1;
		else gHandCharge--;
	}
	else if (c == 1 && gHandCharge < gMAX_PARTICULE_CHARGE)
	{
		if (gHandCharge == -1) gHandCharge = 1;
		else gHandCharge++;
	}
}

void moveParticle()
{
	if (gP)
	{
		gP->xCoord = (double)(gPositionHand.x + gShiftPickedParticleX);
		gP->yCoord = (double)(gPositionHand.y + gShiftPickedParticleY);
	}
}

void getShifting(Particle* p)
{
	gShiftPickedParticleX = (int)p->xCoord - gPositionHand.x;
	gShiftPickedParticleY = (int)p->yCoord - gPositionHand.y;
}

void ticks60fps()
{
	uint32_t ticks = SDL_GetTicks();
	printf("-----\n"
		"deltaltime = %d - tick = %d - lastupdatetick = %d \n", gDeltaTime, ticks, gLastUpdateTick);
	printf("tick - lastupdatetick = %d \n", ticks - gLastUpdateTick);
	if (ticks < gLastUpdateTick) return;
	if (ticks - gLastUpdateTick < 16)
	{
		SDL_Delay(16 - (ticks - gLastUpdateTick));
		gDeltaTime = 16;
	}
	else
	{
		gDeltaTime = ticks - gLastUpdateTick;
	}
	printf("deltatime = %d \n", gDeltaTime);
	gLastUpdateTick = ticks;
}

void movementCalculation()
{
	moveParticle();
	int i;

	for (i = gParticlesSize - gNumberOfMovingParticle; i<gParticlesSize; ++i)
	{
		if (gParticles[i])
		{
			int t = gDeltaTime;
			double xInit = gParticles[i]->xCoord;
			double yInit = gParticles[i]->yCoord;
			double x = xInit;
			double y = yInit;
			double xs = gParticles[i]->xSpeed;
			double ys = gParticles[i]->ySpeed;
			double xa = getAccelerationX(gParticles[i]);
			double ya = getAccelerationY(gParticles[i]);

			x += xs*t + 0.5*xa*t*t;
			y += ys*t + 0.5*ya*t*t;

			if (t != 0)
			{
				gParticles[i]->xSpeed = (x - xInit) / t;
				clamp(&(gParticles[i]->xSpeed), gMAX_SPEED);

				gParticles[i]->ySpeed = (y - yInit) / t;
				clamp(&(gParticles[i]->ySpeed), gMAX_SPEED);
			}

			gParticles[i]->xCoord = x;
			gParticles[i]->yCoord = y;

			box(gParticles[i]);

			printf("xs = %lf - ys = %lf - xa = %lf - ya = %lf \n", gParticles[i]->xSpeed, gParticles[i]->ySpeed, xa, ya);
		}
	}
}

double getAccelerationX(Particle* p)
{
	double sumForcesX = 0;
	int i;

	for (i = 0; i<gParticlesSize; ++i)
	{
		if (p != gParticles[i])
		{
			double d = getDistance(p, gParticles[i]);
			int c1 = p->charge;
			int c2 = gParticles[i]->charge;
			double r = (gParticles[i]->xCoord - p->xCoord) / d;

			sumForcesX += -((c1*c2) / (d*d))*r;
		}
	}

	double xa = sumForcesX * gPARTICLE_MASS;
	double* pxa = &xa;
	clamp(pxa, gMAX_ACCELERATION);

	return xa;
}

double getAccelerationY(Particle* p)
{
	double sumForcesY = 0;
	int i;

	for (i = 0; i<gParticlesSize; ++i)
	{
		if (p != gParticles[i])
		{
			double d = getDistance(p, gParticles[i]);
			int c1 = p->charge;
			int c2 = gParticles[i]->charge;
			double r = (gParticles[i]->yCoord - p->yCoord) / d;

			sumForcesY += -((c1*c2) / (d*d))*r;
		}
	}

	double ya = sumForcesY * gPARTICLE_MASS;
	double* pya = &ya;
	clamp(pya, gMAX_ACCELERATION);

	return ya;
}

void clamp(double* value, double limit)
{
	if (*value < -limit) *value = -limit;
	else if (*value > limit) *value = limit;
}

double getDistance(Particle* p1, Particle* p2)
{
	double dist = sqrt(pow(p1->xCoord - p2->xCoord, 2) + pow(p1->yCoord - p2->yCoord, 2));

	if (dist <= gMIN_DISTANCE) return gMIN_DISTANCE;
	return dist;
}

void box(Particle* p)
{
	if (p)
	{
		if (p->xCoord - gPositiveParticule1->w*0.5f <= 0 && p->xSpeed < 0)
		{
			p->xSpeed = 0;
			p->xCoord = 0 + gPositiveParticule1->w*0.5f;
		}
		if (p->xCoord + gPositiveParticule1->w*0.5f >= gScreen->w && p->xSpeed > 0)
		{
			p->xSpeed = 0;
			p->xCoord = gScreen->w - gPositiveParticule1->w*0.5f;
		}
		if (p->yCoord - gPositiveParticule1->h*0.5f <= 0 && p->ySpeed < 0)
		{
			p->ySpeed = 0;
			p->yCoord = 0 + gPositiveParticule1->h*0.5f;
		}
		if (p->yCoord + gPositiveParticule1->h*0.5f >= gScreen->h && p->ySpeed > 0)
		{
			p->ySpeed = 0;
			p->yCoord = gScreen->h - gPositiveParticule1->h*0.5f;
		}
	}
}


