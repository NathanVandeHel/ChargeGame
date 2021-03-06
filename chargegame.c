#include <stdlib.h>
#include <stdio.h>
#include <SDL.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <SDL_ttf.h>


/*----- CONSTANTS -----*/
/** \brief Number of particles the game can process*/
#define gMAX_PARTICLES 500

/** \brief Number of walls the game can process*/
#define gMAX_WALL 100

/** \brief Maximum charge of a particle. If changed, be sure to create the corresponding SDL_Surfaces and images*/
const int32_t gMAX_PARTICULE_CHARGE = 3;

/** \brief Minimum charge of a particle. If changed, be sure to create the corresponding SDL_Surfaces and images*/
const int32_t gMIN_PARTICULE_CHARGE = -3;

/** \brief Maximum speed of a moving particle. Used to clamp the particle speed in movementCalculation()*/
const double gMAX_SPEED = 0.5;

/** \brief Maximum acceleration of a moving particle. Used to clamp the particle acceleration ingetAcceleration()*/
const double gMAX_ACCELERATION = 0.005;

/** \brief Mass of a particle. Used in getAcceleration()*/
const int32_t gPARTICLE_MASS = 10;

/** \brief Minimum distance at which the force between two particles is calculated by : f = C1C2/d^2.
 * Below that limit, the calculation is done with the distance equals to this constaNT, in order to avoid infinites forces*/
const double gMIN_DISTANCE = 5;

/** \brief Time multiplier of the game. Used to modify the overall speed of the particles movement*/
const double gTIME_MULTIPLIER = 0.7;

/** \brief Number of level loadable by the game.
 *This limit is dicted by the number of level which can be displayed at once by renderLevelList()*/
const int32_t gMAX_LEVELS_LOADABLE = 42;


/*----- VARIABLES -----*/
/*---- State variables -----*/
/** \brief Variable of the software's main loop. If 0 : software stops*/
uint32_t gIsRunning = 1;

/** \brief State of the game. 1: play, 2: pause*/
uint32_t gPlay = 0;

/** \brief State of the level. 1: win, 2: in process*/
uint32_t gWin = 0;

/** \brief Drives the apprearance of the cursor in rendering(). Modified in eventManager()*/
int32_t gCursorState = 1;

/** \brief Notice if the player is in level creative mode or not*/
uint32_t gCreativeMode = 0;

/** \brief Id of the level currently loaded*/
uint32_t gLevel = 0;

/** \brief High scores of the level currently loaded*/
int32_t gLevelHighScores[3] = {0, 0, 0};

/** \brief Notice if the player is creating a wall*/
uint32_t gCreationWall = 0;

/** \brief Notice if the player is creating a goal*/
uint32_t gCreationGoal = 0;

/*---- Counter variables ----*/
/** \brief Overall number of particles present in the game*/
int32_t gNumberOfParticle = 0;

/** \brief Number of moving particles present in the game*/
int32_t gNumberOfMovingParticle = 0;

/** \brief Number of particles included in the loaded level and that can't be modified by the user in game*/
uint32_t gNumberOfparticlesInTheLevel = 0;

/** \brief Number of moving particles already in the goal*/
int32_t gNumberOfParticlesOnGoal = 0;

/** \brief Number of walls present in the game*/
int32_t gNumberOfWall = 0;

/** \brief Number of levelfiles found in the game directory by scanLevels().
* Clamped by the constant gMAX_LEVELS_LOADABLE*/
int32_t gNumberOfLevels = 0;

/*---- Other variables ----*/
/** \brief Used in the computation of particle collisions, hitbox, etc.
 * Involve that all the particles have the same size.
 * Involve that the SDL surfaces of the particles are square surfaces*/
uint32_t gParticleRadius = 0;

/** \brief Store the time of the previous clock tick. Used in ticks60fps to obtaint gDeltaTime*/
uint32_t gLastUpdateTick = 0;

/** \brief Time between two clock ticks. Modified in ticks60fps(). Used int movementCalculation*/
uint32_t gDeltaTime = 0;

/** \brief X distance between the position of the cursor and the center of a particle when this particle is being picked.
 *Modified in getShifting(). Used in moveParticle()*/
uint32_t gShiftPickedParticleX = 0;

/** \brief Y distance between the position of the cursor and the center of a particle when this particle is being picked.
*Modified in getShifting(). Used in moveParticle()*/
uint32_t gShiftPickedParticleY = 0;

/** \brief Store the position of two points when the player creates a surface (goal or wall) in drag and drop
 * The first point stores the coordinates at which the player starts to draw the surface
 * The second point stores the coordinates of the cursor while the player keep drawing the surface*/
int gp1x = 0, gp1y = 0, gp2x = 0, gp2y = 0;


/*----- STRUCTURES -----*/
/** \brief Struture of a particle
 * \param moving : states if the particle is a moving particle or not
 * \param modifiable : states if a particle can be modified by the player or not when in normal game
 * \param xCoord : x coordinate. Stored as a double for smoother movements
 * \param yCoord : y coordinate. Stored as a double for smoother movements
 * \param xCoordInit : initial x coordinate. Used to reset the particle position when reseting the level
 * \param yCoordInit : initial y coordinate. Used to reset the particle position when reseting the level
 * \param charge : charge of the particle. Varies between gMIN_PARTICLE_CHARGE and gMAX_PARTICLE_CHARGE
 * \param xSpeed : x speed
 * \param ySpeed : y speed
 * \param goal : states if the particle is on the goal or not (used only for moving particles)*/
typedef struct Particle
{
	bool moving;
	bool modifiable;
	double xCoord;
	double yCoord;
	double xCoordInit;
	double yCoordInit;
	int32_t charge;
	double xSpeed;
	double ySpeed;
	bool goal;
} Particle;


/** \brief Structure of a wall. Managed as a SDL surface
 * \param x : x coordinate of the uper left wedge of the wall
 * \param y : y coordinate of the uper left wedge of the wall
 * \param w : x length of the wall
 * \param h	: y lenght of the wall*/
typedef struct Wall
{
	int x;
	int y;
	int w;
	int h;
} Wall;


/*----- ARRAYS -----*/
Particle* gParticleArray[gMAX_PARTICLES];
Wall* gWallArray[gMAX_WALL];


/*----- ENUMERATIONS -----*/
/** \brief internal states of the program*/
typedef enum EProgramState
{
	EProgramState_GAME,
	EProgramState_MENU,
	EProgramState_LEVEL_CHOICE,
	EProgramState_CREATE_LEVEL,
	EProgramState_HELP
} EProgramState;

EProgramState gGameState = EProgramState_MENU; //Initial state of the program : menu

/** \brief denomination of all surface buttons of the program (used for easier lisibility)*/
typedef enum EButton
{
	EButton_UNKNOWN,
	EButton_MENU_PLAY_GAME,
	EButton_MENU_CUSTOM_LEVEL,
	EButton_MENU_HELP,
	EButton_MENU_QUIT,
	EButton_MENU_CUSTOM_LEVEL_MODIFY,
	EButton_MENU_CUSTOM_LEVEL_NEW,
	EButton_FOOTER_MENU,
	EButton_FOOTER_PLAY_RESTART,
	EButton_FOOTER_SAVE
} EButton;


/*----- POINTERS -----*/
/*---- Pointers to SDL surfaces ----*/
/*--- Particle surfaces ---*/
SDL_Surface* gMovingParticle = NULL;
SDL_Surface* gPositiveParticule1 = NULL;
SDL_Surface* gPositiveParticule2 = NULL;
SDL_Surface* gPositiveParticule3 = NULL;
SDL_Surface* gNegativeParticule1 = NULL;
SDL_Surface* gNegativeParticule2 = NULL;
SDL_Surface* gNegativeParticule3 = NULL;

/*--- Cursor surfaces ---*/
SDL_Surface* gCursor = NULL;
SDL_Rect gPositionCursor;
SDL_Surface* gPositiveHand1 = NULL;
SDL_Surface* gPositiveHand2 = NULL;
SDL_Surface* gPositiveHand3 = NULL;
SDL_Surface* gNegativeHand1 = NULL;
SDL_Surface* gNegativeHand2 = NULL;
SDL_Surface* gNegativeHand3 = NULL;
SDL_Surface* gOpenHand = NULL;
SDL_Surface* gCloseHand = NULL;
SDL_Surface* gDeniedHand = NULL;
SDL_Surface* gPointingHand = NULL;
SDL_Surface* gMovingParticleHand = NULL;
SDL_Surface* gGoalHand = NULL;
SDL_Surface* gWallHand = NULL;

/*--- Button surfaces ---*/
SDL_Surface* gPlayButton = NULL;
SDL_Surface* gRestartButton = NULL;
SDL_Rect gPositionPlayRestartButton;
SDL_Surface* gMenuButton = NULL;
SDL_Rect gPositionMenuButton;
SDL_Surface* gSaveButton = NULL;
SDL_Rect gPositionSaveButton;

/*--- Menu surface ---*/
SDL_Surface* gMenuPlayGame = NULL;
SDL_Rect gPositionMenuPlayGame;
SDL_Surface* gMenuCustomLevel = NULL;
SDL_Rect gPositionMenuCustomLevel;
SDL_Surface* gMenuHelp = NULL;
SDL_Rect gPositionMenuHelp;
SDL_Surface* gMenuQuit = NULL;
SDL_Rect gPositionMenuQuit;
SDL_Surface* gMenuCustomLevelModify = NULL;
SDL_Rect gPositionMenuCustomLevelModify;
SDL_Surface* gMenuCustomLevelNew = NULL;
SDL_Rect gPositionMenuCustomLevelNew;

/*--- Other surfaces ---*/
SDL_Surface* gScreen = NULL;
SDL_Surface* gBackground = NULL;
SDL_Rect gPositionBackground;
SDL_Surface* gGoal = NULL;
SDL_Rect gPositionGoal;
SDL_Surface* gScoresDisplay = NULL;
SDL_Rect gPositionScoresDisplay;
SDL_Surface* gRules = NULL;
SDL_Rect gPositionRules;

/*---- Pointers to fonts ----*/
TTF_Font* gFont = NULL;
TTF_Font* gFontMenu = NULL;
SDL_Color gFontColor = {200, 210, 220};

/*---- Other pointers ----*/
/**\brief Pointer to the particle which is being moved (draged) by the player*/
Particle* gDragedParticle = NULL;

/** \brief Pointer to the particle pointed by the cursor*/
Particle* gPointedParticle = NULL;


/*----- FUNCTION PROTOTYPES -----*/
/** \brief initialize all the necessary steps for the software : load images, set positions etc.*/
void init(void);

/** \brief Manage all the user entries and select the actions according to the program state*/
void eventManager(void);

/** \brief Compute the movements of the  moving particles*/
void movementCalculation(void);

/** \brief Render the programm in function of its state.
 * three main things to render : the playable area content (particles, walls, text buttons...), the footer (scores, buttons...) and the cursor*/
void rendering(void);

/** \brief Ensure that the program runs smoothly at 60 fps*/
void ticks60fps(void);

/** \brief Handle left mouse button down event when the program is in game state
* \param event : event of the left mouse button down*/
inline void handleLBMDInGameState(const SDL_Event event);

/** \brief Handle left mouse button down event when the program is in menu state
* \param event : event of the left mouse button down*/
inline void handleLBMDInMenuState(const SDL_Event event);

/** \brief Handle left mouse button down event when the program is in level choice state
* \param event : event of the left mouse button down*/
inline void handleLBMDInLevelChoiceState(const SDL_Event event);

/** \brief Handle left mouse button down event when the program is increate level state
* \param event : event of the left mouse button down*/
inline void handleLBMDInCreateLevelState(const SDL_Event event);

/** \brief Render particles when the program is in game state*/
inline void renderParticlesInGameState(void);

/** \brief Render the cursor when the program is in game state*/
inline void renderCursorInGameState(void);

/** \brief Render the footer when the program is in game state*/
inline void renderFooterInGameState(void);

/** \brief Free all the dynamicaly allocated data defore closing the program*/
void destroy(void);

/** \brief Allocate dynamicaly a particle structure and store a pointer to this struture in the particle array
 * \param moving : state of the particle, see particle structure
 * \param modifiable : state of the particle, see particle structure
 * \param charge : charge of the particle
 * \param x : x coordinate of the particle
 * \param y : y coordinate of the particle*/
void createParticle(bool moving, bool modifiable, const int32_t charge, const int x, const int y);

/** \brief Allocate dynamically a wall structure and store a pointer to this structure in the wall array
 * \param x : wall parameter, see wall structure
 * \param y : wall parameter, see wall structure
 * \param w : wall parameter, see wall structure
 * \param h : wall parameter, see wall structure*/
void createWall(const int x, const int y, const int w, const int h);

/** \brief Create a goal, fill the SDL_Rect gPositionGoal with the input parameters
 * \param x : see SDL_Rect
 * \param y : see SDL_Rect
 * \param w : see SDL_Rect
 * \param h : see SDL_Rect*/
void createGoal(const int32_t x, const int32_t y, const int32_t w, const int32_t h);

/** \brief Free the particle pointed by the input parameter. Reorganise the particle array
 * \param p : pointer to particle to destroy*/
void destroyParticle(Particle* p);

/** \brief Free th wall pointed by the input parameter. Reorganise the wall array
 * \param w : pointer to wall to destroy*/
void destroyWall(Wall* w);

/** \brief Free SDl surface gGoal. Set SDL_Rect gPositionGoal parameters to 0*/
void destroyGoal(void);

/** \brief Set the position of the draged particle according to the cursor position.
 * The particle has to be modifiable in order to be draged*/
void moveParticle(void);

/** \brief Get the difference of position between the cursor and the center of a particle passed as an input parameter.
* This difference is stored in gShiftPickedParticleX and gShiftPickedParticleY.
* \param p : pointer to the particle to get the shift*/
void getShifting(Particle* p);

/** \brief Get the x and y acceleration of a particle according to the other particles present in the game
* \param p : pointer to the particle to get the acceleration
* \param ax : pointer to output parameter x acceleration
* \param ay : pointer to output parameter y acceleration*/
void getAcceleration(Particle* p, double* ax, double* ay);

/** \brief Clamp the abolute value of a double passed by a pointer by a double limit
* \param value : pointer to value to clamp
* \param limit : limit of the clamp*/
void clamp(double* value, const double limit);

/** \brief return the distance between two particles passed as input parameters
 * \param p1 : pointer to first particle
 * \param p2 : pointer to second particle
 * \return : distance between the two particles*/
double getDistance(Particle* p1, Particle* p2);

/** \brief Test if a particle is within the limits of the goal.
* If a particle is on the goal, modify the goal parameter of this particle and iterate gNumberOfParticlesOnGoal.
* If all the moving particles are on goal, call levelFinished();
* \param p : pointer to particle to test*/
void onGoal(Particle* p);

/** \brief organize the particle array in function of the y coordinates of the particles.
* Firt, get all the moving particles at the end of the array with a bubble sorting algorithm.
* Next, sort the moving particles in function of their y coordinate with a bubble sorting algorithm.
* Finally, sort the non moving particles in function of their y coordinate with a bubble sorting algorithm.
* This sorting system allows the particles with a greater y coordinate to be blit on top of the one with a smaller y coordiante.
* Sorting the moving and the non moving particles allows the moving particles to be always blit on top of the non moving ones*/
void organizeParticleBlitering(void);

/** \brief Render the walls of the wall array by creating a SDL_Surface for each wall, blit the SDL_Surface and then free it
 * Also render the surface drawn by the player if he is creating a wall*/
void renderWall(void);

/** \brief Render the goal by bliting its SDL surface gGoal.
* Also render the surface drawn by the player if he is creating a new goal*/
void renderGoal(void);

/** \brief Render a list of all the levels available.
* Create a SDL surface for each level, fill it with TTF, blit it and then free it*/
void renderLevelList(void);

/** \brief Fill a string passed as input parameter with the high scores of the level played and the actual score of the player.
* \param txt : string to fill*/
void createScoreText(char* txt);

/** \brief Fill a string passed as input parameter to tell the player he wins the level, with his score.
* \param txt : string to fill*/
void createEndLevelText(char* txt);

/** \brief Read the level file corresponding to the integer input parameter.
* Load in memory all the particles, walls, goal and high scores of the level.
* \param n : level to load*/
void loadLevel(const int n);

/** \brief Write in the level file corresponding to the integer input parameter.
* If called when in normal game mode, save the high scores and the particles, goal and walls already present in the level.
* the particles created by the player will not be saved.
* If called when in creation mode, save all the particles, walls and gaol.  the high scores are reset.
* \param n : level to save*/
void saveLevel(const int n);

/** \brief Get the moving particles coordinates to their initial coordinates.
*Set the moving particles speed to 0.
*Reset the gNumberOfparticlesOnGoal to 0*/
void reinitLevel(void);

/** \brief Try to read all the level files in the .exe directory. Count the number of level file readed.
* Warning : can only read level files in increasing order, from 1, without steps greater than 1.*/
void scanLevels(void);

/** \brief Free the particle array and the wall array entirely, without calling detroyParticle() or destroyWall().
* Also call freeGoal(). Also reset counting constants and high scores to 0*/
void freeLevel(void);

/** \brief Set the game state to win. If a new record is set by the player, call saveLevel() to save the new high score*/
void levelFinished(void);

/** \brief play game if game in pause. Pause and reset game if game in play*/
void levelPlayRestart(void);

/** \brief Fill a string passed as input parameter with the name of the level. Add ".txt" if input paremeter addExtension is true.
* \param fileName : String to fill
* \param n : Level number
* \param addExtension : if true, add ".txt" at the end of the string*/
void getLevelFileName(char* fileName, const int n, bool addExtension);

/** \brief Modify the high score array if the input parameter newScore is smaller than one of the current high score
* \param newScore : player score
* \return : if true : high scores updated*/
bool updateHighScores(const int newScore);

/** \brief Test for a particle passed in parameter collisions with walls, goal and the limits the the playable area.
* If a particle touch a limit, set it's coordinate and speed appropriately.
* \param p : pointer to particle to test*/
void collisions(Particle* p);

/** \brief test if the position passed in parameter (x, y) i on a surface passed in parameter.
* The surface to test can be extend by a certain ammount in the four directions (x positive, x negative, y positive, y negative)
* Those extensions are useful to take in acccount the particle radius for example.
* \param S : SDL_Rect to test
* \param x : x coordinate of the position
* \param y : y coordinate of the position
* \param xNegShift : extend at the left of the surface
* \param xPosShift : extend at the right of the surface
* \param yNegShift : extend on top of the surface
* \param yPosShift : extend on the bottom of the surface
* \return : true if the position is within the surface*/
bool surfaceHitbox(SDL_Rect S, const double x, const double y, const int xNegShift, const int xPosShift,
                   const int yNegShift, const int yPosShift);

/** \brief Get the four parameters x, y, w and h for a SDL_Rect in function of the two global points coordinates gp1x, gp1y, gp2x and gp2y.
* \param x : see SDL_Rect
* \param y : see SDL_Rect
* \param w : see SDL_Rect
* \param h : see SDL_Rect*/
void getPositionSurface(short* x, short* y, unsigned short* w, unsigned short* h);

/** \brief Test if the position passed as input parameter is on a particle.
* The particle array is browe in reverse, in order to select the particle with the greatest y coordinate if two particles are overlaped.
* The particle array should be sorted before calling this function.
* The particle hitbox is a circle defined by gParticleRadius
* \param x : x cordinate
* \param y : y coordinate
* \return pointer to the particle which the input position is on. Return NULL if the position is not on a particle*/
Particle* handOnParticle(const int x, const int y);

/** \brief Test if the y coordinate passed as input parameter is on the footer.
* \param y : y coordinate
* \return true if the y coodinate is on the footer*/
bool handOnFooter(const int y);

/** \brief Test if the position passed as input parameter is on a wall.
* \param x : x coordinate
* \param y : y coordinate
* \return pointer to the wall which the input position is on? Return NULL if the position is not on a wall*/
Wall* handOnWall(const int x, const int y);

/** \brief Manage the surface button according to the program state. Test if the position passed as input parameter is on a surface button.
* \param x : x coordinate
* \param y : y coordinate
* \return Ebutton corresponding to the surface button the position is on. Return EButton_UNKNOWN (default enumeration) if the position is not on a surface button*/
EButton surfaceButtonManager(const int x, const int y);

/** \brief Manage the surface level list. Test if the position passed as input parameter is on a surface level.
* \param x : x coordinate
* \param y : y coordinate
* \return integer corresponding to the level number the position is on. Return 0 if the psition is not on a surface level*/
int surfaceLevelListManager(const int x, const int y);

/** \brief Allow the player to draw a wall in drag and drop.
* If call when gCreationWall == 0, initialise gp1 and gp2 coordinates with the position passed as input parameter.
* The function also set gCreationWall to 1, which allows the posision of gp2 to be updated when the player move the mouse in eventManager().
* If call when gCreationWall == 1, Create a wall with the positions of gp1 ad gp2 and reset gCreationWall to 0.
* \param x : x coordinate
* \param y : y coordinate*/
void drawWall(int x, int y);

/** \brief Allow the player to draw a goal in drag and drop.
* If call when gCreationGoal == 0, initialise gp1 and gp2 coordinates with the position passed as input parameter.
* The function also set gCreationGoal to 1, which allows the posision of gp2 to be updated when the player move the mouse in eventManager().
* If call when gCreationGoal == 1, Create a goal with the positions of gp1 ad gp2 and reset gCreationGoal to 0.
* \param x : x coordinate
* \param y : y coordinate*/
void drawGoal(int x, int y);

/** \brief Change the charge of a particle according to the input parameter.
* \param p : pointer to particle to change charge
* \param c : 1 or -1 only, increase or decrease the charge by one only*/
void changeParticleCharge(Particle* p, const int c);

/** \brief Change the state of the cursor according to the input parameter
* \param c : 1 or -1 only, increase or decrease the charge by one only*/
void changeCursorState(const int c);


/*----- MAIN -----*/
int main(int argc, char* argv[])
{
	/*Allows the software to print messages in the console*/
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

#ifdef WIN32
	system("pause");
#endif

	return EXIT_SUCCESS;
}


/*----- MAIN LOOP FUNCTIONS -----*/
void init(void)
{
	/*Initialize the arrays*/
	memset(gParticleArray, 0, gMAX_PARTICLES * sizeof(Particle*));
	memset(gWallArray, 0, gMAX_PARTICLES * sizeof(Wall*));

	/*Load images*/
	gBackground = SDL_LoadBMP("background.bmp");
	if (gBackground == NULL) printf("ERROR : can't find background image \n");

	gMovingParticle = SDL_LoadBMP("movingParticle.bmp");
	if (gMovingParticle == NULL) printf("ERROR : can't find moving particle 1 image \n");
	gPositiveParticule1 = SDL_LoadBMP("positiveParticle1.bmp");
	if (gPositiveParticule1 == NULL) printf("ERROR : can't find positivite particle 1 image \n");
	gPositiveParticule2 = SDL_LoadBMP("positiveParticle2.bmp");
	if (gPositiveParticule2 == NULL) printf("ERROR : can't find positivite particle 2 image \n");
	gPositiveParticule3 = SDL_LoadBMP("positiveParticle3.bmp");
	if (gPositiveParticule3 == NULL) printf("ERROR : can't find positivite particle 3 image \n");
	gNegativeParticule1 = SDL_LoadBMP("negativeParticle1.bmp");
	if (gNegativeParticule1 == NULL) printf("ERROR : can't find negative particle 1 image \n");
	gNegativeParticule2 = SDL_LoadBMP("negativeParticle2.bmp");
	if (gNegativeParticule2 == NULL) printf("ERROR : can't find negative particle 2 image \n");
	gNegativeParticule3 = SDL_LoadBMP("negativeParticle3.bmp");
	if (gNegativeParticule3 == NULL) printf("ERROR : can't find negative particle 3 image \n");

	gPositiveHand1 = SDL_LoadBMP("positiveHand1.bmp");
	if (gPositiveHand1 == NULL) printf("ERROR : can't find positive hand 1 image \n");
	gPositiveHand2 = SDL_LoadBMP("positiveHand2.bmp");
	if (gPositiveHand2 == NULL) printf("ERROR : can't find positive hand 2 image \n");
	gPositiveHand3 = SDL_LoadBMP("positiveHand3.bmp");
	if (gPositiveHand3 == NULL) printf("ERROR : can't find positive hand 3 image \n");
	gNegativeHand1 = SDL_LoadBMP("negativeHand1.bmp");
	if (gNegativeHand1 == NULL) printf("ERROR : can't find negative hand 1 image \n");
	gNegativeHand2 = SDL_LoadBMP("negativeHand2.bmp");
	if (gNegativeHand2 == NULL) printf("ERROR : can't find negative hand 2 image \n");
	gNegativeHand3 = SDL_LoadBMP("negativeHand3.bmp");
	if (gNegativeHand3 == NULL) printf("ERROR : can't find negative hand 3 image \n");
	gOpenHand = SDL_LoadBMP("OpenHand.bmp");
	if (gOpenHand == NULL) printf("ERROR : can't find nopen hand image \n");
	gCloseHand = SDL_LoadBMP("CloseHand.bmp");
	if (gCloseHand == NULL) printf("ERROR : can't find close hand image \n");
	gDeniedHand = SDL_LoadBMP("DeniedHand.bmp");
	if (gDeniedHand == NULL) printf("ERROR : can't find denied hand image \n");
	gPointingHand = SDL_LoadBMP("PointingHand.bmp");
	if (gPointingHand == NULL) printf("ERROR : can't find pointing hand image \n");
	gGoalHand = SDL_LoadBMP("GoalHand.bmp");
	if (gGoalHand == NULL) printf("ERROR : can't find goal hand image \n");
	gWallHand = SDL_LoadBMP("WallHand.bmp");
	if (gWallHand == NULL) printf("ERROR : can't find wall hand image \n");
	gMovingParticleHand = SDL_LoadBMP("MovingParticleHand.bmp");
	if (gMovingParticleHand == NULL) printf("ERROR : can't find moving particle hand image \n");

	gPlayButton = SDL_LoadBMP("PlayButton.bmp");
	if (gPlayButton == NULL) printf("ERROR : can't find play button image \n");
	gRestartButton = SDL_LoadBMP("RestartButton.bmp");
	if (gRestartButton == NULL) printf("ERROR : can't find restart button image \n");
	gMenuButton = SDL_LoadBMP("MenuButton.bmp");
	if (gMenuButton == NULL) printf("ERROR : can't find menu button image \n");
	gSaveButton = SDL_LoadBMP("SaveButton.bmp");
	if (gSaveButton == NULL) printf("ERROR : can't find save button image \n");

	gRules = SDL_LoadBMP("Rules.bmp");
	if (gRules == NULL) printf("ERROR : can't find rules image \n");

	/*Initialize SDL*/
	SDL_Init(SDL_INIT_VIDEO); //Load SDL video mode
	gScreen = SDL_SetVideoMode(640, 520, 32, SDL_HWSURFACE | SDL_DOUBLEBUF); //Choice of the video mode
	if (gScreen == NULL) //Test of an error at the initialization of the SDL
	{
		printf("ERROR : can't charge SDL video mode \n");
		exit(EXIT_FAILURE);
	}

	/*Initialize TTF*/
	if (TTF_Init() == -1) //Test of an error et the initialization of TTF
	{
		printf("ERROR : can't initialise TTF \n");
		exit(EXIT_FAILURE);
	}

	/*Initialize fonts*/
	gFont = TTF_OpenFont("arial.ttf", 20);
	TTF_SetFontStyle(gFont, TTF_STYLE_BOLD);

	gFontMenu = TTF_OpenFont("arial.ttf", 30);
	TTF_SetFontStyle(gFontMenu, TTF_STYLE_BOLD);

	/*Initialize TTF surfaces*/
	gMenuPlayGame = TTF_RenderText_Blended(gFontMenu, "Play Game", gFontColor);
	gMenuCustomLevel = TTF_RenderText_Blended(gFontMenu, "Custom Level", gFontColor);
	gMenuHelp = TTF_RenderText_Blended(gFontMenu, "Help", gFontColor);
	gMenuQuit = TTF_RenderText_Blended(gFontMenu, "Quit", gFontColor);
	gMenuCustomLevelModify = TTF_RenderText_Blended(gFontMenu, "Modify existing level", gFontColor);
	gMenuCustomLevelNew = TTF_RenderText_Blended(gFontMenu, "New level", gFontColor);

	/*Initialize positions of SDl surfaces*/
	gPositionBackground.x = 0;
	gPositionBackground.y = 0;

	gPositionMenuPlayGame.x = (Sint16)(640 * 0.5 - gMenuPlayGame->w * 0.5);
	gPositionMenuPlayGame.y = 100;
	gPositionMenuCustomLevel.x = (Sint16)(640 * 0.5 - gMenuCustomLevel->w * 0.5);
	gPositionMenuCustomLevel.y = 150;
	gPositionMenuHelp.x = (Sint16)(640 * 0.5 - gMenuHelp->w * 0.5);
	gPositionMenuHelp.y = 200;
	gPositionMenuQuit.x = (Sint16)(640 * 0.5 - gMenuQuit->w * 0.5);
	gPositionMenuQuit.y = 250;
	gPositionMenuCustomLevelModify.x = (Sint16)(640 * 0.5 - gMenuCustomLevelModify->w * 0.5);
	gPositionMenuCustomLevelModify.y = 150;
	gPositionMenuCustomLevelNew.x = (Sint16)(640 * 0.5 - gMenuCustomLevelNew->w * 0.5);
	gPositionMenuCustomLevelNew.y = 200;

	gPositionScoresDisplay.x = 10;
	gPositionScoresDisplay.y = 487;
	gPositionPlayRestartButton.x = 570;
	gPositionPlayRestartButton.y = 489;
	gPositionMenuButton.x = 610;
	gPositionMenuButton.y = 489;
	gPositionSaveButton.x = 530;
	gPositionSaveButton.y = 489;

	gPositionRules.x = (Sint16)(640 * 0.5 - gRules->w * 0.5);
	gPositionRules.y = (Sint16)(480 * 0.5 - gRules->h * 0.5);

	/*Other initialisation steps*/
	SDL_WM_SetCaption("Charge Game", NULL); //Title of the window

	SDL_ShowCursor(SDL_DISABLE); //Disable Windows cursor appearance 
	SDL_WarpMouse((Uint16)(gScreen->w / 2), (Uint16)(gScreen->h / 2)); //Set cursor the the center of the window
	gCursor = gPointingHand; //Set default cursor appearance

	gParticleRadius = (int)(gPositiveParticule1->w * 0.5); //Set the particle radius
}

void eventManager(void)
{
	SDL_Event event;
	SDL_PollEvent(&event);

	Particle* pTemp = NULL;
	Wall* wTemp = NULL;

	switch (event.type)
	{
	case SDL_KEYDOWN:
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			switch (gGameState)
			{
			case EProgramState_GAME:
				freeLevel();
				gGameState = EProgramState_MENU;
				break;

			case EProgramState_MENU:
				gIsRunning = 0;
				break;

			case EProgramState_LEVEL_CHOICE:
				/*If the player was choosing a level to modify it (gCreativeMode = 1), return to create level state
				If the player was choosing a level to play on it (gCreativeMode = 0), return to menu state*/
				gGameState = gCreativeMode ? EProgramState_CREATE_LEVEL : EProgramState_MENU;
				break;

			case EProgramState_CREATE_LEVEL:
				gGameState = EProgramState_MENU;
				gCreativeMode = 0;
				break;

			case EProgramState_HELP:
				gGameState = EProgramState_MENU;
				break;

			default: ;
			}
			break;

		case SDLK_SPACE:
			if (gGameState == EProgramState_GAME) levelPlayRestart();
			break;

		case SDLK_s:
			if (gGameState == EProgramState_GAME && gCreativeMode) saveLevel(gLevel);
			break;

		default: ;
		}
		break;

	case SDL_MOUSEBUTTONDOWN:
		switch (event.button.button)
		{
		case SDL_BUTTON_LEFT:
			switch (gGameState)
			{
			case EProgramState_GAME:
				handleLBMDInGameState(event);
				break;

			case EProgramState_MENU:
				handleLBMDInMenuState(event);
				break;

			case EProgramState_LEVEL_CHOICE:
				handleLBMDInLevelChoiceState(event);
				break;

			case EProgramState_CREATE_LEVEL:
				handleLBMDInCreateLevelState(event);
				break;

			case EProgramState_HELP:
				{
					const EButton button = surfaceButtonManager(event.button.x, event.button.y);
					if (button == EButton_FOOTER_MENU) gGameState = EProgramState_MENU; //Left click on menu button
				}
				break;

			default: ;
			}
			break;

		case SDL_BUTTON_RIGHT:
			if (gGameState == EProgramState_GAME) //If in game
			{
				pTemp = handOnParticle(event.button.x, event.button.y);
				wTemp = handOnWall(event.button.x, event.button.y);

				if (!gPlay) //If game in pause
				{
					if (pTemp) destroyParticle(pTemp); //Delete particle

					if (gCreativeMode) //Delete wall and goal only on creative mode
					{
						if (wTemp) destroyWall(wTemp);
						if (surfaceHitbox(gPositionGoal, event.button.x, event.button.y, 0, 0, 0, 0)) destroyGoal();
					}
				}
			}
			break;

		case SDL_BUTTON_WHEELUP:
			if (gGameState == EProgramState_GAME) //If in game
			{
				pTemp = handOnParticle(event.button.x, event.button.y);

				if (!gPlay) //If game in pause
				{
					if (pTemp) changeParticleCharge(pTemp, 1); //If cursor on particle, change particle charge
					else //Else change cursor state
					{
						changeCursorState(1);
						/*Get new position for cursor surface*/
						gPositionCursor.x = (Sint16)(event.motion.x - gCursor->w / 2);
						gPositionCursor.y = (Sint16)(event.motion.y - gCursor->h / 2);
					}
				}
			}
			break;

		case SDL_BUTTON_WHEELDOWN:
			if (gGameState == EProgramState_GAME) //If in game
			{
				pTemp = handOnParticle(event.button.x, event.button.y);

				if (!gPlay) //If game in pause
				{
					if (pTemp) changeParticleCharge(pTemp, -1); //If cursor on particle, change particle charge
					else //Else change cursor state
					{
						changeCursorState(-1);
						/*Get new position for cursor surface*/
						gPositionCursor.x = (Sint16)(event.motion.x - gCursor->w / 2);
						gPositionCursor.y = (Sint16)(event.motion.y - gCursor->h / 2);
					}
				}
			}
			break;

		default: ;
		}
		break;

	case SDL_MOUSEBUTTONUP:
		switch (event.button.button)
		{
		case SDL_BUTTON_LEFT:
			if (gGameState == EProgramState_GAME) //If in game
			{
				gDragedParticle = NULL; //If the player was moving a particle, stop moving this particle
				if (gCreationWall) drawWall(event.button.x, event.button.y); //If the player was drawing a wall, create this wall
				if (gCreationGoal) drawGoal(event.button.x, event.button.y); //If the player was drawing a goal, create this goal
			}
			break;

		default: ;
		}
		break;

	case SDL_MOUSEMOTION:
		/*Get the new position of the cursor*/
		gPositionCursor.x = (Sint16)(event.motion.x - gCursor->w * 0.5);
		gPositionCursor.y = (Sint16)(event.motion.y - gCursor->h * 0.5);

		gPointedParticle = handOnParticle(event.motion.x, event.motion.y);

		/*If the player is creating a wall or a goal, get the new position of the second point*/
		if (gCreationWall || gCreationGoal)
		{
			gp2x = gPositionCursor.x + gCursor->w / 2;
			if (handOnFooter(event.motion.y)) gp2y = gPositionBackground.h;
				//A wall or a goal can't be created on the footer
			else gp2y = gPositionCursor.y + gCursor->h / 2;
		}
		break;

	case SDL_QUIT:
		gIsRunning = 0;
		break;

	default: ;
	}
}

void movementCalculation(void)
{
	int i;

	moveParticle(); //Move the non moving particle picked by the player

	if (gPlay)
	{
		for (i = gNumberOfParticle - gNumberOfMovingParticle; i < gNumberOfParticle; ++i)
		{
			if (gParticleArray[i] && !(gParticleArray[i]->goal))
			{
				const double t = gDeltaTime * gTIME_MULTIPLIER;
				const double xInit = gParticleArray[i]->xCoord;
				const double yInit = gParticleArray[i]->yCoord;
				double x = xInit;
				double y = yInit;
				const double xs = gParticleArray[i]->xSpeed;
				const double ys = gParticleArray[i]->ySpeed;
				double xa, ya;
				getAcceleration(gParticleArray[i], &xa, &ya);

				/*Compute the new position*/
				x += xs * t + 0.5 * xa * t * t;
				y += ys * t + 0.5 * ya * t * t;

				/*Get the new speed in function of the old and the new positions*/
				if (t != 0)
				{
					gParticleArray[i]->xSpeed = (x - xInit) / t;
					clamp(&(gParticleArray[i]->xSpeed), gMAX_SPEED);

					gParticleArray[i]->ySpeed = (y - yInit) / t;
					clamp(&(gParticleArray[i]->ySpeed), gMAX_SPEED);
				}

				/*Change the positions of the particle*/
				gParticleArray[i]->xCoord = x;
				gParticleArray[i]->yCoord = y;

				/*Test for collisions of the particle*/
				collisions(gParticleArray[i]);

				/*test if the particle is on goal*/
				onGoal(gParticleArray[i]);
			}
		}
	}
}

void rendering(void)
{
	organizeParticleBlitering(); //organise the array of particles

	SDL_FillRect(gScreen, NULL, SDL_MapRGB(gScreen->format, 113, 113, 155)); //Fill screen (erase all previous blits)
	SDL_BlitSurface(gBackground, NULL, gScreen, &gPositionBackground); //Blit background image

	/*Rendering screen content (particles, walls, text surfaces, etc.)*/
	switch (gGameState)
	{
	case EProgramState_GAME:
		renderGoal();
		renderWall();
		renderParticlesInGameState();
		break;

	case EProgramState_MENU:
		SDL_BlitSurface(gMenuPlayGame, NULL, gScreen, &gPositionMenuPlayGame);
		SDL_BlitSurface(gMenuCustomLevel, NULL, gScreen, &gPositionMenuCustomLevel);
		SDL_BlitSurface(gMenuHelp, NULL, gScreen, &gPositionMenuHelp);
		SDL_BlitSurface(gMenuQuit, NULL, gScreen, &gPositionMenuQuit);
		break;

	case EProgramState_LEVEL_CHOICE:
		renderLevelList();
		break;

	case EProgramState_CREATE_LEVEL:
		SDL_BlitSurface(gMenuCustomLevelModify, NULL, gScreen, &gPositionMenuCustomLevelModify);
		SDL_BlitSurface(gMenuCustomLevelNew, NULL, gScreen, &gPositionMenuCustomLevelNew);
		break;

	case EProgramState_HELP:
		SDL_SetColorKey(gRules, SDL_SRCCOLORKEY, SDL_MapRGB(gRules->format, 255, 255, 255));
		SDL_BlitSurface(gRules, NULL, gScreen, &gPositionRules);
		break;

	default: ;
	}

	/*Rendering footer*/
	switch (gGameState)
	{
	case EProgramState_GAME:
		renderFooterInGameState();
		break;

	case EProgramState_LEVEL_CHOICE: case EProgramState_CREATE_LEVEL: case EProgramState_HELP:
		/*Render menu button*/
		SDL_SetColorKey(gMenuButton, SDL_SRCCOLORKEY, SDL_MapRGB(gMenuButton->format, 255, 255, 255));
		SDL_BlitSurface(gMenuButton, NULL, gScreen, &gPositionMenuButton);
		break;

	default: ;
	}

	/*Rendering cursor*/
	switch (gGameState)
	{
	case EProgramState_GAME:
		renderCursorInGameState();
		break;

	case EProgramState_MENU: case EProgramState_LEVEL_CHOICE: case EProgramState_CREATE_LEVEL: case EProgramState_HELP:
		gCursor = gPointingHand; //Pointing hand cursor
		SDL_SetColorKey(gCursor, SDL_SRCCOLORKEY, SDL_MapRGB(gCursor->format, 255, 255, 255));
		SDL_BlitSurface(gCursor, NULL, gScreen, &gPositionCursor);//Blit cursor
		break;

	default: ;
	}

	SDL_Flip(gScreen);
}

void ticks60fps(void)
{
	const uint32_t ticks = SDL_GetTicks();

	if (ticks < gLastUpdateTick) return; //If this happens -> problem

	if (ticks - gLastUpdateTick < 16)
	{
		SDL_Delay(16 - (ticks - gLastUpdateTick));
		gDeltaTime = 16;
	}
	else
	{
		gDeltaTime = ticks - gLastUpdateTick;
	}

	gLastUpdateTick = ticks;
}

void destroy(void)
{
	/*Free the arrays of particles and walls*/
	int i;
	for (i = 0; i < gNumberOfParticle; ++i)
	{
		if (gParticleArray[i]) free(gParticleArray[i]);
	}
	for (i = 0; i < gNumberOfWall; ++i)
	{
		if (gWallArray[i]) free(gWallArray[i]);
	}

	/*Free all the SDl surfaces*/
	if (gBackground) SDL_FreeSurface(gBackground);
	if (gPositiveParticule1) SDL_FreeSurface(gPositiveParticule1);
	if (gPositiveParticule2) SDL_FreeSurface(gPositiveParticule2);
	if (gPositiveParticule3) SDL_FreeSurface(gPositiveParticule3);
	if (gNegativeParticule1) SDL_FreeSurface(gNegativeParticule1);
	if (gNegativeParticule2) SDL_FreeSurface(gNegativeParticule2);
	if (gNegativeParticule3) SDL_FreeSurface(gNegativeParticule3);
	if (gMovingParticle) SDL_FreeSurface(gMovingParticle);
	if (gPositiveHand1) SDL_FreeSurface(gPositiveHand1);
	if (gPositiveHand2) SDL_FreeSurface(gPositiveHand2);
	if (gPositiveHand3) SDL_FreeSurface(gPositiveHand3);
	if (gNegativeHand1) SDL_FreeSurface(gNegativeHand1);
	if (gNegativeHand2) SDL_FreeSurface(gNegativeHand2);
	if (gNegativeHand3) SDL_FreeSurface(gNegativeHand3);
	if (gOpenHand) SDL_FreeSurface(gOpenHand);
	if (gCloseHand) SDL_FreeSurface(gCloseHand);
	if (gDeniedHand) SDL_FreeSurface(gDeniedHand);
	if (gPointingHand) SDL_FreeSurface(gPointingHand);
	if (gGoalHand) SDL_FreeSurface(gGoalHand);
	if (gWallHand) SDL_FreeSurface(gWallHand);
	if (gMovingParticleHand) SDL_FreeSurface(gMovingParticleHand);
	if (gPlayButton) SDL_FreeSurface(gPlayButton);
	if (gRestartButton) SDL_FreeSurface(gRestartButton);
	if (gMenuButton) SDL_FreeSurface(gMenuButton);
	if (gSaveButton) SDL_FreeSurface(gSaveButton);
	if (gGoal) SDL_FreeSurface(gGoal);
	if (gMenuPlayGame) SDL_FreeSurface(gMenuPlayGame);
	if (gMenuCustomLevel) SDL_FreeSurface(gMenuCustomLevel);
	if (gMenuHelp) SDL_FreeSurface(gMenuHelp);
	if (gMenuQuit) SDL_FreeSurface(gMenuQuit);
	if (gMenuCustomLevelModify) SDL_FreeSurface(gMenuCustomLevelModify);
	if (gMenuCustomLevelNew) SDL_FreeSurface(gMenuCustomLevelNew);
	if (gScoresDisplay) SDL_FreeSurface(gScoresDisplay);
	if (gRules) SDL_FreeSurface(gRules);

	if (gFont) TTF_CloseFont(gFont);
	if (gFontMenu) TTF_CloseFont(gFontMenu);
	TTF_Quit();

	SDL_Quit();
}


/*----- INLINE FUNCTIONS FOR eventManager() -----*/
inline void handleLBMDInGameState(const SDL_Event event)
{
	/*Left click on playable area*/
	Particle* pTemp = handOnParticle(event.button.x, event.button.y);

	if (!gPlay) //If the game is in pause
	{
		/*If the cursor is on a particle, move the particle*/
		if (pTemp)
		{
			getShifting(pTemp);
			gDragedParticle = pTemp;
		}
			/*Else create in function of the cursor state*/
		else
		{
			switch (gCursorState)
			{
			case -6: //Create goal
				{
					/*Goal can't be created on the footer*/
					if (!handOnFooter(event.button.y)) drawGoal(event.button.x, event.button.y);
				}
				break;

			case -5: //Create wall
				{
					/*Wall can't be created on the footer*/
					if (!handOnFooter(event.button.y)) drawWall(event.button.x, event.button.y);
				}
				break;

			case -4: //Create moving particle
				{
					/*Moving particle can't be created on goal, wall or footer*/
					if (!surfaceHitbox(gPositionGoal, event.button.x, event.button.y, 0, 0, 0, 0)
						&& !handOnWall(event.button.x, event.button.y)
						&& !handOnFooter(event.button.y))

					{
						createParticle(true, true, 1, event.button.x, event.button.y);
					}
				}
				break;

			default: //Create non moving particle
				{
					/*Non moving particle can't be created on goal, wall or footer*/
					if (!surfaceHitbox(gPositionGoal, event.button.x, event.button.y, 0, 0, 0, 0)
						&& !handOnWall(event.button.x, event.button.y)
						&& !handOnFooter(event.button.y))

					{
						createParticle(false, true, gCursorState, event.button.x, event.button.y);
					}
				}
				break;
			}
		}
	}

	/*Left click on the footer*/
	const EButton button = surfaceButtonManager(event.button.x, event.button.y);

	switch (button)
	{
	case EButton_FOOTER_MENU:
		freeLevel();
		gGameState = EProgramState_MENU;
		break;

	case EButton_FOOTER_PLAY_RESTART:
		levelPlayRestart();
		break;

	case EButton_FOOTER_SAVE:
		saveLevel(gLevel);
		break;

	default: ;
	}
}

inline void handleLBMDInMenuState(const SDL_Event event)
{
	const EButton button = surfaceButtonManager(event.button.x, event.button.y);

	switch (button)
	{
	case EButton_MENU_PLAY_GAME:
		gGameState = EProgramState_LEVEL_CHOICE;
		gCreativeMode = 0;
		scanLevels();
		break;

	case EButton_MENU_CUSTOM_LEVEL:
		gGameState = EProgramState_CREATE_LEVEL;
		gCreativeMode = 1;
		break;

	case EButton_MENU_HELP:
		gGameState = EProgramState_HELP;
		break;

	case EButton_MENU_QUIT:
		gIsRunning = 0;
		break;

	default: ;
	}
}

inline void handleLBMDInLevelChoiceState(const SDL_Event event)
{
	/*Left click on a level*/
	const int levelChoice = surfaceLevelListManager(event.button.x, event.button.y);

	if (levelChoice != 0) //If a level is choosen
	{
		gGameState = EProgramState_GAME;
		gPlay = 0;
		gCursorState = 1;
		gLevel = levelChoice;
		loadLevel(gLevel);
	}

	/*Left click on menu button*/
	const EButton button = surfaceButtonManager(event.button.x, event.button.y);
	if (button == EButton_FOOTER_MENU) gGameState = EProgramState_MENU;
}

inline void handleLBMDInCreateLevelState(const SDL_Event event)
{
	const EButton button = surfaceButtonManager(event.button.x, event.button.y);

	switch (button)
	{
	case EButton_MENU_CUSTOM_LEVEL_MODIFY:
		gGameState = EProgramState_LEVEL_CHOICE;
		scanLevels();
		break;

	case EButton_MENU_CUSTOM_LEVEL_NEW:
		gGameState = EProgramState_GAME;
		scanLevels();
		gLevel = gNumberOfLevels + 1;
		break;

	case EButton_FOOTER_MENU:
		gGameState = EProgramState_MENU;

	default: ;
	}
}


/*----- INLINE FUNCTIONS FOR rendering() -----*/
inline void renderParticlesInGameState(void)
{
	int i;
	SDL_Surface* particle = NULL;
	SDL_Rect positionParticle;

	for (i = 0; i < gNumberOfParticle; ++i)
	{
		positionParticle.x = (Sint16)((int)gParticleArray[i]->xCoord - gParticleRadius);
		positionParticle.y = (Sint16)((int)gParticleArray[i]->yCoord - gParticleRadius);

		if (gParticleArray[i]->moving) //Render moving particle
		{
			particle = gMovingParticle;
		}
		else //Render non moving particle
		{
			switch (gParticleArray[i]->charge)
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
			default: ;
			}
		}

		if (particle)
		{
			SDL_SetColorKey(particle, SDL_SRCCOLORKEY, SDL_MapRGB(particle->format, 255, 255, 255));
			SDL_BlitSurface(particle, NULL, gScreen, &positionParticle); //Blit particle
		}
	}
}

inline void renderCursorInGameState(void)
{
	/*If cursor not on a particle, render the cursor in function of it's state*/
	if (!gPointedParticle)
	{
		switch (gCursorState)
		{
		case -6: //Goal creation cursor
			gCursor = gGoalHand;
			break;
		case -5: //Wall creation cursor
			gCursor = gWallHand;
			break;
		case -4: //Moving particle creation cursor
			gCursor = gMovingParticleHand;
			break;
		case -3:
			gCursor = gNegativeHand3;
			break;
		case -2:
			gCursor = gNegativeHand2;
			break;
		case -1:
			gCursor = gNegativeHand1;
			break;
		case 1:
			gCursor = gPositiveHand1;
			break;
		case 2:
			gCursor = gPositiveHand2;
			break;
		case 3:
			gCursor = gPositiveHand3;
			break;
		default: ;
		}
	}
	else //Else render the cursor in function of the pointed particle parameters
	{
		if (gPointedParticle->modifiable) //If the pointed particle is modifiable
		{
			if (gDragedParticle) gCursor = gCloseHand; //If the particle is being moved : closed hand cursor
			else gCursor = gOpenHand; //Else open hand cursor
		}
		else gCursor = gDeniedHand; //Else denied hand cursor
	}

	SDL_SetColorKey(gCursor, SDL_SRCCOLORKEY, SDL_MapRGB(gCursor->format, 255, 255, 255));
	SDL_BlitSurface(gCursor, NULL, gScreen, &gPositionCursor); //Blit cursor
}

inline void renderFooterInGameState(void)
{
	if (!gCreativeMode) //If in normal game, render scores
	{
		char text[100];
		if (gWin) createEndLevelText(text);
		else createScoreText(text);

		gScoresDisplay = TTF_RenderText_Blended(gFont, text, gFontColor);
		SDL_BlitSurface(gScoresDisplay, NULL, gScreen, &gPositionScoresDisplay);
		SDL_FreeSurface(gScoresDisplay);
		gScoresDisplay = NULL;
	}

	/*Render footer buttons*/
	/*Render play/restart button*/
	if (gPlay)
	{
		SDL_SetColorKey(gRestartButton, SDL_SRCCOLORKEY, SDL_MapRGB(gRestartButton->format, 255, 255, 255));
		SDL_BlitSurface(gRestartButton, NULL, gScreen, &gPositionPlayRestartButton);
	}
	else
	{
		SDL_SetColorKey(gPlayButton, SDL_SRCCOLORKEY, SDL_MapRGB(gPlayButton->format, 255, 255, 255));
		SDL_BlitSurface(gPlayButton, NULL, gScreen, &gPositionPlayRestartButton);
	}

	/*Render menu button*/
	SDL_SetColorKey(gMenuButton, SDL_SRCCOLORKEY, SDL_MapRGB(gMenuButton->format, 255, 255, 255));
	SDL_BlitSurface(gMenuButton, NULL, gScreen, &gPositionMenuButton);

	/*Render save button (only in creative mode)*/
	if (gCreativeMode)
	{
		SDL_SetColorKey(gSaveButton, SDL_SRCCOLORKEY, SDL_MapRGB(gSaveButton->format, 255, 255, 255));
		SDL_BlitSurface(gSaveButton, NULL, gScreen, &gPositionSaveButton);
	}
}


/*----- CREATION FUNCTION -----*/
void createParticle(bool moving, bool modifiable, const int32_t charge, const int x, const int y)
{
	if (gNumberOfParticle < gMAX_PARTICLES)
	{
		Particle* p = (Particle*)malloc(sizeof(Particle));
		if (!p) return;

		p->moving = moving;
		p->modifiable = modifiable;
		p->charge = charge;
		p->xCoord = (double)x;
		p->yCoord = (double)y;
		p->xCoordInit = p->xCoord;
		p->yCoordInit = p->yCoord;
		p->xSpeed = 0;
		p->ySpeed = 0;
		p->goal = 0;

		/*Ensure that the particle doesn't overlap the goal, a wall or the footer if it is created next to them*/
		collisions(p);

		gParticleArray[gNumberOfParticle] = p;
		gNumberOfParticle++;

		if (moving) gNumberOfMovingParticle++;
	}
	else
	{
		printf("ERROR: Number max of particles reached \n");
	}
}

void createWall(const int x, const int y, const int w, const int h)
{
	if (gNumberOfWall < gMAX_WALL)
	{
		Wall* wallTemp = (Wall*)malloc(sizeof(Wall));
		if (!wallTemp) return;

		wallTemp->x = x;
		wallTemp->y = y;
		wallTemp->w = w;
		wallTemp->h = h;

		gWallArray[gNumberOfWall] = wallTemp;
		gNumberOfWall++;

		/*Move all the particles present in the created wall*/
		int i;
		for (i = 0; i < gNumberOfParticle; ++i) collisions(gParticleArray[i]);
	}
	else
	{
		printf("ERROR: Number max of walls reached \n");
	}
}

void createGoal(const int32_t x, const int32_t y, const int32_t w, const int32_t h)
{
	gPositionGoal.x = (Sint16)x;
	gPositionGoal.y = (Sint16)y;
	gPositionGoal.w = (Uint16)w;
	gPositionGoal.h = (Uint16)h;

	if (gGoal) SDL_FreeSurface(gGoal); //If a goal is already existing, delete it

	gGoal = SDL_CreateRGBSurface(SDL_HWSURFACE, gPositionGoal.w, gPositionGoal.h, 32, 0, 0, 0, 0);
	SDL_FillRect(gGoal, NULL, SDL_MapRGB(gScreen->format, 205, 131, 0));

	/*Move all the particles present in the created goal*/
	int i;
	for (i = 0; i < gNumberOfParticle; ++i) collisions(gParticleArray[i]);
}


/*----- DELETION FUNCTIONS -----*/
void destroyParticle(Particle* p)
{
	int i, j;

	for (i = 0; i < gNumberOfParticle; ++i)
	{
		if (gParticleArray[i] && p && gParticleArray[i]->modifiable && gParticleArray[i] == p)
		{
			if (gParticleArray[i]->moving == true) gNumberOfMovingParticle--;

			free(gParticleArray[i]);
			gParticleArray[i] = NULL;

			for (j = i; j < gNumberOfParticle - 1; ++j)
			{
				gParticleArray[j] = gParticleArray[j + 1];
			}
			gParticleArray[gNumberOfParticle - 1] = NULL;

			gNumberOfParticle--;
		}
	}
}

void destroyWall(Wall* w)
{
	int i, j;

	for (i = 0; i < gNumberOfWall; ++i)
	{
		if (gWallArray[i] && w && gWallArray[i] == w)
		{
			free(gWallArray[i]);
			gWallArray[i] = NULL;

			for (j = i; j < gNumberOfWall - 1; ++j)
			{
				gWallArray[j] = gWallArray[j + 1];
			}
			gWallArray[gNumberOfWall - 1] = NULL;

			gNumberOfWall--;
		}
	}
}

void destroyGoal(void)
{
	if (gGoal) SDL_FreeSurface(gGoal);
	gGoal = NULL;
	gPositionGoal.x = 0;
	gPositionGoal.y = 0;
	gPositionGoal.w = 0;
	gPositionGoal.h = 0;
}


/*----- MOVEMENT FUNCTIONS -----*/
void moveParticle(void)
{
	if (gDragedParticle && gDragedParticle->modifiable)
	{
		gDragedParticle->xCoord = (double)(gPositionCursor.x + gShiftPickedParticleX);
		gDragedParticle->yCoord = (double)(gPositionCursor.y + gShiftPickedParticleY);

		collisions(gDragedParticle);
	}
}

void getShifting(Particle* p)
{
	gShiftPickedParticleX = (int)p->xCoord - gPositionCursor.x;
	gShiftPickedParticleY = (int)p->yCoord - gPositionCursor.y;
}

void getAcceleration(Particle* p, double* ax, double* ay)
{
	double sumForcesX = 0, sumForcesY = 0;
	int i;

	for (i = 0; i < gNumberOfParticle; ++i)
	{
		if (p != gParticleArray[i])
		{
			int c1 = p->charge;
			int c2 = gParticleArray[i]->charge;
			double d = max(getDistance(p, gParticleArray[i]), gMIN_DISTANCE);
			double positionVectorX = (gParticleArray[i]->xCoord - p->xCoord) / d;
			double positionVectorY = (gParticleArray[i]->yCoord - p->yCoord) / d;

			sumForcesX += -((c1 * c2) / (d * d)) * positionVectorX;
			sumForcesY += -((c1 * c2) / (d * d)) * positionVectorY;
		}
	}

	*ax = sumForcesX * gPARTICLE_MASS;
	clamp(ax, gMAX_ACCELERATION);

	*ay = sumForcesY * gPARTICLE_MASS;
	clamp(ay, gMAX_ACCELERATION);
}

void clamp(double* value, const double limit)
{
	if (*value < -limit) *value = -limit;
	else if (*value > limit) *value = limit;
}

double getDistance(Particle* p1, Particle* p2)
{
	return sqrt(pow(p1->xCoord - p2->xCoord, 2) + pow(p1->yCoord - p2->yCoord, 2));
}

void onGoal(Particle* p)
{
	if (surfaceHitbox(gPositionGoal, p->xCoord, p->yCoord, -(int)gParticleRadius, -(int)gParticleRadius,
	                  -(int)gParticleRadius, -(int)gParticleRadius))
	{
		p->goal = 1;
		gNumberOfParticlesOnGoal++;
		printf("Partice on goal \n");

		/*If in creative mode, the level can be tested but can't be win*/
		if (!gCreativeMode && gNumberOfParticlesOnGoal == gNumberOfMovingParticle) levelFinished();
	}
}


/*----- RENDERING FUNCTIONS -----*/
void organizeParticleBlitering(void)
{
	int counterMovingParticlesSorted = 0;
	int i = 0, j;
	bool nonMovingParticleFound;

	bool arrayInOrderP = false;
	bool arrayInOrderMP = false;
	int k = 1;
	int l = 1;
	Particle* ptemp = NULL;

	/* Sort the moving particles from the non-moving particles. Put the moving particles at the end of the particle array */
	while (gParticleArray[i] && counterMovingParticlesSorted < gNumberOfMovingParticle)
	{
		if (!(gParticleArray[gNumberOfParticle - 1 - i]->moving))
		{
			j = i + 1;
			nonMovingParticleFound = false;
			while (!nonMovingParticleFound)
			{
				if (gParticleArray[gNumberOfParticle - 1 - j]->moving)
				{
					ptemp = gParticleArray[gNumberOfParticle - 1 - i];
					gParticleArray[gNumberOfParticle - 1 - i] = gParticleArray[gNumberOfParticle - 1 - j];
					gParticleArray[gNumberOfParticle - 1 - j] = ptemp;

					nonMovingParticleFound = true;
				}
				++j;
			}
		}
		counterMovingParticlesSorted++;
		++i;
	}

	/* Sort the moving particle if function of their y coordinate */
	while (!arrayInOrderMP)
	{
		arrayInOrderMP = true;

		for (i = gNumberOfParticle - l; i > gNumberOfParticle - gNumberOfMovingParticle; --i)
		{
			if (gParticleArray[i] && gParticleArray[i - 1] && gParticleArray[i]->yCoord < gParticleArray[i - 1]->yCoord)
			{
				ptemp = gParticleArray[i];
				gParticleArray[i] = gParticleArray[i - 1];
				gParticleArray[i - 1] = ptemp;

				arrayInOrderMP = false;
			}
		}
		l++;
	}

	/* Sort the non-moving particle if function of their y coordinate */
	while (!arrayInOrderP)
	{
		arrayInOrderP = true;

		for (i = gNumberOfParticle - gNumberOfMovingParticle - k; i > 0; --i)
		{
			if (gParticleArray[i] && gParticleArray[i - 1] && gParticleArray[i]->yCoord < gParticleArray[i - 1]->yCoord)
			{
				ptemp = gParticleArray[i];
				gParticleArray[i] = gParticleArray[i - 1];
				gParticleArray[i - 1] = ptemp;

				arrayInOrderP = false;
			}
		}
		k++;
	}
}

void renderWall(void)
{
	int i;

	SDL_Surface* wallTemp = NULL;
	SDL_Rect positionWallTemp;

	for (i = 0; i < gNumberOfWall; ++i) //Render the existing walls
	{
		positionWallTemp.x = (Sint16)gWallArray[i]->x;
		positionWallTemp.y = (Sint16)gWallArray[i]->y;
		positionWallTemp.w = (Uint16)gWallArray[i]->w;
		positionWallTemp.h = (Uint16)gWallArray[i]->h;

		wallTemp = SDL_CreateRGBSurface(SDL_HWSURFACE, positionWallTemp.w, positionWallTemp.h, 32, 0, 0, 0, 0);
		SDL_FillRect(wallTemp, NULL, SDL_MapRGB(gScreen->format, 128, 64, 64));
		SDL_BlitSurface(wallTemp, NULL, gScreen, &positionWallTemp);
		SDL_FreeSurface(wallTemp);
	}

	if (gCreationWall)
		//If the player is in the process of creating a wall, render the surface selected by the player as a wall
	{
		getPositionSurface(&positionWallTemp.x, &positionWallTemp.y, &positionWallTemp.w, &positionWallTemp.h);

		wallTemp = SDL_CreateRGBSurface(SDL_HWSURFACE, positionWallTemp.w, positionWallTemp.h, 32, 0, 0, 0, 0);
		SDL_FillRect(wallTemp, NULL, SDL_MapRGB(gScreen->format, 128, 64, 64));
		SDL_BlitSurface(wallTemp, NULL, gScreen, &positionWallTemp);
		SDL_FreeSurface(wallTemp);
	}
}

void renderGoal(void)
{
	SDL_BlitSurface(gGoal, NULL, gScreen, &gPositionGoal); //Render the actual goal

	if (gCreationGoal)
		//If the player is in the process of creating a new goal, render the surface selected by the player as a goal
	{
		SDL_Surface* goalTemp = NULL;
		SDL_Rect positionGoalTemp;
		getPositionSurface(&positionGoalTemp.x, &positionGoalTemp.y, &positionGoalTemp.w, &positionGoalTemp.h);

		goalTemp = SDL_CreateRGBSurface(SDL_HWSURFACE, positionGoalTemp.w, positionGoalTemp.h, 32, 0, 0, 0, 0);
		SDL_FillRect(goalTemp, NULL, SDL_MapRGB(gScreen->format, 205, 131, 0));
		SDL_BlitSurface(goalTemp, NULL, gScreen, &positionGoalTemp);
		SDL_FreeSurface(goalTemp);
	}
}

void renderLevelList(void)
{
	int i;
	char levelName[100];
	SDL_Surface* levelNameDisplay;
	SDL_Rect positionLevelNameDisplay;

	for (i = 0; i < gNumberOfLevels && i < 14; ++i)
	{
		getLevelFileName(levelName, i + 1, false);
		levelNameDisplay = TTF_RenderText_Blended(gFont, levelName, gFontColor);
		positionLevelNameDisplay.x = (Sint16)40;
		positionLevelNameDisplay.y = (Sint16)(30 + i * 30);
		SDL_BlitSurface(levelNameDisplay, NULL, gScreen, &positionLevelNameDisplay);
		SDL_FreeSurface(levelNameDisplay);
	}

	for (i = 14; i < gNumberOfLevels && i < 28; ++i)
	{
		getLevelFileName(levelName, i + 1, false);
		levelNameDisplay = TTF_RenderText_Blended(gFont, levelName, gFontColor);
		positionLevelNameDisplay.x = (Sint16)270;
		positionLevelNameDisplay.y = (Sint16)(30 + (i - 14) * 30);
		SDL_BlitSurface(levelNameDisplay, NULL, gScreen, &positionLevelNameDisplay);
		SDL_FreeSurface(levelNameDisplay);
	}

	for (i = 28; i < gNumberOfLevels; ++i)
	{
		getLevelFileName(levelName, i + 1, false);
		levelNameDisplay = TTF_RenderText_Blended(gFont, levelName, gFontColor);
		positionLevelNameDisplay.x = (Sint16)500;
		positionLevelNameDisplay.y = (Sint16)(30 + (i - 28) * 30);
		SDL_BlitSurface(levelNameDisplay, NULL, gScreen, &positionLevelNameDisplay);
		SDL_FreeSurface(levelNameDisplay);
	}
}

void createScoreText(char* txt)
{
	char highScores[100];
	char actualScore[100];
	int i;
	char hs1[100], hs2[100], hs3[100];
	char* hslist[3];// = { hs1, hs2, hs3 };
	hslist[0] = hs1;
	hslist[1] = hs2;
	hslist[2] = hs3;

	/*Set a string for each high scores. If a high score is -1 (not yet set), set the string to "x"*/
	for (i = 0; i < 3; ++i)
	{
		if (gLevelHighScores[i] == -1) strcpy(hslist[i], "x");
		else sprintf(hslist[i], "%d", gLevelHighScores[i]);
	}

	/*Create the string for the display of the three high scores*/
	strcpy(highScores, hslist[0]);
	for (i = 1; i < 3; ++i)
	{
		strcat(highScores, " - ");
		strcat(highScores, hslist[i]);
	}

	/*Set the string for the actual score*/
	sprintf(actualScore, "%d", gNumberOfParticle - gNumberOfparticlesInTheLevel);

	/*Assemble the strings for the high scores and the actual score*/
	strcpy(txt, "High Scores : ");
	strcat(txt, highScores);
	strcat(txt, "   |   Your Score : ");
	strcat(txt, actualScore);
}

void createEndLevelText(char* txt)
{
	char actualScore[100];
	sprintf(actualScore, "%d", gNumberOfParticle - gNumberOfparticlesInTheLevel);

	strcpy(txt, "Level finished ! Your Score : ");
	strcat(txt, actualScore);
}


/*----- LEVEL MANAGEMENT FUNCTIONS -----*/
void loadLevel(const int n)
{
	int numberOfMovingParticles = 0, numberOfNonMovingParticles = 0, numberOfWalls = 0;
	int xGoal = 0, yGoal = 0, wGoal = 0, hGoal = 0;
	int xMovingParticle = 0, yMovingParticle = 0;
	int xNonMovingParticle = 0, yNonMovingParticle = 0, chargeNonMovingParticle = 0;
	int xWall = 0, yWall = 0, wWall = 0, hWall = 0;
	int i;
	char filename[100];
	bool modifiable = false; //All the particles of the level will be created as non-modifiable

	if (gCreativeMode) modifiable = true;
	//If the level is loaded in creative mode, all the particles will be created as modifiable

	getLevelFileName(filename, n, true);

	FILE* f = NULL;
	f = fopen(filename, "r");
	if (f)
	{
		/*Loading of the high scores*/
		fscanf(f, "%d %d %d", &gLevelHighScores[0], &gLevelHighScores[1], &gLevelHighScores[2]);

		/*Loading of the goal position*/
		fscanf(f, "%d %d %d %d", &xGoal, &yGoal, &wGoal, &hGoal);
		createGoal(xGoal, yGoal, wGoal, hGoal);

		/*Loading of the number of particles and walls*/
		fscanf(f, "%d %d %d", &numberOfMovingParticles, &numberOfNonMovingParticles, &numberOfWalls);

		/*Loading of the moving particles*/
		for (i = 0; i < numberOfMovingParticles; ++i)
		{
			fscanf(f, "%d %d", &xMovingParticle, &yMovingParticle);
			createParticle(true, modifiable, 1, xMovingParticle, yMovingParticle);
		}

		/*Loading of the non moving particles*/
		for (i = 0; i < numberOfNonMovingParticles; ++i)
		{
			fscanf(f, "%d %d %d", &xNonMovingParticle, &yNonMovingParticle, &chargeNonMovingParticle);
			createParticle(false, modifiable, chargeNonMovingParticle, xNonMovingParticle, yNonMovingParticle);
		}

		/*Loading of the walls positions*/
		for (i = 0; i < numberOfWalls; ++i)
		{
			fscanf(f, "%d %d %d %d", &xWall, &yWall, &wWall, &hWall);
			createWall(xWall, yWall, wWall, hWall);
		}

		fclose(f);

		gNumberOfparticlesInTheLevel = numberOfMovingParticles + numberOfNonMovingParticles;

		printf("Level %d loaded \n", n);
	}
	else
	{
		printf("Fail to load %s \n", filename);
	}
}

void saveLevel(const int n)
{
	int i;
	char filename[100];

	getLevelFileName(filename, n, true);

	organizeParticleBlitering();
	//Organise the particle array to be sure that all the moving particles are at the end of the array

	FILE* f = NULL;
	f = fopen(filename, "w");
	if (f)
	{
		/*Saving of the high scores*/
		if (gCreativeMode) fprintf(f, "%d %d %d\n", -1, -1, -1);
			//If the level was created/modified, the high scores are saved as -1 (reset)
		else fprintf(f, "%d %d %d\n", gLevelHighScores[0], gLevelHighScores[1], gLevelHighScores[2]);

		/*Saving the goal position*/
		fprintf(f, "%d %d %d %d\n", gPositionGoal.x, gPositionGoal.y, gPositionGoal.w, gPositionGoal.h);

		/*Saving the numbers of particles and walls*/
		if (gCreativeMode)
			fprintf(f, "%d %d %d\n", gNumberOfMovingParticle, gNumberOfParticle - gNumberOfMovingParticle,
			        gNumberOfWall);
		else
			fprintf(f, "%d %d %d\n", gNumberOfMovingParticle, gNumberOfparticlesInTheLevel - gNumberOfMovingParticle,
			        gNumberOfWall);

		/*Saving the moving particles*/
		for (i = gNumberOfParticle - gNumberOfMovingParticle; i < gNumberOfParticle; ++i)
		{
			fprintf(f, "%d %d\n", (int)gParticleArray[i]->xCoordInit, (int)gParticleArray[i]->yCoordInit);
		}

		/*Saving the non moving particles*/
		for (i = 0; i < gNumberOfParticle - gNumberOfMovingParticle; ++i)
		{
			if (gCreativeMode) //If in creative mode, all the non moving particles are saved
			{
				fprintf(f, "%d %d %d\n", (int)gParticleArray[i]->xCoord, (int)gParticleArray[i]->yCoord, gParticleArray[i]->charge);
			}
			else
			{
				if (!gParticleArray[i]->modifiable) //If in game, just the initials non modifiables particles are saved
				{
					fprintf(f, "%d %d %d\n", (int)gParticleArray[i]->xCoord, (int)gParticleArray[i]->yCoord,
					        gParticleArray[i]->charge);
				}
			}
		}

		/*Saving the walls positions*/
		for (i = 0; i < gNumberOfWall; ++i)
		{
			fprintf(f, "%d %d %d %d\n", gWallArray[i]->x, gWallArray[i]->y, gWallArray[i]->w, gWallArray[i]->h);
		}

		fclose(f);

		printf("Level saved \n");
	}
	else
	{
		printf("Fail to save %s \n", filename);
	}
}

void reinitLevel(void)
{
	int i;

	for (i = gNumberOfParticle - gNumberOfMovingParticle; i < gNumberOfParticle; ++i)
	{
		gParticleArray[i]->xCoord = gParticleArray[i]->xCoordInit;
		gParticleArray[i]->yCoord = gParticleArray[i]->yCoordInit;
		gParticleArray[i]->xSpeed = 0;
		gParticleArray[i]->ySpeed = 0;
		gParticleArray[i]->goal = 0;
	}

	gNumberOfParticlesOnGoal = 0;
}

void scanLevels(void)
{
	int levelCounter = 0;
	char filename[100];

	getLevelFileName(filename, levelCounter + 1, true);

	FILE* f = NULL;
	f = fopen(filename, "r");

	while (f != NULL && levelCounter < gMAX_LEVELS_LOADABLE)
	{
		fclose(f);
		levelCounter++;
		getLevelFileName(filename, levelCounter + 1, true);
		f = fopen(filename, "r");
	}

	gNumberOfLevels = levelCounter;

	printf("%d level(s) found \n", gNumberOfLevels);
	if (levelCounter == gMAX_LEVELS_LOADABLE)
		printf(
			"WARNING : Number maximum of levels loadable reached, some levels may have not been loaded \n");
}

void freeLevel(void)
{
	int i;

	/*Delete the particles*/
	for (i = 0; i < gNumberOfParticle; ++i)
	{
		free(gParticleArray[i]);
		gParticleArray[i] = NULL;
	}
	gNumberOfParticle = 0;
	gNumberOfMovingParticle = 0;
	gNumberOfParticlesOnGoal = 0;
	gNumberOfparticlesInTheLevel = 0;

	/*Delete the walls*/
	for (i = 0; i < gNumberOfWall; ++i)
	{
		free(gWallArray[i]);
		gWallArray[i] = NULL;
	}
	gNumberOfWall = 0;

	/*Delete the goal*/
	destroyGoal();

	/*Reset the high scores*/
	memset(gLevelHighScores, 0, 3 * sizeof(uint32_t));
}

void levelFinished(void)
{
	gWin = 1;
	printf("Level finished \n");

	/*If a record is set, save the level with the new high score*/
	if (updateHighScores(gNumberOfParticle - gNumberOfparticlesInTheLevel))
	{
		saveLevel(gLevel);
	}
}

void levelPlayRestart(void)
{
	if (gPlay)
	{
		reinitLevel();
		gPlay = 0;
		gWin = 0;
	}
	else
	{
		gPlay = 1;
	}
}

void getLevelFileName(char* fileName, const int n, bool addExtension)
{
	char lvlNumber[100];
	sprintf(lvlNumber, "%d", n);

	strcpy(fileName, "lvl");
	strcat(fileName, lvlNumber);
	if (addExtension) strcat(fileName, ".txt");
}

bool updateHighScores(const int newScore)
{
	int i, j;

	for (i = 0; i < 3; ++i)
	{
		if (newScore < gLevelHighScores[i] || gLevelHighScores[i] == -1)
		{
			for (j = 2; j > i; --j)
			{
				gLevelHighScores[j] = gLevelHighScores[j - 1];
			}
			gLevelHighScores[i] = newScore;
			return true;
		}
	}
	return false;
}


/*----- SURFACES MANAGEMENT FUNCTIONS -----*/
void collisions(Particle* p)
{
	int i;
	SDL_Rect surfaceTemp;

	if (p)
	{
		/*Collisions with walls*/
		for (i = 0; i < gNumberOfWall; ++i)
		{
			/*Creation of a temporary SDL_Rect for the walls*/
			surfaceTemp.x = (Sint16)gWallArray[i]->x;
			surfaceTemp.y = (Sint16)gWallArray[i]->y;
			surfaceTemp.w = (Uint16)gWallArray[i]->w;
			surfaceTemp.h = (Uint16)gWallArray[i]->h;

			if (p->xSpeed >= 0 && surfaceHitbox(surfaceTemp, p->xCoord, p->yCoord, gParticleRadius, 0, 0, 0))
			{
				p->xSpeed = 0;
				p->xCoord = gWallArray[i]->x - gParticleRadius;
			}

			if (p->xSpeed <= 0 && surfaceHitbox(surfaceTemp, p->xCoord, p->yCoord, 0, gParticleRadius, 0, 0))
			{
				p->xSpeed = 0;
				p->xCoord = gWallArray[i]->x + gWallArray[i]->w + gParticleRadius;
			}

			if (p->ySpeed >= 0 && surfaceHitbox(surfaceTemp, p->xCoord, p->yCoord, 0, 0, gParticleRadius, 0))
			{
				p->ySpeed = 0;
				p->yCoord = gWallArray[i]->y - gParticleRadius;
			}

			if (p->ySpeed <= 0 && surfaceHitbox(surfaceTemp, p->xCoord, p->yCoord, 0, 0, 0, gParticleRadius))
			{
				p->ySpeed = 0;
				p->yCoord = gWallArray[i]->y + gWallArray[i]->h + gParticleRadius;
			}
		}

		/*Collisions with the goal*/
		if (!(p->moving) || (gCreativeMode && !gPlay))
		{
			if (p->xSpeed >= 0 && surfaceHitbox(gPositionGoal, p->xCoord, p->yCoord, gParticleRadius, 0, 0, 0))
			{
				p->xSpeed = 0;
				p->xCoord = gPositionGoal.x - gParticleRadius;
			}

			if (p->xSpeed <= 0 && surfaceHitbox(gPositionGoal, p->xCoord, p->yCoord, 0, gParticleRadius, 0, 0))
			{
				p->xSpeed = 0;
				p->xCoord = gPositionGoal.x + gPositionGoal.w + gParticleRadius;
			}

			if (p->ySpeed >= 0 && surfaceHitbox(gPositionGoal, p->xCoord, p->yCoord, 0, 0, gParticleRadius, 0))
			{
				p->ySpeed = 0;
				p->yCoord = gPositionGoal.y - gParticleRadius;
			}

			if (p->ySpeed <= 0 && surfaceHitbox(gPositionGoal, p->xCoord, p->yCoord, 0, 0, 0, gParticleRadius))
			{
				p->ySpeed = 0;
				p->yCoord = gPositionGoal.y + gPositionGoal.h + gParticleRadius;
			}
		}

		/*Collisions with the limits playable*/
		if (p->xSpeed >= 0 && p->xCoord >= gScreen->w - gParticleRadius)
		{
			p->xSpeed = 0;
			p->xCoord = gScreen->w - gParticleRadius;
		}

		if (p->xSpeed <= 0 && p->xCoord <= 0 + gParticleRadius)
		{
			p->xSpeed = 0;
			p->xCoord = 0 + gParticleRadius;
		}

		if (p->ySpeed >= 0 && p->yCoord >= gBackground->h - gParticleRadius)
		{
			p->ySpeed = 0;
			p->yCoord = gBackground->h - gParticleRadius;
		}

		if (p->ySpeed <= 0 && p->yCoord <= 0 + gParticleRadius)
		{
			p->ySpeed = 0;
			p->yCoord = 0 + gParticleRadius;
		}
	}
}

bool surfaceHitbox(SDL_Rect S, const double x, const double y, const int xNegShift, const int xPosShift,
                   const int yNegShift, const int yPosShift)
{
	if ((int)x >= S.x - xNegShift && (int)x <= S.x + S.w + xPosShift
		&& (int)y >= S.y - yNegShift && (int)y <= S.y + S.h + yPosShift)
	{
		return true;
	}
	return false;
}

void getPositionSurface(short* x, short* y, unsigned short* w, unsigned short* h)
{
	*x = (short)min(gp1x, gp2x);
	*y = (short)min(gp1y, gp2y);
	*w = (unsigned short)abs(gp1x - gp2x);
	*h = (unsigned short)abs(gp1y - gp2y);
}

Particle* handOnParticle(const int x, const int y)
{
	int i;

	for (i = gNumberOfParticle - 1; i >= 0; --i)
	{
		if (gParticleArray[i]
			&& sqrt(pow((int)gParticleArray[i]->xCoord - x, 2) + pow((int)gParticleArray[i]->yCoord - y, 2)) < gParticleRadius)
		{
			return gParticleArray[i];
		}
	}
	return NULL;
}

bool handOnFooter(const int y)
{
	if (y >= 480) return true;
	return false;
}

Wall* handOnWall(const int x, const int y)
{
	int i;
	SDL_Rect surfaceTemp;

	for (i = 0; i < gNumberOfWall; ++i)
	{
		surfaceTemp.x = (Sint16)gWallArray[i]->x;
		surfaceTemp.y = (Sint16)gWallArray[i]->y;
		surfaceTemp.w = (Uint16)gWallArray[i]->w;
		surfaceTemp.h = (Uint16)gWallArray[i]->h;

		if (surfaceHitbox(surfaceTemp, x, y, 0, 0, 0, 0)) return gWallArray[i];
	}

	return NULL;
}

EButton surfaceButtonManager(const int x, const int y)
{
	switch (gGameState)
	{
	case EProgramState_GAME:
		if (surfaceHitbox(gPositionMenuButton, x, y, 0, 0, 0, 0)) return EButton_FOOTER_MENU;
		if (surfaceHitbox(gPositionPlayRestartButton, x, y, 0, 0, 0, 0)) return EButton_FOOTER_PLAY_RESTART;
		if (gCreativeMode && surfaceHitbox(gPositionSaveButton, x, y, 0, 0, 0, 0)) return EButton_FOOTER_SAVE;
		break;

	case EProgramState_MENU:
		if (surfaceHitbox(gPositionMenuPlayGame, x, y, 0, 0, 0, 0)) return EButton_MENU_PLAY_GAME;
		if (surfaceHitbox(gPositionMenuCustomLevel, x, y, 0, 0, 0, 0)) return EButton_MENU_CUSTOM_LEVEL;
		if (surfaceHitbox(gPositionMenuHelp, x, y, 0, 0, 0, 0)) return EButton_MENU_HELP;
		if (surfaceHitbox(gPositionMenuQuit, x, y, 0, 0, 0, 0)) return EButton_MENU_QUIT;
		break;

	case EProgramState_LEVEL_CHOICE: case EProgramState_HELP:
		if (surfaceHitbox(gPositionMenuButton, x, y, 0, 0, 0, 0)) return EButton_FOOTER_MENU;
		break;

	case EProgramState_CREATE_LEVEL:
		if (surfaceHitbox(gPositionMenuCustomLevelModify, x, y, 0, 0, 0, 0)) return EButton_MENU_CUSTOM_LEVEL_MODIFY;
		if (surfaceHitbox(gPositionMenuCustomLevelNew, x, y, 0, 0, 0, 0)) return EButton_MENU_CUSTOM_LEVEL_NEW;
		if (surfaceHitbox(gPositionMenuButton, x, y, 0, 0, 0, 0)) return EButton_FOOTER_MENU;
		break;

	default: ;
	}

	return EButton_UNKNOWN;
}

int surfaceLevelListManager(const int x, const int y)
{
	int i;

	for (i = 0; i < gNumberOfLevels && i < 14; ++i)
		if (x >= 40 && x <= 131 && y >= 30 + 30 * i && y <= 54 + 30 * i)
			return
				(i + 1);
	for (i = 14; i < gNumberOfLevels && i < 28; ++i)
		if (x >= 270 && x <= 361 && y >= 30 + 30 * (i - 14) && y <= 54 + 30 *
			(i - 14))
			return (i + 1);
	for (i = 28; i < gNumberOfLevels; ++i)
		if (x >= 500 && x <= 591 && y >= 30 + 30 * (i - 28) && y <= 54 + 30 * (i - 28))
			return (i + 1);
	return 0;
}

void drawWall(int x, int y)
{
	if (gCreationWall == 0)
	{
		gp1x = x;
		gp1y = y;
		gp2x = x;
		gp2y = y;
		gCreationWall = 1;
	}
	else
	{
		short xWall, yWall;
		unsigned short wWall, hWall;
		getPositionSurface(&xWall, &yWall, &wWall, &hWall);
		createWall(xWall, yWall, wWall, hWall);
		gCreationWall = 0;
	}
}

void drawGoal(int x, int y)
{
	if (gCreationGoal == 0)
	{
		gp1x = x;
		gp1y = y;
		gp2x = x;
		gp2y = y;
		gCreationGoal = 1;
	}
	else
	{
		short xGoal, yGoal;
		unsigned short wGoal, hGoal;
		getPositionSurface(&xGoal, &yGoal, &wGoal, &hGoal);
		createGoal(xGoal, yGoal, wGoal, hGoal);
		gCreationGoal = 0;
	}
}


/*----- OTHER FUNCTIONS -----*/
void changeParticleCharge(Particle* p, const int c)
{
	if (p && p->modifiable && !(p->moving))
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

void changeCursorState(const int c)
{
	int minimumHandCharge = gMIN_PARTICULE_CHARGE;

	/*if in creative mode, allows three additional charges for creating moving particles, walls and goal*/
	if (gCreativeMode) minimumHandCharge -= 3;

	if (c == -1 && gCursorState > minimumHandCharge) gCursorState = gCursorState == 1 ? -1 : gCursorState - 1;
	else if (c == 1 && gCursorState < gMAX_PARTICULE_CHARGE) gCursorState = gCursorState == -1 ? 1 : gCursorState + 1;
}
